#
# Makefile - fpgatools
# Author: Wolfgang Spraul
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

LIBS_VERSION = 0

LIBFPGA_BIT_OBJS       = bit_frames.o bit_regs.o
LIBFPGA_MODEL_OBJS     = model_main.o model_tiles.o model_devices.o \
	model_ports.o model_conns.o model_switches.o model_helper.o
LIBFPGA_FLOORPLAN_OBJS = floorplan.o
LIBFPGA_CONTROL_OBJS   = control.o
LIBFPGA_CORES_OBJS     = parts.o helper.o

OBJS := $(LIBFPGA_CORES_OBJS) $(LIBFPGA_BIT_OBJS) $(LIBFPGA_MODEL_OBJS) \
	$(LIBFPGA_FLOORPLAN_OBJS) $(LIBFPGA_CONTROL_OBJS)

DYNAMIC_LIBS = libfpga-model.so libfpga-bit.so libfpga-floorplan.so \
	libfpga-control.so libfpga-cores.so

DYNAMIC_HEADS = bit.h control.h floorplan.h helper.h model.h parts.h

.PHONY:	all clean install uninstall FAKE

all: libfpga-model.so libfpga-bit.so libfpga-floorplan.so \
	libfpga-control.so libfpga-cores.so

include ../Makefile.common

libfpga-cores.so: $(LIBFPGA_CORES_OBJS)

libfpga-bit.so: $(LIBFPGA_BIT_OBJS)

libfpga-model.so: $(LIBFPGA_MODEL_OBJS)

libfpga-floorplan.so: $(LIBFPGA_FLOORPLAN_OBJS)

libfpga-control.so: $(LIBFPGA_CONTROL_OBJS)

%.so:
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@.$(LIBS_VERSION) -o $@.$(LIBS_VERSION) $^
	@rm -f $@ && ln -s $@.$(LIBS_VERSION) $@

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<
	$(MKDEP)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(DYNAMIC_LIBS) $(DYNAMIC_LIBS:.so=.so.$(LIBS_VERSION))

install: all
	mkdir -p $(DESTDIR)/$(PREFIX)/lib/
	mkdir -p $(DESTDIR)/$(PREFIX)/include/
	install -m 644 $(DYNAMIC_HEADS) $(DESTDIR)/$(PREFIX)/include/
	for f in $(DYNAMIC_LIBS); do \
		chrpath -d $$f.$(LIBS_VERSION) && \
		install -m 644 $$f.$(LIBS_VERSION) $(DESTDIR)/$(PREFIX)/lib/$$f.$(LIBS_VERSION) && \
		(cd $(DESTDIR)/$(PREFIX)/lib/ && ln -s $$f.$(LIBS_VERSION) $$f) \
		|| exit 1; done

uninstall:
	for f in $(DYNAMIC_HEADS); do rm -f $(DESTDIR)/$(PREFIX)/include/$$f || exit 1; done
	for f in $(DYNAMIC_LIBS); do rm -f $(DESTDIR)/$(PREFIX)/lib/$$f* || exit 1; done
