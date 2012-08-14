#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

.PHONY:	all clean
.SECONDARY:
CFLAGS += -Wall -Wshadow -Wmissing-prototypes -Wmissing-declarations \
	-Wno-format-zero-length -Ofast
CFLAGS += `pkg-config libxml-2.0 --cflags`
LDLIBS += `pkg-config libxml-2.0 --libs`

MODEL_OBJ = model_main.o model_tiles.o model_devices.o model_ports.o model_conns.o model_switches.o model_helper.o

all: bit2txt draw_svg_tiles new_fp hstrrep sort_seq merge_seq

new_fp: new_fp.o $(MODEL_OBJ) helper.o

new_fp.o: new_fp.c model.h helper.h

draw_svg_tiles: draw_svg_tiles.o $(MODEL_OBJ) helper.o

draw_svg_tiles.o: draw_svg_tiles.c model.h helper.h

bit2txt: bit2txt.o helper.o

bit2txt.o: bit2txt.c helper.h

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

compare_all: compare.tiles compare.devices compare.conns compare.ports compare.sw

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

%.devices: %.fp
	@cat $<|awk '{if ($$1=="device") printf "%s %s %s\n",$$2,$$3,$$4}'|sort >$@

%.nets: %.fp pair2net
	cat $<|awk '{if ($$1=="static_conn") printf "%s-%s-%s %s-%s-%s\n",$$2,$$3,$$4,$$5,$$6,$$7}' |./pair2net -|sort >$@
	@echo Number of nets:
	@cat $@|wc -l
	@echo Number of connection points:
	@cat $@|wc -w
	@echo Largest net:
	@cat $@|awk '{if (NF>max) max=NF} END {print max}'

%.conns: %.fp sort_seq merge_seq
	@cat $<|awk '{if ($$1=="static_conn") printf "%s %s %s %s %s %s\n",$$2,$$3,$$5,$$6,$$4,$$7}'|sort|./sort_seq -|./merge_seq -|awk '{printf "%s %s %s %s %s %s\n",$$1,$$2,$$5,$$3,$$4,$$6}'|sort >$@
	
%.ports: %.fp
	@cat $<|awk '{if ($$1=="port") printf "%s %s %s\n",$$2,$$3,$$4}'|sort >$@

%.sw: %.fp
	@cat $<|awk '{if ($$1=="switch") printf "%s %s %s %s %s\n",$$2,$$3,$$4,$$5,$$6}'|sort >$@

clean:
	rm -f bit2txt bit2txt.o \
		draw_svg_tiles draw_svg_tiles.o \
		new_fp new_fp.o \
		helper.o $(MODEL_OBJ) hstrrep hstrrep.o \
		sort_seq sort_seq.o \
		merge_seq merge_seq.o \
		xc6slx9_empty.fp xc6slx9.svg \
		xc6slx9_empty.tiles xc6slx9_empty.devices xc6slx9_empty.conns \
		xc6slx9_empty.ports xc6slx9_empty.sw xc6slx9_empty.nets \
		compare_other.tiles compare_other.devices compare_other.conns compare_other.ports \
		compare_other.sw compare_other.nets \
		compare_tiles_matching.txt compare_tiles_diff.txt compare_tiles_extra.txt \
		compare_devices_matching.txt compare_devices_diff.txt compare_devices_extra.txt \
		compare_conns_matching.txt compare_conns_diff.txt compare_conns_extra.txt \
		compare_ports_matching.txt compare_ports_diff.txt compare_ports_extra.txt \
		compare_sw_matching.txt compare_sw_diff.txt compare_sw_extra.txt \
		compare_nets_matching.txt compare_nets_diff.txt compare_nets_extra.txt
