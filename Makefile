#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

PREFIX ?= /usr/local

# ----- Verbosity control -----------------------------------------------------

CPP := $(CPP)   # make sure changing CC won't affect CPP

CC_normal	:= $(CC)
AR_normal	:= $(AR) rsc
DEPEND_normal	:= $(CPP) $(CFLAGS) -D__OPTIMIZE__ -MM -MG

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
AR_quiet	= @echo "  AR       " $@ && $(AR_normal)
DEPEND_quiet	= @$(DEPEND_normal)

ifeq ($(V),1)
    CC		= $(CC_normal)
    AR		= $(AR_normal)
    DEPEND	= $(DEPEND_normal)
else
    CC		= $(CC_quiet)
    AR		= $(AR_quiet)
    DEPEND	= $(DEPEND_quiet)
endif

# ----- Rules -----------------------------------------------------------------

.PHONY:	all test clean install uninstall
.SECONDARY:

# -fno-omit-frame-pointer and -ggdb add almost nothing to execution
# time right now, so we can leave them in all the time.
CFLAGS_DBG = -fno-omit-frame-pointer -ggdb
CFLAGS += $(CFLAGS_DBG)

CFLAGS += -Wall -Wshadow -Wmissing-prototypes -Wmissing-declarations \
	-Wno-format-zero-length -Ofast
CFLAGS += `pkg-config libxml-2.0 --cflags`
LDLIBS += `pkg-config libxml-2.0 --libs`
LDFLAGS += -Wl,-rpath,$(CURDIR)

LIBFPGA_BIT_OBJS       = bit_frames.o bit_regs.o
LIBFPGA_MODEL_OBJS     = model_main.o model_tiles.o model_devices.o \
	model_ports.o model_conns.o model_switches.o model_helper.o
LIBFPGA_FLOORPLAN_OBJS = floorplan.o
LIBFPGA_CONTROL_OBJS   = control.o
LIBFPGA_CORES_OBJS     = parts.o helper.o

OBJS 	= autotest.o bit2fp.o draw_svg_tiles.o fp2bit.o hstrrep.o \
	merge_seq.o new_fp.o pair2net.o sort_seq.o

OBJS	+= $(LIBFPGA_BIT_OBJS) $(LIBFPGA_MODEL_OBJS) $(LIBFPGA_FLOORPLAN_OBJS) \
	$(LIBFPGA_CONTROL_OBJS) $(LIBFPGA_CORES_OBJS)

DYNAMIC_LIBS = libfpga-model.so libfpga-bit.so libfpga-floorplan.so \
	libfpga-control.so libfpga-cores.so

DYNAMIC_HEADS = bit.h control.h floorplan.h helper.h model.h parts.h
#- libfpga-test       autotest suite
#- libfpga-design     larger design elements on top of libfpga-control

all: new_fp fp2bit bit2fp draw_svg_tiles \
	autotest hstrrep sort_seq merge_seq pair2net

test: test_logic_cfg test_routing_sw

test_logic_cfg: autotest.out/autotest_logic_cfg.diff_to_gold

test_routing_sw: autotest.out/autotest_routing_sw.diff_to_gold

autotest_%.diff_to_gold: autotest_%.log
	@diff -U 0 -I "^O #NODIFF" autotest.gold/autotest_$(*F).log $< > $@ || true
	@if test -s $@; then echo "Test failed: $(*F), diff follows"; cat $@; else echo "Test succeeded: $(*F)"; fi;

autotest_%.log: autotest fp2bit bit2fp
	@mkdir -p $(@D)
	./autotest --test=$(*F) 2>&1 >$@

autotest: autotest.o $(DYNAMIC_LIBS)

fp2bit: fp2bit.o $(DYNAMIC_LIBS)

bit2fp: bit2fp.o $(DYNAMIC_LIBS)

new_fp: new_fp.o $(DYNAMIC_LIBS)

draw_svg_tiles: draw_svg_tiles.o $(DYNAMIC_LIBS)

pair2net: pair2net.o $(DYNAMIC_LIBS)

sort_seq: sort_seq.o $(DYNAMIC_LIBS)

merge_seq: merge_seq.o $(DYNAMIC_LIBS)

hstrrep: hstrrep.o $(DYNAMIC_LIBS)

libfpga-cores.so: $(addprefix build-libs/,$(LIBFPGA_CORES_OBJS))

libfpga-bit.so: $(addprefix build-libs/,$(LIBFPGA_BIT_OBJS))

libfpga-model.so: $(addprefix build-libs/,$(LIBFPGA_MODEL_OBJS))

libfpga-floorplan.so: $(addprefix build-libs/,$(LIBFPGA_FLOORPLAN_OBJS))

libfpga-control.so: $(addprefix build-libs/,$(LIBFPGA_CONTROL_OBJS))

%.so:
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

build-libs/%.o:	%.c
	@mkdir -p build-libs
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<
	$(MKDEP)

xc6slx9.fp: new_fp
	./new_fp > $@

xc6slx9.svg: draw_svg_tiles
	./draw_svg_tiles | xmllint --pretty 1 - > $@

compare_all: compare_tiles compare_devs compare_conns \
	compare_ports compare_sw compare_swbits

compare_gold: compare.gold/xc6slx9.swbits

