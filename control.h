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

// returns the number of outgoing connections for the
// connection point given with 'name', and the connection
// point's first dest offset in connpt_dests_o.
int fpga_connpt_find(struct fpga_model* model, int y, int x,
	str16_t name_i, int* connpt_dests_o);

void fpga_conn_dest(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x, str16_t* str_i);

typedef int swidx_t; // swidx_t is an index into the uint32_t switches array
#define MAX_SW_DEPTH 64 // largest seen so far was 20

struct sw_set
{
	swidx_t sw[MAX_SW_DEPTH];
	int len;
};

// returns a switch index, or -1 (NO_SWITCH) if no switch was found
swidx_t fpga_switch_first(struct fpga_model* model, int y, int x,
	str16_t name_i, int from_to);
swidx_t fpga_switch_next(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to);
swidx_t fpga_switch_backtofirst(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to);

const char* fpga_switch_str(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to);
str16_t fpga_switch_str_i(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to);
int fpga_switch_is_bidir(struct fpga_model* model, int y, int x,
	swidx_t swidx);
int fpga_switch_is_enabled(struct fpga_model* model, int y, int x,
	swidx_t swidx);
void fpga_switch_enable(struct fpga_model* model, int y, int x,
	swidx_t swidx);
int fpga_switch_set_enable(struct fpga_model* model, int y, int x,
	struct sw_set* set);
void fpga_switch_disable(struct fpga_model* model, int y, int x,
	swidx_t swidx);

const char* fmt_sw(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to);
const char* fmt_swset(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to);

struct sw_chain
{
	// start and recurring values:
	struct fpga_model* model;
	int y;
	int x;
	// start_switch will be set to STRIDX_NO_ENTRY (0) after the first call
	str16_t start_switch;
	int from_to;
	int max_chain_size;

	// return values:
	struct sw_set set;

	// internal:
	int first_round;	
};

// Returns 0 if another switchset is returned in chain, or
// NO_SWITCH (-1) if there is no other switchset.
// chain_size set to 0 when there are no more switches in the tree
int fpga_switch_chain(struct sw_chain* chain);

// returns error code or 0 for no errors
int fpga_switch_chains(struct sw_chain* chain, int max_sets,
	struct sw_set* sets, int* num_sets);

struct sw_conns
{
	// start and recurring values:
   	struct fpga_model* model;
	int y;
	int x;
	// start_switch will be set to STRIDX_NO_ENTRY (0) after first call
	str16_t start_switch;
	int max_switch_depth;

	// return values:
	struct sw_chain chain;
	int connpt_dest_start;
	int num_dests;
	int dest_i;
	int dest_y;
	int dest_x;
	str16_t dest_str_i;
};

// Returns 0 if another connection is returned in conns, or
// NO_CONN (-1) if there is no other connection.
int fpga_switch_conns(struct sw_conns* conns);

void printf_swchain(struct fpga_model* model, int y, int x,
	str16_t sw, int max_depth, int from_to);
void printf_swconns(struct fpga_model* model, int y, int x,
	str16_t sw, int max_depth);

#define SWTO_YX_DEF			0
#define SWTO_YX_CLOSEST			0x0001
#define SWTO_YX_TARGET_CONNPT		0x0002
#define SWTO_YX_MAX_SWITCH_DEPTH	0x0004

struct switch_to_yx
{
	// input:
	int yx_req; // YX_-value
	int flags; // SWTO_YX-value
	struct fpga_model* model;
	int y;
	int x;
	str16_t start_switch;
	int max_switch_depth; // only if SWTO_YX_MAX_SWITCH_DEPTH is set
	str16_t target_connpt; // only if SWTO_YX_TARGET_CONNPT is set

	// output:
	struct sw_set set;
	int dest_y;
	int dest_x;
	str16_t dest_connpt;
};

int fpga_switch_to_yx(struct switch_to_yx* p);
