//
// Author: Xiangfu Liu
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#ifndef LOAD_BITS_H
#define LOAD_BITS_H

struct load_bits {
    char *design;
    char *part_name;
    char *date;
    char *time;
    uint32_t   length;
    uint8_t    *data;
};

int read_section(FILE *bit_file, char *id, uint8_t **data, uint32_t *len);
int load_bits(FILE *bit_file, struct load_bits *bs);
void bits_free(struct load_bits *bs);

#endif /* LOAD_BITS_H */
