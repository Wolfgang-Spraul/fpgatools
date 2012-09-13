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

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
AR_quiet	= @echo "  AR       " $@ && $(AR_normal)

ifeq ($(V),1)
    CC		= $(CC_normal)
    AR		= $(AR_normal)
else
    CC		= $(CC_quiet)
    AR		= $(AR_quiet)
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

LIBFPGA_BIT_OBJS       = bit_frames.o bit_regs.o
LIBFPGA_MODEL_OBJS     = model_main.o model_tiles.o model_devices.o \
	model_ports.o model_conns.o model_switches.o model_helper.o
LIBFPGA_FLOORPLAN_OBJS = floorplan.o
LIBFPGA_CONTROL_OBJS   = control.o
LIBFPGA_CORES_OBJS     = parts.o helper.o

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

autotest: autotest.o model.h libfpga-model.a libfpga-floorplan.a \
	libfpga-control.a libfpga-cores.a

autotest.o: model.h floorplan.h control.h

new_fp: new_fp.o libfpga-model.a libfpga-floorplan.a libfpga-cores.a \
	libfpga-control.a

new_fp.o: new_fp.c floorplan.h model.h helper.h control.h

fp2bit: fp2bit.o libfpga-model.a libfpga-bit.a libfpga-floorplan.a \
	libfpga-control.a libfpga-cores.a

fp2bit.o: fp2bit.c model.h floorplan.h bit.h helper.h

bit2fp: bit2fp.o libfpga-model.a libfpga-bit.a libfpga-floorplan.a \
	libfpga-control.a libfpga-cores.a

bit2fp.o: bit2fp.c model.h floorplan.h bit.h helper.h

floorplan.o: floorplan.c floorplan.h model.h control.h

bit_regs.o: bit_regs.c bit.h model.h

bit_frames.o: bit_frames.c bit.h model.h

parts.o: parts.c parts.h

control.o: control.c control.h model.h

draw_svg_tiles: draw_svg_tiles.o libfpga-model.a libfpga-cores.a \
	libfpga-control.a

draw_svg_tiles.o: draw_svg_tiles.c model.h helper.h

pair2net: pair2net.o libfpga-cores.a

pair2net.o: pair2net.c helper.h

sort_seq: sort_seq.o libfpga-cores.a

sort_seq.o: sort_seq.c helper.h

merge_seq: merge_seq.o libfpga-cores.a

merge_seq.o: merge_seq.c helper.h

hstrrep: hstrrep.o libfpga-cores.a

helper.o: helper.c helper.h

model_main.o: model_main.c model.h

model_tiles.o: model_tiles.c model.h

model_devices.o: model_devices.c model.h

model_ports.o: model_ports.c model.h

model_conns.o: model_conns.c model.h

model_switches.o: model_switches.c model.h

model_helper.o: model_helper.c model.h

libfpga-cores.a: $(LIBFPGA_CORES_OBJS)
	$(AR) $@ $^

libfpga-bit.a: $(LIBFPGA_BIT_OBJS)
	$(AR) $@ $^

libfpga-model.a: $(LIBFPGA_MODEL_OBJS)
	$(AR) $@ $^

libfpga-floorplan.a: $(LIBFPGA_FLOORPLAN_OBJS)
	$(AR) $@ $^

libfpga-control.a: $(LIBFPGA_CONTROL_OBJS)
	$(AR) $@ $^

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
	rm -f $(LIBFPGA_BIT_OBJS) $(LIBFPGA_MODEL_OBJS) $(LIBFPGA_FLOORPLAN_OBJS) \
		$(LIBFPGA_CONTROL_OBJS) $(LIBFPGA_CORES_OBJS) \
		draw_svg_tiles draw_svg_tiles.o \
		new_fp new_fp.o \
		hstrrep hstrrep.o \
		sort_seq sort_seq.o \
		merge_seq merge_seq.o \
		autotest autotest.o \
		fp2bit fp2bit.o \
		bit2fp bit2fp.o \
		pair2net pair2net.o \
		libfpga-bit.a libfpga-control.a libfpga-cores.a libfpga-floorplan.a libfpga-model.a \
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
		install -m 755 fp2bit  $(DESTDIR)/$(PREFIX)/bin/
		install -m 755 bit2fp $(DESTDIR)/$(PREFIX)/bin/
uninstall:
		rm -f $(DESTDIR)/$(PREFIX)/bin/{fp2bit,bit2fp}
