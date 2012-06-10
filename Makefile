#
# Makefile - fpgatools
#
# Copyright 2012 by Wolfgang Spraul
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#

CFLAGS = -Wall -g
TARGETS = bit2txt draw_fpga
OBJS = $(TARGETS:=.o)
LDLIBS = -lxml2

.PHONY:	all clean

all:		$(TARGETS)

clean:
		rm -f $(OBJS) $(TARGETS)
