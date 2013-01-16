#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#


CFLAGS  += -I$(CURDIR)/libs

LDFLAGS += -Wl,-rpath,$(CURDIR)/libs

OBJS 	= autotest.o bit2fp.o printf_swbits.o draw_svg_tiles.o fp2bit.o \
	hstrrep.o merge_seq.o new_fp.o pair2net.o sort_seq.o hello_world.o \
	blinking_led.o

DYNAMIC_LIBS = libs/libfpga-model.so libs/libfpga-bit.so \
	libs/libfpga-floorplan.so libs/libfpga-control.so \
	libs/libfpga-cores.so

.PHONY:	all test clean install uninstall FAKE
.SECONDARY:
.SECONDEXPANSION:

all: new_fp fp2bit bit2fp printf_swbits draw_svg_tiles autotest hstrrep \
	sort_seq merge_seq pair2net hello_world blinking_led

include Makefile.common

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
	$(MKDEP)

libs/%.so: FAKE
	@make -C libs $(notdir $@)

#
# Testing section - there are three types of tests:
#
# 1. design
#
# design tests run a design binary to produce floorplan stdout, which is
# converted to binary configuration and back to floorplan. The resulting
# floorplan after conversion through binary must match the original
# floorplan. Additionally, the original floorplan is compared to a gold
# standard if one exists.
#
# ./binary -> .fp -> .bit -> .fp -> compare first .fp to second and gold .fp
#
# 2. autotest
#
# autotest runs an automated test of some fpga logic and produces a log output
# which is compared against a gold standard.
#
# ./autotest -> stdout -> compare with gold standard
#
# 3. compare
#
# tool output is parsed to extract different types of model data and compare
# against a gold standard.
#
# tool output -> awk/processing -> compare to gold standard
#
# - extensions
#
# .ftest = fpgatools run test (design, autotest, compare)
#
# .f2gd = fpgatools to-gold diff
# .ffbd = diff between first fp and after roundtrip through binary config
# .fb2f = fpgatools binary config back to floorplan
# .ff2b = fpgatools floorplan to binary config
# .fco = fpgatools compare output for missing/extra
# .fcr = fpgatools compare missing/extra result
# .fcm = fpgatools compare match
# .fcd = fpgatools compare diff
# .fce = fpgatools compare extra
# .fao = fpgatools autotest output
# .far = fpgatools autotest result (diff to gold output)
#

test_dirs := $(shell mkdir -p test.gold test.out)

DESIGN_TESTS := hello_world blinking_led
AUTO_TESTS := logic_cfg routing_sw io_sw iob_cfg lut_encoding
COMPARE_TESTS := xc6slx9_tiles xc6slx9_devs xc6slx9_ports xc6slx9_conns xc6slx9_sw xc6slx9_swbits

DESIGN_GOLD := $(foreach target, $(DESIGN_TESTS), test.gold/design_$(target).fp)
AUTOTEST_GOLD := $(foreach target, $(AUTO_TESTS), test.gold/autotest_$(target).fao)
COMPARE_GOLD := $(foreach target, $(COMPARE_TESTS), test.gold/compare_$(target).fco)

test_gold: design_gold autotest_gold compare_gold
design_gold: $(DESIGN_GOLD)
autotest_gold: $(AUTOTEST_GOLD)
compare_gold: $(COMPARE_GOLD)

test: test_design test_auto test_compare
test_design: $(foreach target, $(DESIGN_TESTS), test.out/design_$(target).ftest)
test_auto: $(foreach target, $(AUTO_TESTS), test.out/autotest_$(target).ftest)
test_compare: $(foreach target, $(COMPARE_TESTS), test.out/compare_$(target).ftest)

# design testing targets

