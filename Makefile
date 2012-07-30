#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

.PHONY:	all clean
CFLAGS = -Wall -g
LDLIBS = -lxml2

all: bit2txt draw_svg_tiles new_fp hstrrep sort_seq merge_seq xc6slx9.svg xc6slx9.fp

xc6slx9.svg: draw_svg_tiles
	./draw_svg_tiles | xmllint --pretty 1 - > $@

xc6slx9.fp: new_fp
	./new_fp > $@

bit2txt: bit2txt.o helper.o

bit2txt.o: bit2txt.c helper.h

helper.o: helper.c helper.h

model.o: model.c model.h

new_fp.o: new_fp.c model.h helper.h

draw_svg_tiles.o: draw_svg_tiles.c model.h helper.h

draw_svg_tiles: draw_svg_tiles.o model.o helper.o

new_fp: new_fp.o model.o helper.o

hstrrep: hstrrep.o helper.o

sort_seq: sort_seq.o

merge_seq: merge_seq.o

clean:
		rm -f bit2txt bit2txt.o \
			draw_svg_tiles draw_svg_tiles.o \
			new_fp new_fp.o \
			helper.o model.o
