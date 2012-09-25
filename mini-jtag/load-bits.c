//
// Author: Xiangfu Liu
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "load-bits.h"

int read_section(FILE *bit_file, char *id, uint8_t **data, uint32_t *len)
{
	uint8_t buf[4];
	int lenbytes;

	/* first read 1 bytes, the section key */
	if (fread(buf, 1, 1, bit_file) != 1)
		return -1;

	*id = buf[0];

	/* section 'e' has 4 bytes indicating section length */
	if (*id == 'e')
		lenbytes = 4;
	else
		lenbytes = 2;

	/* first read 1 bytes */
	if (fread(buf, 1, lenbytes, bit_file) != lenbytes)
		return -1;

	/* second and third is section length */
	if (*id != 'e')
		*len = buf[0] << 8 | buf[1];
	else
		*len = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

	/* now allocate memory for data */
	*data = malloc(*len);

	if (fread(*data, 1, *len, bit_file) != *len)
		return -1;

	return 0;
}

int load_bits(FILE *bit_file, struct load_bits *bs)
{
	char sid = 0;
	uint8_t *sdata;
	uint32_t slen;

	uint8_t buf[128];
	uint8_t header[] = {
		0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
		0x0f, 0xf0, 0x00, 0x00, 0x01,
	};

	if (fread(buf, 1, sizeof (header), bit_file) != sizeof (header))
		return -1;

	if (memcmp(buf, header, sizeof (header)) != 0)
		return -1;

	while (sid != 'e') {
		if (read_section(bit_file, &sid, &sdata, &slen) != 0)
			return -1;

		/* make sure that strings are terminated */
		if (sid != 'e')
			sdata[slen-1] = '\0';

		switch (sid) {
		case 'a': bs->design = (char *)sdata; break;
		case 'b': bs->part_name = (char *)sdata; break;
		case 'c': bs->date = (char *)sdata; break;
		case 'd': bs->time = (char *)sdata; break;
		case 'e': bs->data = sdata; bs->length = slen; break;
		}
	}

	return 0;
}

void bits_free(struct load_bits *bs)
{
	if (bs->design)
		free(bs->design);
	if (bs->part_name)
		free(bs->part_name);
	if (bs->date)
		free(bs->date);
	if (bs->time)
		free(bs->time);
	if (bs->data)
		free(bs->data);
	free (bs);
}
