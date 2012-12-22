//
// Author: Xiangfu Liu
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <ftdi.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "jtag.h"

int tap_tms(struct ftdi_context *ftdi, int tms, uint8_t bit7)
{
	uint8_t buf[3];
	buf[0] = MPSSE_WRITE_TMS|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
	buf[1] = 0;		/* value = length - 1 */
	buf[2] = (tms ? 0x01 : 0x00) | ((bit7 & 0x01) << 7);
	if (ftdi_write_data(ftdi, buf, 3) != 3)
		return -1;

	return 0;
}

void tap_reset_rti(struct ftdi_context *ftdi)
{
	int i;
	for(i = 0; i < 5; i++)
		tap_tms(ftdi, 1, 0);

	tap_tms(ftdi, 0, 0);	/* Goto RTI */
}

int tap_shift_ir_only(struct ftdi_context *ftdi, uint8_t ir)
{
	int ret = 0;
	uint8_t buf[3] = {0, 0, 0};

	buf[0] = MPSSE_DO_WRITE|MPSSE_LSB|
		MPSSE_BITMODE|MPSSE_WRITE_NEG;
	buf[1] = 4;
	buf[2] = ir;
	if (ftdi_write_data(ftdi,buf, 3) != 3) {
		fprintf(stderr, "Write loop failed\n");
		ret = -1;
	}

	tap_tms(ftdi, 1, (ir >> 5));

	return ret;
}

int tap_shift_ir(struct ftdi_context *ftdi, uint8_t ir)
{
	int ret;

	tap_tms(ftdi, 1, 0);	/* RTI status */
	tap_tms(ftdi, 1, 0);
	tap_tms(ftdi, 0, 0);
	tap_tms(ftdi, 0, 0);	/* Goto shift IR */

	ret = tap_shift_ir_only(ftdi, ir);

	tap_tms(ftdi, 1, 0);
	tap_tms(ftdi, 0, 0);	/* Goto RTI */

	return ret;
}

static int shift_last_bits(struct ftdi_context *ftdi,
	       uint8_t *in, uint8_t len, uint8_t *out)
{
	uint8_t buf[3];

	if (!len)
		return -1;

	buf[0] = MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;

	if (in)
		buf[0] |= MPSSE_DO_WRITE;

	if (out)
		buf[0] |= MPSSE_DO_READ;

	buf[1] = len - 1;

	if (in)
		buf[2] = *in;

	if (ftdi_write_data(ftdi, buf, 3) != 3) {
		fprintf(stderr,
			"Ftdi write failed\n");
		return -1;
	}

	if (out)
		ftdi_read_data(ftdi, out, 1);

	return 0;
}

int tap_shift_dr_bits_only(struct ftdi_context *ftdi,
		      uint8_t *in, uint32_t in_bits,
		      uint8_t *out)
{
	/* Have to be at RTI status before call this function */
	uint8_t buf_bytes[FTDI_MAX_RW_SIZE + 3];

	uint32_t in_bytes = 0;
	uint32_t last_bits = 0;
	uint16_t last_bytes, len;
	int i, t;

	in_bytes = in_bits / 8;
	last_bits = in_bits % 8;

	/* If last_bits == 0, the last bit of last byte should send out with TMS */
	if (in_bytes) {
		t = in_bytes / FTDI_MAX_RW_SIZE;
		last_bytes = in_bytes % FTDI_MAX_RW_SIZE;

		for (i = 0; i <= t; i++) {
			len = (i == t) ? last_bytes : FTDI_MAX_RW_SIZE;

			buf_bytes[0] = MPSSE_LSB|MPSSE_WRITE_NEG;

			if (in)
				buf_bytes[0] |= MPSSE_DO_WRITE;

			if (out)
				buf_bytes[0] |= MPSSE_DO_READ;

			buf_bytes[1] = (len - 1) & 0xff;
			buf_bytes[2] = ((len - 1) >> 8) & 0xff;

			if (in)
				memcpy(&buf_bytes[3], (in + i * FTDI_MAX_RW_SIZE), len);

			if (ftdi_write_data(ftdi, buf_bytes, len + 3) != len + 3) {
				fprintf(stderr,
					"Ftdi write failed\n");
				return -1;
			}

			if (out)
				ftdi_read_data(ftdi, (out + i * FTDI_MAX_RW_SIZE), len);
		}
	}

	if (last_bits) {
		/* Send last few bits */
		shift_last_bits(ftdi, &in[in_bytes], last_bits - 1, out);
		tap_tms(ftdi, 1, (in[in_bytes] >> (last_bits - 1)));
	} else
		tap_tms(ftdi, 1, 0);

	return 0;
}

int tap_shift_dr_bits(struct ftdi_context *ftdi,
		      uint8_t *in, uint32_t in_bits,
		      uint8_t *out)
{
	int ret;

	/* Send 3 Clocks with TMS = 1 0 0 to reach SHIFTDR*/
	tap_tms(ftdi, 1, 0);
	tap_tms(ftdi, 0, 0);
	tap_tms(ftdi, 0, 0);

	ret = tap_shift_dr_bits_only(ftdi, in, in_bits, out);

	tap_tms(ftdi, 1, 0);
	tap_tms(ftdi, 0, 0);	/* Goto RTI */

	return ret;
}

int ft232_flush(struct ftdi_context *ftdi)
{
	uint8_t buf[1] = { SEND_IMMEDIATE };
	if (ftdi_write_data(ftdi, buf, 1) != 1) {
		fprintf(stderr,
			 "Can't SEND_IMMEDIATE\n");
		return -1;
	}

	return 0;
}
