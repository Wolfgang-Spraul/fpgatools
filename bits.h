//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

struct fpga_config
{
	int param1, param2;
	uint8_t* bits;
	uint8_t* bram_data;
};

// read_bitfile() write_bitfile()
// get_bit() set_bit()

int read_bits(struct fpga_model* model, FILE* f);
int write_bits(FILE* f, struct fpga_model* model);
