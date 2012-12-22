#
# Author: Xiangfu Liu
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

LDLIBS += `pkg-config libftdi --libs`
OBJS := mini-jtag.o load-bits.o jtag.o

.PHONY:	all clean
.PHONY:	install uninstall
.PHONY:	test test-counter test-blinking test-hello_world

all: mini-jtag

include ../Makefile.common

mini-jtag: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
	$(MKDEP)

install: all
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	install -m 755 mini-jtag $(DESTDIR)/$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/bin/mini-jtag

clean:
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)
	rm -f mini-jtag

%.bit:
	@echo ""
	@echo "Move the config bits file: <$@> here."
	@echo ""
	@exit 1

test: test-hello_world test-blinking test-counter

hello_world.bit:
	make -C .. hello_world fp2bit
	../hello_world | ../fp2bit - $@

test-hello_world: hello_world.bit mini-jtag
	./mini-jtag load $<
	sleep 2

blinking.bit:
	make -C .. blinking_led fp2bit
	../blinking_led | ../fp2bit - $@

test-blinking: blinking.bit mini-jtag
	./mini-jtag load $<
	sleep 2

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
