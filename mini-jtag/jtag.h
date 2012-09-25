//
// Author: Xiangfu Liu
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//


#ifndef JTAG_H
#define JTAG_H

/* FPGA Boundary-Scan Instructions */
#define EXTEST	0x0F
#define SAMPLE	0x01
#define USER1	0x02
#define USER2	0x03
#define USER3	0x1A
#define USER4	0x1B
#define CFG_OUT	0x04
#define CFG_IN	0x05
#define INTEST	0x07
#define USERCODE	0x08
#define IDCODE	0x09
#define JPROGRAM	0x0B
#define JSTART	0x0C
#define JSHUTDOWN	0x0D
#define BYPASS	0x3F

/* The max read/write size is 65536, we use 65532 here */
#define FTDI_MAX_RW_SIZE	65532

int tap_tms(struct ftdi_context *ftdi, int tms, uint8_t bit7);
void tap_reset_rti(struct ftdi_context *ftdi);
int tap_shift_ir_only(struct ftdi_context *ftdi, uint8_t ir);
int tap_shift_dr_bits_only(struct ftdi_context *ftdi,
		      uint8_t *in, uint32_t in_bits,
		      uint8_t *out);
int tap_shift_ir(struct ftdi_context *ftdi, uint8_t ir);
int tap_shift_dr_bits(struct ftdi_context *ftdi,
		      uint8_t *in, uint32_t in_bits,
		      uint8_t *out);
int ft232_flush(struct ftdi_context *ftdi);

#endif
