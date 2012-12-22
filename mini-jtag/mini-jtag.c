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

#include "load-bits.h"
#include "jtag.h"

/* JTAG/Serial board ID */
#define VENDOR  0x20b7
#define PRODUCT 0x0713

static inline uint8_t rev8(uint8_t d)
{
    int i;
    uint8_t out = 0;

    /* (from left to right) */
    for (i = 0; i < 8; i++)
        if (d & (1 << i))
            out |= (1 << (7 - i));

    return out;
}

static uint8_t jtagcomm_checksum(uint8_t *d, uint16_t len)
{
	int i, j;
	uint16_t bytes, bits;
	uint8_t checksum = 0x01;

	bytes = len / 8;
	bits = len % 8;

	j = 0;
	if (bytes)
		for (j = 0; j < bytes; j++)
			for (i = 0; i < 8; i++)
				checksum ^= ((d[j] >> i) & 0x01) ? 1 : 0;

	if (bits)
		for (i = 0; i < bits; i++)
			checksum ^= ((d[j] >> i) & 0x01) ? 1 : 0;

	return checksum;
}

static void rev_dump(uint8_t *buf, uint16_t len)
{
	int i;
	for (i = len - 1; i >= 0 ; i--)
		printf("%02x ", buf[i]);
}

static void brd_reset(struct ftdi_context *ftdi)
{
	tap_reset_rti(ftdi);
	tap_shift_ir(ftdi, JPROGRAM);
	tap_reset_rti(ftdi);
}

static void usage(char *name)
{
	fprintf(stderr,
		"\n"
		"%s - A small JTAG program talk to FPGA chip\n"
		"Usage:\n"
		"    idcode\n"
		"    reset\n"
		"    load <bits file|- for stdin>\n"
		"    readreg <reg>\tRead configure register status\n"
		"    read|write reg <value>\n"
		"Report bugs to xiangfu@openmobilefree.net\n"
		"\n", name);
}

