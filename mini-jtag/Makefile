#
# Author: Xiangfu Liu
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

PREFIX ?= /usr/local

CFLAGS += -g
LDLIBS += `pkg-config libftdi --libs`
OBJS = mini-jtag.o load-bits.o jtag.o

# ----- Verbosity control -----------------------------------------------------

CC_normal	:= $(CC)
BUILD_normal	:=
DEPEND_normal	:= $(CPP) $(CFLAGS) -D__OPTIMIZE__ -MM -MG

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
BUILD_quiet	= @echo "  BUILD    " $@ && $(BUILD_normal)
DEPEND_quiet	= @$(DEPEND_normal)

ifeq ($(V),1)
    CC		= $(CC_normal)
    BUILD	= $(BUILD_normal)
    DEPEND	= $(DEPEND_normal)
else
    CC		= $(CC_quiet)
    BUILD	= $(BUILD_quiet)
    DEPEND	= $(DEPEND_quiet)
endif

# ----- Rules -----------------------------------------------------------------

.PHONY:		all clean
.PHONY:		install uninstall

all: mini-jtag

mini-jtag: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

install: all
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 mini-jtag $(DESTDIR)/$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/bin/mini-jtag

clean:
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)
	rm -f mini-jtag

# ----- Dependencies ----------------------------------------------------------

MKDEP =									\
	$(DEPEND) $< |							\
	  sed 								\
	    -e 's|^$(basename $(notdir $<)).o:|$@:|'			\
	    -e '/^\(.*:\)\? */{p;s///;s/ *\\\?$$/ /;s/  */:\n/g;H;}'	\
	    -e '$${g;p;}'						\
	    -e d >$(basename $@).d;					\
	  [ "$${PIPESTATUS[*]}" = "0 0" ] ||				\
	  { rm -f $(basename $@).d; exit 1; }

%.o:		%.c
		$(CC) $(CFLAGS) -o $@ -c $<
		@$(MKDEP)

-include $(OBJS:.o=.d)

# ----- Test -------------------------------------------------------------------

%.bit:
	wget -O $@ http://downloads.qi-hardware.com/people/xiangfu/mini-slx9/firmware/$@ || { rm -f $@; exit 1; }

test-counter: counter.bit mini-jtag
	@./mini-jtag load $<
	@echo "Read counter registers (1 ~ 5)"
	@./mini-jtag read 0		# read version
	@./mini-jtag read 1		# read counter EN
	@./mini-jtag read 2		# read counter WE
	@./mini-jtag read 3		# read counter_set
	@./mini-jtag read 4		# read counter
	@echo "Read counter"
	@./mini-jtag write 1 1		# enable counter
	@./mini-jtag read 4		# read counter
	@./mini-jtag write 1 0		# disable counter
	@echo "Set counter to 50000000"
	@./mini-jtag write 2 1		# set counter WE
	@./mini-jtag write 3 50000000	# set counter
	@./mini-jtag read 3		# read counter_set
	@./mini-jtag write 2 0		# set counter WE
	@echo "Read counter"
	@./mini-jtag write 1 1		# enable counter
	@./mini-jtag read 4		# read counter
	@echo "Sleep 1 second for count"
	@sleep 1
	@./mini-jtag read 4		# read counter
	@./mini-jtag write 1 0		# disable counter

test-blinking: blinking.bit mini-jtag
	./mini-jtag idcode
	./mini-jtag load $<
	sleep 1
	./mini-jtag reset
	./mini-jtag load $<