design_%.ftest: design_%.ffbd
	@if test -s $<; then echo "Design test: $(*F) - failed, diff follows"; cat $<; else echo "Design test: $(*F) - succeeded"; fi;
	@if test -s test.gold/$(basename $(@F)).fp; then diff -U 0 test.gold/$(basename $(@F)).fp $(basename $@).fp > $(basename $@).f2gd || (echo Diff to gold: && cat $(basename $@).f2gd); fi;

%.ffbd: %.fp %.fb2f
	@diff -u $(basename $@).fp $(basename $@).fb2f >$@ || true

%.fb2f: %.ff2b bit2fp
	@./bit2fp $< >$@ 2>&1

%.ff2b: %.fp fp2bit
	@./fp2bit $< $@

design_%.fp: $$*
	@./$(*F) >$@ 2>&1

# autotest targets

autotest_%.ftest: autotest_%.far
	@if test -s $<; then echo "Test failed: $(*F) (autotest), diff follows"; cat $<; else echo "Test succeeded: $(*F) (autotest)"; fi;

%.far: %.fao
	@if test ! -e test.gold/$(*F).fao; then echo Gold test.gold/$(*F).fao does not exist, aborting.; false; fi;
	@diff -U 0 -I "^O #NODIFF" test.gold/$(*F).fao $< >$@ || true

autotest_%.fao: autotest fp2bit bit2fp
	./autotest --test=$(*F) >$@ 2>&1

# compare testing targets

compare_%.ftest: compare_%.fcr
	@echo Test: $(*F) \(compare\)
	@cat $<|awk '{print " "$$0}'

%.fcr: %.fcm %.fcd %.fce
	@echo Matching lines - $*.fcm >$@
	@cat $*.fcm | wc -l >>$@
	@echo Missing lines - $*.fcd >>$@
	@cat $*.fcd | grep ^-[^-] | wc -l >>$@
	@if test -s $*.fce; then echo Extra lines - $*.fce: >>$@; cat $*.fce >>$@; fi;

%.fcm: %.fco
	@if test ! -e test.gold/$(*F).fco; then echo Gold test.gold/$(*F).fco does not exist, aborting.; false; fi;
	@comm -1 -2 $< test.gold/$(*F).fco >$@

%.fcd: %.fco
	@if test ! -e test.gold/$(*F).fco; then echo Gold test.gold/$(*F).fco does not exist, aborting.; false; fi;
	@diff -u test.gold/$(*F).fco $< >$@ || true

%.fce: %.fcd
	@cat $< | grep ^+[^+] >$@ || true

compare_%_tiles.fco: compare_%.fp
	@cat $<|awk '{if ($$1=="tile" && $$4=="name") printf "%s %s %s\n",$$2,$$3,$$5}'|sort >$@

compare_%_devs.fco: compare_%.fp
	@cat $<|awk '{if ($$1=="dev") {if ($$6=="type") printf "%s %s %s %s\n",$$2,$$3,$$4,$$7; else printf "%s %s %s\n",$$2,$$3,$$4; }}'|sort >$@

compare_%_ports.fco: compare_%.fp
	@cat $<|awk '{if ($$1=="port") printf "%s %s %s\n",$$2,$$3,$$4}'|sort >$@

compare_%_conns.fco: compare_%.fp sort_seq merge_seq
	@cat $<|awk '{if ($$1=="conn") printf "%s %s %s %s %s %s\n",$$2,$$3,$$5,$$6,$$4,$$7}'|sort|./sort_seq -|./merge_seq -|awk '{printf "%s %s %s %s %s %s\n",$$1,$$2,$$5,$$3,$$4,$$6}'|sort >$@

compare_%_sw.fco: compare_%.fp
	@cat $<|awk '{if ($$1=="sw") printf "%s %s %s %s %s\n",$$2,$$3,$$4,$$5,$$6}'|sort >$@

compare_%_swbits.fco: printf_swbits
	@./printf_swbits | sort > $@

compare_%.fp: new_fp
	@./new_fp >$@

