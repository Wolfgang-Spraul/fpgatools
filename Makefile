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

all: bit2txt draw_svg_tiles xc6slx9.svg

xc6slx9.svg: draw_svg_tiles
	./draw_svg_tiles | xmllint --pretty 1 - > $@

bit2txt: bit2txt.o helper.o

bit2txt.o: bit2txt.c helper.h

helper.o: helper.c helper.h

model.o: model.c model.h

draw_svg_tiles: draw_svg_tiles.o model.o

clean:
		rm -f bit2txt bit2txt.o \
			draw_svg_tiles draw_svg_tiles.o \
			helper.o model.o
