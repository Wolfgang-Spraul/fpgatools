#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#


CFLAGS  += `pkg-config libxml-2.0 --cflags`
CFLAGS  += -I$(CURDIR)/libs

LDLIBS  += `pkg-config libxml-2.0 --libs`

LDFLAGS += -Wl,-rpath,$(CURDIR)/libs

OBJS 	= autotest.o bit2fp.o draw_svg_tiles.o fp2bit.o hstrrep.o \
	merge_seq.o new_fp.o pair2net.o sort_seq.o hello_world.o

DYNAMIC_LIBS = libs/libfpga-model.so libs/libfpga-bit.so \
	libs/libfpga-floorplan.so libs/libfpga-control.so \
	libs/libfpga-cores.so

.PHONY:	all test clean install uninstall FAKE
.SECONDARY:

all: new_fp fp2bit bit2fp draw_svg_tiles autotest hstrrep \
	sort_seq merge_seq pair2net hello_world

include Makefile.common

%.o:	%.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(MKDEP)

libs/%.so: FAKE
	@make -C libs $(notdir $@)

test: test_logic_cfg test_iob_cfg test_routing_sw

test_logic_cfg: autotest.out/autotest_logic_cfg.diff_to_gold

test_iob_cfg: autotest.out/autotest_iob_cfg.diff_to_gold

test_routing_sw: autotest.out/autotest_routing_sw.diff_to_gold

autotest_%.diff_to_gold: autotest_%.log
	@diff -U 0 -I "^O #NODIFF" autotest.gold/autotest_$(*F).log $< > $@ || true
	@if test -s $@; then echo "Test failed: $(*F), diff follows"; cat $@; else echo "Test succeeded: $(*F)"; fi;

autotest_%.log: autotest fp2bit bit2fp
	@mkdir -p $(@D)
	./autotest --test=$(*F) 2>&1 >$@

autotest: autotest.o $(DYNAMIC_LIBS)

hello_world: hello_world.o $(DYNAMIC_LIBS)

fp2bit: fp2bit.o $(DYNAMIC_LIBS)

bit2fp: bit2fp.o $(DYNAMIC_LIBS)

new_fp: new_fp.o $(DYNAMIC_LIBS)

draw_svg_tiles: draw_svg_tiles.o $(DYNAMIC_LIBS)

pair2net: pair2net.o $(DYNAMIC_LIBS)

sort_seq: sort_seq.o $(DYNAMIC_LIBS)

merge_seq: merge_seq.o $(DYNAMIC_LIBS)

hstrrep: hstrrep.o $(DYNAMIC_LIBS)

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
	@make -C libs clean
	rm -f $(OBJS) *.d
	rm -f $(foreach test,$(TESTS),"autotest.out/autotest_$(test).diff_to_gold")
	rm -f $(foreach test,$(TESTS),"autotest.out/autotest_$(test).log")
	rmdir --ignore-fail-on-non-empty autotest.out || exit 0
	rm -f 	draw_svg_tiles new_fp hstrrep sort_seq merge_seq autotest fp2bit bit2fp pair2net \
		hello_world 
	rm -f	xc6slx9.fp xc6slx9.svg \
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
	@make -C libs install
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 fp2bit $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 bit2fp $(DESTDIR)/$(PREFIX)/bin/
	chrpath -d $(DESTDIR)/$(PREFIX)/bin/fp2bit
	chrpath -d $(DESTDIR)/$(PREFIX)/bin/bit2fp

uninstall:
	@make -C libs uninstall
	rm -f $(DESTDIR)/$(PREFIX)/bin/{fp2bit,bit2fp}