# todo: .cnets not integrated yet
%.cnets: %.fp pair2net
	cat $<|awk '{if ($$1=="conn") printf "%s-%s-%s %s-%s-%s\n",$$2,$$3,$$4,$$5,$$6,$$7}' |./pair2net -|sort >$@
	@echo Number of conn nets:
	@cat $@|wc -l
	@echo Number of connection points:
	@cat $@|wc -w
	@echo Largest net:
	@cat $@|awk '{if (NF>max) max=NF} END {print max}'

#
# end of testing section
#

autotest: autotest.o $(DYNAMIC_LIBS)

hello_world: hello_world.o $(DYNAMIC_LIBS)

blinking_led: blinking_led.o $(DYNAMIC_LIBS)

fp2bit: fp2bit.o $(DYNAMIC_LIBS)

bit2fp: bit2fp.o $(DYNAMIC_LIBS)

printf_swbits: printf_swbits.o $(DYNAMIC_LIBS)

new_fp: new_fp.o $(DYNAMIC_LIBS)

draw_svg_tiles: CFLAGS += `pkg-config libxml-2.0 --cflags`
draw_svg_tiles: LDLIBS += `pkg-config libxml-2.0 --libs`
draw_svg_tiles: draw_svg_tiles.o $(DYNAMIC_LIBS)

pair2net: pair2net.o $(DYNAMIC_LIBS)

sort_seq: sort_seq.o $(DYNAMIC_LIBS)

merge_seq: merge_seq.o $(DYNAMIC_LIBS)

hstrrep: hstrrep.o $(DYNAMIC_LIBS)

xc6slx9.fp: new_fp
	./new_fp > $@

xc6slx9.svg: draw_svg_tiles
	./draw_svg_tiles | xmllint --pretty 1 - > $@

clean:
	@make -C libs clean
	rm -f $(OBJS) *.d
	rm -f 	draw_svg_tiles new_fp hstrrep sort_seq merge_seq autotest
	rm -f	fp2bit bit2fp printf_swbits pair2net hello_world blinking_led
	rm -f	xc6slx9.fp xc6slx9.svg
	rm -f	$(DESIGN_GOLD) $(AUTOTEST_GOLD) $(COMPARE_GOLD)
	rm -f	test.gold/compare_xc6slx9.fp
	rm -f	$(foreach f, $(DESIGN_TESTS), test.out/design_$(f).ffbd)
	rm -f	$(foreach f, $(DESIGN_TESTS), test.out/design_$(f).ff2b)
	rm -f	$(foreach f, $(DESIGN_TESTS), test.out/design_$(f).fb2f)
	rm -f	$(foreach f, $(DESIGN_TESTS), test.out/design_$(f).f2gd)
	rm -f	$(foreach f, $(DESIGN_TESTS), test.out/design_$(f).fp)
	rm -f	test.out/autotest_*
	rm -f	$(foreach f, $(COMPARE_TESTS), test.out/compare_$(f).fco)
	rm -f	$(foreach f, $(COMPARE_TESTS), test.out/compare_$(f).fcr)
	rm -f	$(foreach f, $(COMPARE_TESTS), test.out/compare_$(f).fcm)
	rm -f	$(foreach f, $(COMPARE_TESTS), test.out/compare_$(f).fcd)
	rm -f	$(foreach f, $(COMPARE_TESTS), test.out/compare_$(f).fce)
	rm -f	test.out/compare_xc6slx9.fp
	rmdir --ignore-fail-on-non-empty test.out test.gold

install: fp2bit bit2fp
	@make -C libs install
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 fp2bit $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 bit2fp $(DESTDIR)/$(PREFIX)/bin/
	chrpath -d $(DESTDIR)/$(PREFIX)/bin/fp2bit
	chrpath -d $(DESTDIR)/$(PREFIX)/bin/bit2fp

uninstall:
	@make -C libs uninstall
	rm -f $(DESTDIR)/$(PREFIX)/bin/{fp2bit,bit2fp}
