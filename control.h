//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

int fpga_find_iob(struct fpga_model* model, const char* sitename,
	int* y, int* x, int* idx);

const char* fpga_iob_sitename(struct fpga_model* model, int y, int x,
	int idx);

//
// When dealing with devices, there are two indices:
// 1. The index of the device in the device array for that tile.
// 2. The index of the device within devices of the same type in the tile.
//

// Looks up a device pointer based on the type index.
struct fpga_device* fpga_dev(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, int type_idx);

// Counts how many devices of type 'type' are in the device
// array up to devidx.
int fpga_dev_typecount(struct fpga_model* model, int y, int x,
	enum fpgadev_type type, int dev_idx);

enum { A6_LUT, B6_LUT, C6_LUT, D6_LUT };
// lut_len can be -1 (ZTERM)
int fpga_set_lut(struct fpga_model* model, struct fpga_device* dev,
	int which_lut, const char* lut_str, int lut_len);