compare_%: xc6slx9.%
	@comm -1 -2 $< compare.gold/$< > compare_$*_matching.txt
	@echo Matching lines - compare_$*_matching.txt:
	@cat compare_$*_matching.txt | wc -l
	@diff -u compare.gold/$< $< > compare_$*_diff.txt || true
	@echo Missing lines - compare_$*_diff.txt
	@cat compare_$*_diff.txt | grep ^-[^-] | wc -l
	@cat compare_$*_diff.txt | grep ^+[^+] > compare_$*_extra.txt || true
	@if test -s compare_$*_extra.txt; then echo Extra lines - compare_$*_extra.txt: ; cat compare_$*_extra.txt; fi;

%.tiles: %.fp
	@cat $<|awk '{if ($$1=="tile" && $$4=="name") printf "%s %s %s\n",$$2,$$3,$$5}'|sort >$@

%.devs: %.fp
	@cat $<|awk '{if ($$1=="dev") {if ($$6=="type") printf "%s %s %s %s\n",$$2,$$3,$$4,$$7; else printf "%s %s %s\n",$$2,$$3,$$4; }}'|sort >$@

%.cnets: %.fp pair2net
	cat $<|awk '{if ($$1=="conn") printf "%s-%s-%s %s-%s-%s\n",$$2,$$3,$$4,$$5,$$6,$$7}' |./pair2net -|sort >$@
	@echo Number of conn nets:
	@cat $@|wc -l
	@echo Number of connection points:
	@cat $@|wc -w
	@echo Largest net:
	@cat $@|awk '{if (NF>max) max=NF} END {print max}'

%.conns: %.fp sort_seq merge_seq
	@cat $<|awk '{if ($$1=="conn") printf "%s %s %s %s %s %s\n",$$2,$$3,$$5,$$6,$$4,$$7}'|sort|./sort_seq -|./merge_seq -|awk '{printf "%s %s %s %s %s %s\n",$$1,$$2,$$5,$$3,$$4,$$6}'|sort >$@

%.ports: %.fp
	@cat $<|awk '{if ($$1=="port") printf "%s %s %s\n",$$2,$$3,$$4}'|sort >$@

%.sw: %.fp
	@cat $<|awk '{if ($$1=="sw") printf "%s %s %s %s %s\n",$$2,$$3,$$4,$$5,$$6}'|sort >$@

%.swbits: bit2fp
	./bit2fp --printf-swbits | sort > $@

clean:
	rm -f $(foreach test,$(TESTS),"autotest.out/autotest_$(test).diff_to_gold")
	rm -f $(foreach test,$(TESTS),"autotest.out/autotest_$(test).log")
	rmdir --ignore-fail-on-non-empty autotest.out || exit 0
	rm -rf build-libs
	rm -f $(OBJS) *.d *.so \
		draw_svg_tiles new_fp hstrrep sort_seq merge_seq autotest fp2bit bit2fp pair2net \
		xc6slx9.fp xc6slx9.svg \
		xc6slx9.tiles xc6slx9.devs xc6slx9.conns \
		xc6slx9.ports xc6slx9.sw xc6slx9.cnets xc6slx9.swbits \
		compare_tiles_matching.txt compare_tiles_diff.txt compare_tiles_extra.txt \
		compare_devs_matching.txt compare_devs_diff.txt compare_devs_extra.txt \
		compare_conns_matching.txt compare_conns_diff.txt compare_conns_extra.txt \
		compare_ports_matching.txt compare_ports_diff.txt compare_ports_extra.txt \
		compare_sw_matching.txt compare_sw_diff.txt compare_sw_extra.txt \
		compare_cnets_matching.txt compare_cnets_diff.txt compare_cnets_extra.txt \
		compare_swbits_matching.txt compare_swbits_diff.txt compare_swbits_extra.txt

install:	all
		mkdir -p $(DESTDIR)/$(PREFIX)/bin/
		mkdir -p $(DESTDIR)/$(PREFIX)/lib/
		install -m 755 fp2bit  $(DESTDIR)/$(PREFIX)/bin/
		install -m 755 bit2fp $(DESTDIR)/$(PREFIX)/bin/
		install -m 755 $(DYNAMIC_LIBS)  $(DESTDIR)/$(PREFIX)/lib/
		mkdir -p $(DESTDIR)/$(PREFIX)/include/
		install -m 644 $(DYNAMIC_HEADS) $(DESTDIR)/$(PREFIX)/include/

uninstall:
		rm -f $(DESTDIR)/$(PREFIX)/bin/{fp2bit,bit2fp}


# ----- Dependencies ----------------------------------------------------------

MKDEP =									\
	$(DEPEND) $< |							\
	  sed 								\
	    -e 's|^$(basename $(notdir $<)).o:|$@:|'			\
	    -e '/^\(.*:\)\? */{p;s///;s/ *\\\?$$/ /;s/  */:\n/g;H;}'	\
	    -e '$${g;p;}'						\
	    -e d >$(basename $@).d;					\
	  [ "$${PIPESTATUS[*]}" = "0 0" ] ||				\
	  { rm -f $(basename $@).d; exit 1; }

%.o:	%.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(MKDEP)

-include $(OBJS:.o=.d)
