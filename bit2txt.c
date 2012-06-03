//
// Authors: Wolfgang Spraul <wspraul@q-ag.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 3 of the License, or (at your option) any later version.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define PROGRAM_REVISION "2012-06-01"

// 120 MB max bitstream size is enough for now
#define BITSTREAM_READ_PAGESIZE		4096
#define BITSTREAM_READ_MAXPAGES		30000	// 120 MB max bitstream

__inline uint16_t __swab16(uint16_t x)
{
        return (((x & 0x00ffU) << 8) | \
            ((x & 0xff00U) >> 8)); \
}

__inline uint32_t __swab32(uint32_t x)
{
        return (((x & 0x000000ffUL) << 24) | \
            ((x & 0x0000ff00UL) << 8) | \
            ((x & 0x00ff0000UL) >> 8) | \
            ((x & 0xff000000UL) >> 24)); \
}

#define __be32_to_cpu(x) __swab32((uint32_t)(x))
#define __be16_to_cpu(x) __swab16((uint16_t)(x))
#define __cpu_to_be32(x) __swab32((uint32_t)(x))
#define __cpu_to_be16(x) __swab16((uint16_t)(x))

#define __le32_to_cpu(x) ((uint32_t)(x))
#define __le16_to_cpu(x) ((uint16_t)(x))
#define __cpu_to_le32(x) ((uint32_t)(x))
#define __cpu_to_le16(x) ((uint16_t)(x))

void help();

int main(int argc, char** argv)
{
	uint8_t* bit_data = 0;
	FILE* bitf = 0;
	uint32_t bit_cur, bit_eof, cmd_len;
	uint16_t str_len;
	int i;

	if (argc != 2
	    || !strcmp(argv[1], "--help")) {
		help();
		return EXIT_SUCCESS;
	}
	if (!strcmp(argv[1], "--version")) {
		printf("%s\n", PROGRAM_REVISION);
		return EXIT_SUCCESS;
	}

	// read .bit into memory
	bit_data = malloc(BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE);
	if (!bit_data) {
		fprintf(stderr, "X Cannot allocate %i bytes for bitstream.\n",
		BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE);
		goto fail;
	}
	if (!(bitf = fopen(argv[1], "rb"))) {
		fprintf(stderr, "X Error opening %s.\n", argv[1]);
		goto fail;
        }
	bit_eof = 0;
	while (bit_eof < BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE) {
		size_t num_read = fread(&bit_data[bit_eof], sizeof(uint8_t),
			BITSTREAM_READ_PAGESIZE, bitf);
		bit_eof += num_read;
		if (num_read != BITSTREAM_READ_PAGESIZE)
			break;
	}
	fclose(bitf);
	if (bit_eof >= BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE) {
		fprintf(stderr, "X Bitstream size exceeded maximum of "
			"%i bytes.\n", BITSTREAM_READ_MAXPAGES *
			BITSTREAM_READ_PAGESIZE);
		goto fail;
	}

	printf("bit2txt_format 1\n");

	// offset 0 - magic header
	if (bit_eof < 13) {
		fprintf(stderr, "X File size %i below minimum of 13 bytes.\n",
			bit_eof);
		goto fail;
	}
	printf("hex");
	for (i = 0; i < 13; i++)
		printf(" %.02x", bit_data[i]);
	printf("\n");

	// 4 strings 'a' - 'd', 16-bit length
	bit_cur = 13;
	for (i = 'a'; i <= 'd'; i++) {
		if (bit_eof < bit_cur + 3) {
			fprintf(stderr, "X Unexpected EOF at %i.\n", bit_eof);
			goto fail;
		}
		if (bit_data[bit_cur] != i) {
			fprintf(stderr, "X Expected string code '%c', got "
				"'%c'\n", i, bit_data[bit_cur]);
			goto fail;
		}
		str_len = __be16_to_cpu(*(uint16_t*)&bit_data[bit_cur + 1]);
		if (bit_eof < bit_cur + 3 + str_len) {
			fprintf(stderr, "X Unexpected EOF at %i.\n", bit_eof);
			goto fail;
		}
		if (bit_data[bit_cur + 3 + str_len - 1] != 0) {
			fprintf(stderr, "X z-terminated string ends with %0xh"
				" - aborting.\n",
				bit_data[bit_cur + 3 + str_len - 1]);
			goto fail;
		}
		printf("header_str_%c %s\n", i, &bit_data[bit_cur + 3]);
		bit_cur += 3 + str_len;
	}

	// commands
	if (bit_eof < bit_cur + 5) {
		fprintf(stderr, "X Unexpected EOF at %i.\n", bit_eof);
		goto fail;
	}
	if (bit_data[bit_cur] != 'e') {
		fprintf(stderr, "X Expected string code 'e', got '%c'\n",
			bit_data[bit_cur]);
		goto fail;
	}
	cmd_len = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur + 1]);
	if (bit_eof < bit_cur + 5 + cmd_len) {
		fprintf(stderr, "X Unexpected EOF at %i.\n", bit_eof);
		goto fail;
	}
	if (bit_eof > bit_cur + 5 + cmd_len) {
		fprintf(stderr, "X Unexpected continuation after offset "
			"%i, but continuing.\n", bit_cur + 5 + cmd_len);
	}

	// sync word 0xAA995566
	// T1_RD_reg_num
	// T2_WR_num

	free(bit_data);
	return EXIT_SUCCESS;
fail:
	free(bit_data);
	return EXIT_FAILURE;
}

void help()
{
	printf("\n"
	       "bit2txt %s - convert FPGA bitstream to text\n"
	       "(c) 2012 Wolfgang Spraul <wspraul@q-ag.de>\n"
	       "\n"
	       "bit2txt <path to .bit file>\n"
	       "  --help                       print help message\n"
	       "  --version                    print version number\n"
	       "  <path to .bit file>          bitstream to print on stdout\n"
	       "\n", PROGRAM_REVISION);
}
