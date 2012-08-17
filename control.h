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

// returns a connpt dest index, or -1 (NO_CONN) if no connection was found
int fpga_conn_dest(struct fpga_model* model, int y, int x,
	const char* name, int dest_idx);
const char* fpga_conn_to(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x);

// returns a switch index, or -1 (NO_SWITCH) if no switch was found
int fpga_switch_dest(struct fpga_model* model, int y, int x,
	const char* name, int dest_idx);
const char* fpga_switch_to(struct fpga_model* model, int y, int x,
	int swidx, int* is_bidir);
