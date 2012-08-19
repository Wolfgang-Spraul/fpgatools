#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

PREFIX ?= /usr/local

.PHONY:	all clean install uninstall
.SECONDARY:
CFLAGS += -Wall -Wshadow -Wmissing-prototypes -Wmissing-declarations \
	-Wno-format-zero-length -Ofast
CFLAGS += `pkg-config libxml-2.0 --cflags`
LDLIBS += `pkg-config libxml-2.0 --libs`

MODEL_OBJ = model_main.o model_tiles.o model_devices.o model_ports.o model_conns.o model_switches.o model_helper.o

all: new_fp fp2bit bit2fp draw_svg_tiles \
	autotest hstrrep sort_seq merge_seq pair2net

autotest: autotest.o $(MODEL_OBJ) floorplan.o control.o helper.o model.h

autotest.c: model.h floorplan.h control.h

new_fp: new_fp.o $(MODEL_OBJ) floorplan.o helper.o control.o

new_fp.o: new_fp.c floorplan.h model.h helper.h control.h

fp2bit: fp2bit.o $(MODEL_OBJ) floorplan.o control.o bit_regs.o bit_frames.o helper.o

fp2bit.o: fp2bit.c model.h floorplan.h bit.h helper.h

bit2fp: bit2fp.o $(MODEL_OBJ) floorplan.o control.o bit_regs.o bit_frames.o helper.o

bit2fp.o: bit2fp.c model.h floorplan.h bit.h helper.h

floorplan.o: floorplan.c floorplan.h model.h control.h

bit_regs.o: bit_regs.c bit.h model.h

bit_frames.o: bit_frames.c bit.h model.h

control.o: control.c control.h model.h

draw_svg_tiles: draw_svg_tiles.o $(MODEL_OBJ) helper.o control.o

draw_svg_tiles.o: draw_svg_tiles.c model.h helper.h

pair2net: pair2net.o helper.o

pair2net.o: pair2net.c helper.h

sort_seq: sort_seq.o helper.o

sort_seq.o: sort_seq.c helper.h

merge_seq: merge_seq.o helper.o

merge_seq.o: merge_seq.c helper.h

hstrrep: hstrrep.o helper.o

helper.o: helper.c helper.h

model_main.o: model_main.c model.h

model_tiles.o: model_tiles.c model.h

model_devices.o: model_devices.c model.h

model_ports.o: model_ports.c model.h

model_conns.o: model_conns.c model.h

model_switches.o: model_switches.c model.h

model_helper.o: model_helper.c model.h

xc6slx9_empty.fp: new_fp
	./new_fp > $@

xc6slx9.svg: draw_svg_tiles
	./draw_svg_tiles | xmllint --pretty 1 - > $@

compare_all: compare.tiles compare.devs compare.conns compare.ports compare.sw

compare.%: xc6slx9_empty.%
	@comm -1 -2 $< compare_other.$* > compare_$*_matching.txt
	@echo Matching lines - compare_$*_matching.txt:
	@cat compare_$*_matching.txt | wc -l
	@diff -u compare_other.$* $< > compare_$*_diff.txt || true
	@echo Missing lines - compare_$*_diff.txt
	@cat compare_$*_diff.txt | grep ^-y | wc -l
	@cat compare_$*_diff.txt | grep ^+y > compare_$*_extra.txt || true
	@if test -s compare_$*_extra.txt; then echo Extra lines - compare_$*_extra.txt: ; cat compare_$*_extra.txt; fi;

%.tiles: %.fp
	@cat $<|awk '{if ($$1=="tile" && $$4=="name") printf "%s %s %s\n",$$2,$$3,$$5}'|sort >$@

%.devs: %.fp
	@cat $<|awk '{if ($$1=="dev") {if ($$6=="type") printf "%s %s %s %s\n",$$2,$$3,$$4,$$7; else printf "%s %s %s\n",$$2,$$3,$$4; }}'|sort >$@

%.nets: %.fp pair2net
	cat $<|awk '{if ($$1=="conn") printf "%s-%s-%s %s-%s-%s\n",$$2,$$3,$$4,$$5,$$6,$$7}' |./pair2net -|sort >$@
	@echo Number of nets:
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

clean:
	rm -f draw_svg_tiles draw_svg_tiles.o \
		new_fp new_fp.o \
		helper.o $(MODEL_OBJ) hstrrep hstrrep.o \
		sort_seq sort_seq.o \
		merge_seq merge_seq.o \
		autotest autotest.o control.o floorplan.o \
		fp2bit fp2bit.o \
		bit2fp bit2fp.o \
		bit_regs.o bit_frames.o \
		pair2net pair2net.o \
		xc6slx9_empty.fp xc6slx9.svg \
		xc6slx9_empty.tiles xc6slx9_empty.devs xc6slx9_empty.conns \
		xc6slx9_empty.ports xc6slx9_empty.sw xc6slx9_empty.nets \
		compare_other.tiles compare_other.devs compare_other.conns compare_other.ports \
		compare_other.sw compare_other.nets \
		compare_tiles_matching.txt compare_tiles_diff.txt compare_tiles_extra.txt \
		compare_devs_matching.txt compare_devs_diff.txt compare_devs_extra.txt \
		compare_conns_matching.txt compare_conns_diff.txt compare_conns_extra.txt \
		compare_ports_matching.txt compare_ports_diff.txt compare_ports_extra.txt \
		compare_sw_matching.txt compare_sw_diff.txt compare_sw_extra.txt \
		compare_nets_matching.txt compare_nets_diff.txt compare_nets_extra.txt

install:	all
		mkdir -p $(DESTDIR)/$(PREFIX)/bin/
		install -m 755 new_fp  $(DESTDIR)/$(PREFIX)/bin/
		install -m 755 bit2fp $(DESTDIR)/$(PREFIX)/bin/
uninstall:
		rm -f $(DESTDIR)/$(PREFIX)/bin/{new_fp,bit2fp}
