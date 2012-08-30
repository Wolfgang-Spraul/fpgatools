//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

int fpga_find_iob(struct fpga_model* model, const char* sitename,
	int* y, int* x, dev_type_idx_t* idx);

const char* fpga_iob_sitename(struct fpga_model* model, int y, int x,
	dev_type_idx_t idx);

//
// When dealing with devices, there are two indices:
// 1. The index of the device in the device array for that tile.
// 2. The index of the device within devices of the same type in the tile.
//

const char* fdev_type2str(enum fpgadev_type type);
enum fpgadev_type fdev_str2type(const char* str, int len);

// returns 0 if device not found
struct fpga_device* fdev_p(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx);

// Looks up a device index based on the type index.
// returns NO_DEV (-1) if not found
dev_idx_t fpga_dev_idx(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx);

// Counts how many devices of the same type as dev_idx are in
// the array up to dev_idx.
dev_type_idx_t fdev_typeidx(struct fpga_model* model, int y, int x,
	dev_idx_t dev_idx);

#define PINW_NO_IDX -1
pinw_idx_t fdev_pinw_str2idx(int devtype, const char* str, int len);
// returns 0 when idx not found for the given devtype
const char* fdev_pinw_idx2str(int devtype, pinw_idx_t idx);

int fdev_logic_set_lut(struct fpga_model* model, int y, int x, int type_idx,
	int which_lut, const char* lut_str, int lut_len);
int fdev_set_required_pins(struct fpga_model* model, int y, int x, int type,
	int type_idx);
void fdev_print_required_pins(struct fpga_model* model, int y, int x,
	int type, int type_idx);
void fdev_delete(struct fpga_model* model, int y, int x, int type,
	int type_idx);

// Returns the connpt index or NO_CONN if the name was not
// found. connpt_dests_o and num_dests are optional and may
// return the offset into the connpt's destination array
// and number of elements there.
int fpga_connpt_find(struct fpga_model* model, int y, int x,
	str16_t name_i, int* connpt_dests_o, int* num_dests);

void fpga_conn_dest(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x, str16_t* str_i);

//
// switches
//

typedef int swidx_t; // swidx_t is an index into the uint32_t switches array
// SW_SET_SIZE should be enough for both the largest number
// of switches that can go from or to one specific connection
// point (32), as well as the largest depth of a path inside
// a switchbox (ca. 20).
#define SW_SET_SIZE 64

struct sw_set
{
	swidx_t sw[SW_SET_SIZE];
	int len;
};

// returns a switch index, or -1 (NO_SWITCH) if no switch was found
swidx_t fpga_switch_first(struct fpga_model* model, int y, int x,
	str16_t name_i, int from_to);
swidx_t fpga_switch_next(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to);
swidx_t fpga_switch_backtofirst(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to);

int fpga_swset_fromto(struct fpga_model* model, int y, int x,
	str16_t start_switch, int from_to, struct sw_set* set);
// returns -1 if not found, otherwise index into the set
int fpga_swset_contains(struct fpga_model* model, int y, int x,
	const struct sw_set* set, int from_to, str16_t connpt);
void fpga_swset_remove_connpt(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to, str16_t connpt);
// removes all switches from set whose !from_to is equal to the
// from_to in parents
void fpga_swset_remove_loop(struct fpga_model* model, int y, int x,
	struct sw_set* set, const struct sw_set* parents, int from_to);
void fpga_swset_remove_sw(struct fpga_model* model, int y, int x,
	struct sw_set* set, swidx_t sw);
int fpga_swset_level_down(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to);
void fpga_swset_print(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to);

// When calling, same_len must contain the size of the
// same_sw array. Upon return same_len returns how many
// switches were found and writen to same_sw.
int fpga_switch_same_fromto(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to, swidx_t* same_sw, int *same_len);
swidx_t fpga_switch_lookup(struct fpga_model* model, int y, int x,
	str16_t from_str_i, str16_t to_str_i);

const char* fpga_switch_str(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to);
str16_t fpga_switch_str_i(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to);
const char* fpga_switch_print(struct fpga_model* model, int y, int x,
	swidx_t swidx);
int fpga_switch_is_bidir(struct fpga_model* model, int y, int x,
	swidx_t swidx);
int fpga_switch_is_used(struct fpga_model* model, int y, int x,
	swidx_t swidx);
void fpga_switch_enable(struct fpga_model* model, int y, int x,
	swidx_t swidx);
