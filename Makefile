#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

CFLAGS = -Wall -g
#TARGETS = bit2txt draw_fpga
#OBJS = $(TARGETS:=.o)
LDLIBS = -lxml2

all: bit2txt draw_fpga xc6slx9.svg

xc6slx9.svg: draw_fpga
	./draw_fpga | xmllint --pretty 1 - > $@

bit2txt: bit2txt.o helper.o

bit2txt.o:bit2txt.c helper.h

helper.o:helper.c helper.h

draw_fpga: draw_fpga.o

clean:
		rm -f bit2txt bit2txt.o helper.o draw_fpga
.PHONY:	all clean