int main(int argc, char **argv)
{
	struct ftdi_context ftdi;
	uint8_t buf[4];
	uint8_t conf_buf[] = {SET_BITS_LOW,  0x08, 0x0b,
			      SET_BITS_HIGH, 0x00, 0x00,
			      TCK_DIVISOR,   0x00, 0x00,
			      LOOPBACK_END};

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	if (strcmp (argv[1], "idcode") && strcmp (argv[1], "reset") &&
	    strcmp (argv[1], "load")  && strcmp (argv[1], "readreg") &&
	    strcmp (argv[1], "read") && strcmp (argv[1], "write")
		) {
		usage(argv[0]);
		return 1;
	}

	/* Init */
	ftdi_init(&ftdi);
	if (ftdi_usb_open_desc(&ftdi, VENDOR, PRODUCT, 0, 0) < 0) {
		fprintf(stderr,
			"Can't open device %04x:%04x\n", VENDOR, PRODUCT);
		return 1;
	}
	ftdi_usb_reset(&ftdi);
	ftdi_set_interface(&ftdi, INTERFACE_A);
	ftdi_set_latency_timer(&ftdi, 1);
	ftdi_set_bitmode(&ftdi, 0xfb, BITMODE_MPSSE);
	if (ftdi_write_data(&ftdi, conf_buf, 10) != 10) {
		fprintf(stderr,
			"Can't configure device %04x:%04x\n", VENDOR, PRODUCT);
		return 1;
	}

	buf[0] = GET_BITS_LOW;
	buf[1] = SEND_IMMEDIATE;

	if (ftdi_write_data(&ftdi, buf, 2) != 2) {
		fprintf(stderr,
			"Can't send command to device\n");
		return 1;
	}
	ftdi_read_data(&ftdi, &buf[2], 1);
	if (!(buf[2] & 0x10)) {
		fprintf(stderr,
			"Vref not detected. Please power on target board\n");
		return 1;
	}

	if (!strcmp(argv[1], "idcode")) {
		uint8_t out[4];
		tap_reset_rti(&ftdi);
		tap_shift_dr_bits(&ftdi, NULL, 32, out);
		rev_dump(out, 4);
		printf("\n");
	}

	if (!strcmp (argv[1], "reset"))
		brd_reset(&ftdi);

	if (!strcmp (argv[1], "load")) {
		int i;
		struct load_bits *bs;
		FILE *fp;
		uint8_t *dr_data;
		uint32_t u;

		if(argc < 3) {
			usage(argv[0]);
			goto exit;
		}

		if (!strcmp(argv[2], "-"))
			fp = stdin;
		else {
			fp = fopen(argv[2], "r");
			if (!fp) {
				perror("Unable to open file");
				goto exit;
			}
		}

		bs = calloc(1, sizeof(*bs));
		if (!bs) {
			perror("memory allocation failed");
			goto exit;
		}

		if (load_bits(fp, bs) != 0) {
			fprintf(stderr, "%s not supported\n", argv[2]);
			goto free_bs;
		}

		printf("Bitstream information:\n");
		printf("\tDesign: %s\n", bs->design);
		printf("\tPart name: %s\n", bs->part_name);
		printf("\tDate: %s\n", bs->date);
		printf("\tTime: %s\n", bs->time);
		printf("\tBitstream length: %d\n", bs->length);

		/* copy data into shift register */
		dr_data = calloc(1, bs->length);
		if (!dr_data) {
			perror("memory allocation failed");
			goto free_bs;
		}

		for (u = 0; u < bs->length; u++)
			dr_data[u] = rev8(bs->data[u]);

		brd_reset(&ftdi);

		tap_shift_ir(&ftdi, CFG_IN);
		tap_shift_dr_bits(&ftdi, dr_data, bs->length * 8, NULL);

		/* ug380.pdf
		 * P161: a minimum of 16 clock cycles to the TCK */
		tap_shift_ir(&ftdi, JSTART);
		for (i = 0; i < 32; i++)
			tap_tms(&ftdi, 0, 0);

		tap_reset_rti(&ftdi);

		free(dr_data);
	free_bs:
		bits_free(bs);
		fclose(fp);
	}

	if (!strcmp(argv[1], "readreg") && argc == 3) {
		int i;
		char *err;
		uint8_t reg;
		uint8_t out[2];
		uint8_t dr_in[14];

		uint8_t in[14] = {
			0xaa, 0x99, 0x55, 0x66,
			0x00, 0x00, 0x20, 0x00,
			0x20, 0x00, 0x20, 0x00,
			0x20, 0x00
		};

		uint16_t cmd = 0x2801;	/* type 1 packet (word count = 1) */

		reg = strtol(argv[2], &err, 0);
		if((*err != 0x00) || (reg < 0) || (reg > 0x22)) {
			fprintf(stderr,
				"Invalid register, use a decimal or hexadecimal(0x...) number between 0x0 and 0x22\n");
			goto exit;
		}

		cmd |= ((reg & 0x3f) << 5);
		in[4] = (cmd & 0xff00) >> 8;
		in[5] = cmd & 0xff;

		tap_reset_rti(&ftdi);

		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 0, 0);
		tap_tms(&ftdi, 0, 0);	/* Goto shift IR */

		tap_shift_ir_only(&ftdi, CFG_IN);

		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 0, 0);
		tap_tms(&ftdi, 0, 0);	/* Goto SHIFT-DR */

		for (i = 0; i < 14; i++)
			dr_in[i] = rev8(in[i]);

		tap_shift_dr_bits_only(&ftdi, dr_in, 14 * 8, NULL);

		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);	/* Goto SELECT-IR */
		tap_tms(&ftdi, 0, 0);
		tap_tms(&ftdi, 0, 0);	/* Goto SHIFT-IR */

		tap_shift_ir_only(&ftdi, CFG_OUT);

		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 0, 0);
		tap_tms(&ftdi, 0, 0);	/* Goto SHIFT-IR */

		tap_shift_dr_bits_only(&ftdi, NULL, 2 * 8, out);

		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);
		tap_tms(&ftdi, 1, 0);	/* Goto SELECT-IR */
		tap_tms(&ftdi, 0, 0);
		tap_tms(&ftdi, 0, 0);	/* Goto SHIFT-IR */

		tap_reset_rti(&ftdi);

		out[0] = rev8(out[0]);
		out[1] = rev8(out[1]);

		printf("REG[%d]: 0x%02x%02x\n", reg, out[0], out[1]);
	}

	/* TODO:
	 *   Configuration Memory Read Procedure (IEEE Std 1149.1 JTAG) */

	if (!strcmp (argv[1], "read") && argc == 3) {
		char *err;
		uint8_t addr, checksum;
		uint8_t in[5];
		uint8_t out[4];

		/* Tell the FPGA what address we would like to read */
		addr = strtol(argv[2], &err, 0);
		if((*err != 0x00) || (addr < 0) || (addr > 4)) {
			fprintf(stderr,
				"Invalid address, use a decimal or hexadecimal(0x...) number between 0 and 4\n");
			goto exit;
		}

		addr &= 0xf;
		in[0] = addr;
		checksum = jtagcomm_checksum(in, 4);
		in[0] = (checksum << 5) | (0 << 4) | addr;

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, USER1);
		tap_shift_dr_bits(&ftdi, in, 6, NULL);
		/* Now read back the register */
		tap_shift_dr_bits(&ftdi, NULL, 32, out);

		printf("Read: ");
		rev_dump(out, 4);
		printf("\t[%d]\n",(uint32_t) (out[3] << 24 | out[2] << 16 |
					      out[1] << 8  | out[0]));

		tap_reset_rti(&ftdi);
	}

	if (!strcmp(argv[1], "write") && argc == 4) {
		char *err;
		uint8_t addr, checksum;
		uint8_t in[5];
		uint32_t value;

		value = strtol(argv[3], &err, 0);
		if (*err != 0x00) {
			fprintf(stderr,
				"Invalid value, use a decimal or hexadecimal(0x...) number\n");
			goto exit;
		}

		in[0] = value & 0x000000ff;
		in[1] = (value & 0x0000ff00) >> 8;
		in[2] = (value & 0x00ff0000) >> 16;
		in[3] = (value & 0xff000000) >> 24;

		/* Tell the FPGA what address we would like to read */
		addr = strtol(argv[2], &err, 0);
		if((*err != 0x00) || (addr < 0) || (addr > 4)) {
			fprintf(stderr,
				"Invalid address, use a decimal or hexadecimal(0x...) number between 0 and 4\n");
			goto exit;
		}

		addr &= 0xf;
		in[4] = (1 << 4) | addr;
		checksum = jtagcomm_checksum(in, 37);
		in[4] |= (checksum << 5);

		printf("Write: ");
		rev_dump(in, 5);
		printf("\n");

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, USER1);
		tap_shift_dr_bits(&ftdi, in, 38, NULL);
		tap_reset_rti(&ftdi);
	}


exit:
	/* Clean up */
	ftdi_usb_reset(&ftdi);
	ftdi_usb_close(&ftdi);
	ftdi_deinit(&ftdi);
	return 0;
}
