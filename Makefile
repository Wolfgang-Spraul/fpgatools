#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

CFLAGS = -Wall -g
TARGETS = bit2txt draw_fpga
OBJS = $(TARGETS:=.o)
LDLIBS = -lxml2

.PHONY:	all clean

all:		$(TARGETS)

clean:
		rm -f $(OBJS) $(TARGETS)
