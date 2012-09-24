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

uint8_t jtagcomm_checksum(uint8_t *d, int len)
{
	int i, j, bytes, bits;
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

void usage(char *name)
{
	fprintf(stderr,
		"\n"
		"%s - A small JTAG program talk to FPGA chip\n"
		"Usage:\n"
		"    idcode\n"
		"    reset\n"
		"    load <bits file>\n"
		"    cr\t\t\t\tRead configure register status\n"
		"    read|write reg <value>\n"
		"Report bugs to xiangfu@openmobilefree.net\n"
		"\n", name);
}

int main(int argc, char **argv)
{
	struct ftdi_context ftdi;
	uint8_t buf[65536];

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
	ftdi_read_data(&ftdi, &buf[3], 1);
	if (!(buf[3] & 0x10)) {
		fprintf(stderr,
			"Vref not detected. Please power on target board\n");
		return 1;
	}

	if (!strcmp(argv[1], "idcode")) {
		tap_reset_rti(&ftdi);
		tap_shift_dr_bits(&ftdi, NULL, 32, buf);
		printf("0x%02x%02x%02x%02x\n",
		       buf[3], buf[2], buf[1], buf[0]);
	}

	if (!strcmp (argv[1], "reset")) {
		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, JPROGRAM);
		tap_reset_rti(&ftdi);
	}

	if (!strcmp (argv[1], "load")) {
		if(argc < 3) {
			usage(argv[0]);
			goto exit;
		}

		struct load_bits *bs;
		FILE *pld_file;
		uint8_t *dr_data;
		uint32_t u;
		int i;

		if ((pld_file = fopen(argv[2], "r")) == NULL) {
			perror("Unable to open file");
			goto exit;
		}

		bs = calloc(1, sizeof(*bs));
		if (!bs) {
			perror("memory allocation failed");
			goto exit;
		}

		if (load_bits(pld_file, bs) != 0) {
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
		dr_data = malloc(bs->length * sizeof(char));
		for (u = 0; u < bs->length; u++) {
			/* flip bits */
			dr_data[u] |= ((bs->data[u] & 0x80) ? 1 : 0) << 0;
			dr_data[u] |= ((bs->data[u] & 0x40) ? 1 : 0) << 1;
			dr_data[u] |= ((bs->data[u] & 0x20) ? 1 : 0) << 2;
			dr_data[u] |= ((bs->data[u] & 0x10) ? 1 : 0) << 3;
			dr_data[u] |= ((bs->data[u] & 0x08) ? 1 : 0) << 4;
			dr_data[u] |= ((bs->data[u] & 0x04) ? 1 : 0) << 5;
			dr_data[u] |= ((bs->data[u] & 0x02) ? 1 : 0) << 6;
			dr_data[u] |= ((bs->data[u] & 0x01) ? 1 : 0) << 7;
		}

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, JPROGRAM);
		tap_shift_ir(&ftdi, CFG_IN);

		tap_shift_dr_bits(&ftdi, dr_data, bs->length * 8, NULL);

		/* ug380.pdf
		 * P161: a minimum of 16 clock cycles to the TCK */
		tap_shift_ir(&ftdi, JSTART);
		for (i = 0; i < 32; i++)
			tap_tms(&ftdi, 0, 0);

		tap_reset_rti(&ftdi);

	free_dr:
		free(dr_data);
	free_bs:
		bits_free(bs);
		fclose(pld_file);
	}

	if (!strcmp(argv[1], "readreg") && argc == 3) {
		uint8_t in[14] = {
			0xaa, 0x99, 0x55, 0x66,
			0x00, 0x00, 0x20, 0x00,
			0x20, 0x00, 0x20, 0x00,
			0x20, 0x00
		};

		uint16_t reg = 0x2801;	/* type 1 packet (word count = 1) */
		reg |= ((atoi(argv[2]) & 0x3f) << 5);

		in[4] = (reg & 0xff00) >> 8;
		in[5] = reg & 0x00;

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, CFG_IN);
		tap_shift_dr_bits(&ftdi, in, 14 * 8, NULL);
		tap_shift_ir(&ftdi, CFG_OUT);
		tap_shift_dr_bits(&ftdi, NULL, 2 * 8, buf);

		int i;
		printf("Read: ");
		for (i = 1; i >= 0 ; i--)
			printf("%02x ", buf[i]);
		printf(" [%d]\n",(uint32_t) (buf[1] << 8  | buf[0]));

		tap_reset_rti(&ftdi);
	}

	/* TODO:
	 *   Configuration Memory Read Procedure (IEEE Std 1149.1 JTAG) */

	/* TODO:
	 *   There is no error check on read/write paramters */
	if (!strcmp (argv[1], "read") && argc == 3) {
		int i;
		uint8_t addr, checksum;
		uint8_t in[5];

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, USER1);

		/* Tell the FPGA what address we would like to read */
		addr = atoi(argv[2]);
		addr &= 0xf;
		in[0] = addr;
		checksum = jtagcomm_checksum(in, 4);

		in[0] = (checksum << 5) | (0 << 4) | addr;

		tap_shift_dr_bits(&ftdi, in, 6, NULL);

		/* Now read back the register */
		tap_shift_dr_bits(&ftdi, NULL, 32, buf);
		printf("Read: ");
		for (i = 3; i >= 0 ; i--)
			printf("%02x ", buf[i]);
		printf(" [%d]\n",(uint32_t) (buf[3] << 24 | buf[2] << 16 |
					     buf[1] << 8  | buf[0]));

		tap_reset_rti(&ftdi);
	}

	if (!strcmp(argv[1], "write") && argc == 4) {
		int i;
		uint8_t addr, checksum;
		uint8_t in[5];
		uint32_t value;

		tap_reset_rti(&ftdi);
		tap_shift_ir(&ftdi, USER1);

		value = atoi(argv[3]);
		in[0] = value & 0x000000ff;
		in[1] = (value & 0x0000ff00) >> 8;
		in[2] = (value & 0x00ff0000) >> 16;
		in[3] = (value & 0xff000000) >> 24;

		/* Tell the FPGA what address we would like to read */
		addr = atoi(argv[2]);
		addr &= 0xf;
		in[4] = (1 << 4) | addr;
		checksum = jtagcomm_checksum(in, 37);

		in[4] |= (checksum << 5);
		printf("Write: %02x %02x %02x %02x %02x\n",
		       in[4],in[3],in[2],in[1],in[0]);

		tap_shift_dr_bits(&ftdi, in, 38, buf);

		tap_reset_rti(&ftdi);
	}


exit:
	/* Clean up */
	ftdi_usb_reset(&ftdi);
	ftdi_usb_close(&ftdi);
	ftdi_deinit(&ftdi);
	return 0;
}