int fpga_switch_set_enable(struct fpga_model* model, int y, int x,
	struct sw_set* set);
void fpga_switch_disable(struct fpga_model* model, int y, int x,
	swidx_t swidx);

const char* fmt_swset(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to);

// MAX_SWITCHBOX_SIZE can be used to allocate the block
// list and should be larger than the largest known number
// of switches in a tile, currently 3459 in a slx9 routing tile.
#define MAX_SWITCHBOX_SIZE 4000

struct sw_chain
{
	// start and recurring values:
	struct fpga_model* model;
	int y;
	int x;
	int from_to;
	int max_depth;
	//
	// block_list works as if all switches from or to the ones
	// on the block list are blocked, that is the recursion will
	// never step into a part of the tree that goes through a
	// blocked from or to point.
	// Every call to fpga_switch_chain(), even the last one that
	// returns NO_SWITCH, may add switches to the block list.
	//
	swidx_t* block_list;
	int block_list_len;

	// return value: set is carried forward through the
	// enumeration and must only be read from.
	struct sw_set set;

	// internal:
	int first_round;	
	swidx_t* internal_block_list;
};

int construct_sw_chain(struct sw_chain* chain, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth,
	swidx_t* block_list, int block_list_len);
void destruct_sw_chain(struct sw_chain* chain);

// Returns 0 if another switchset is returned in chain, or
// NO_SWITCH (-1) if there is no other switchset.
// set.len is 0 when there are no more switches in the tree
int fpga_switch_chain(struct sw_chain* chain);

struct sw_conns
{
	struct sw_chain chain;
	int connpt_dest_start;
	int num_dests;
	int dest_i;
	int dest_y;
	int dest_x;
	str16_t dest_str_i;
};

int construct_sw_conns(struct sw_conns* conns, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth);
void destruct_sw_conns(struct sw_conns* conns);

// Returns 0 if another connection is returned in conns, or
// NO_CONN (-1) if there is no other connection.
int fpga_switch_conns(struct sw_conns* conns);

void printf_swchain(struct fpga_model* model, int y, int x,
	str16_t sw, int from_to, int max_depth, swidx_t* block_list,
	int* block_list_len);
void printf_swconns(struct fpga_model* model, int y, int x,
	str16_t sw, int from_to, int max_depth);

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
	int from_to;
	int max_switch_depth; // only if SWTO_YX_MAX_SWITCH_DEPTH is set
	str16_t target_connpt; // only if SWTO_YX_TARGET_CONNPT is set

	// output:
	struct sw_set set;
	int dest_y;
	int dest_x;
	str16_t dest_connpt;
};

int fpga_switch_to_yx(struct switch_to_yx* p);
void printf_switch_to_result(struct switch_to_yx* p);

//
// nets
//

// The last m1 soc has about 20k nets with about 470k
// connection points. The largest net has about 110
// connection points. For now we work with a simple
// fixed-size array, we can later make this more dynamic
// depending on which load on the memory manager is better.
#define MAX_NET_LEN	128

#define NET_IDX_IS_PINW	0x8000
#define NET_IDX_MASK	0x7FFF

struct net_el
{
	uint16_t y;
	uint16_t x;
	// idx is either an index into tile->switches[]
	// if bit15 (NET_IDX_IS_PINW) is off, or an index
	// into dev->pinw[] if bit15 is on.
	uint16_t idx;
	uint16_t dev_idx; // only used if idx&NET_IDX_IS_PINW
};

struct fpga_net
{
	int len;
	struct net_el el[MAX_NET_LEN];
};

typedef int net_idx_t; // net indices are 1-based
#define NO_NET 0

int fpga_net_new(struct fpga_model* model, net_idx_t* new_idx);
void fpga_net_delete(struct fpga_model* model, net_idx_t net_idx);
// start a new enumeration by calling with last==NO_NET
int fpga_net_enum(struct fpga_model* model, net_idx_t last, net_idx_t* next);
struct fpga_net* fpga_net_get(struct fpga_model* model, net_idx_t net_i);
int fpga_net_add_port(struct fpga_model* model, net_idx_t net_i,
	int y, int x, dev_idx_t dev_idx, pinw_idx_t pinw_idx);
int fpga_net_add_switch(struct fpga_model* model, net_idx_t net_i,
	int y, int x, swidx_t one_sw);
int fpga_net_add_switches(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const struct sw_set* set);
void fpga_net_free_all(struct fpga_model* model);

void fprintf_net(FILE* f, struct fpga_model* model, net_idx_t net_i);
