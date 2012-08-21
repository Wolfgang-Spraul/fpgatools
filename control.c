//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "control.h"
 
struct iob_site
{
	int xy;
	const char* name[4]; // left and right only use 2, top and bottom 4
};

static const struct iob_site xc6slx9_iob_top[] =
{
	{ 5, {"P144",  "P143",   "P142",   "P141"}},
	{ 7, {"P140",  "P139",   "P138",   "P137"}},
	{12, {"UNB9",  "UNB10",  "UNB11",  "UNB12"}},
	{14, {"UNB13", "UNB14",  "UNB15",  "UNB16"}},
	{19, {"UNB17", "UNB18",  "UNB19",  "UNB20"}},
	{21, {"P134",  "P133",   "P132",   "P131"}},
	{25, {"P127",  "P126",   "P124",   "P123"}},
	{29, {"UNB29", "UNB30",  "UNB31",  "UNB32"}},
	{31, {"UNB33", "UNB34",  "P121",   "P120"}},
	{36, {"P119",  "P118",   "P117",   "P116"}},
	{38, {"P115",  "P114",   "P112",   "P111"}},
};

static const struct iob_site xc6slx9_iob_bottom[] =
{
	{ 5, {"P39",    "P38",    "P40",    "P41"}},
	{ 7, {"UNB139", "UNB140", "P43",    "P44"}},
	{12, {"P46",    "P45",    "P47",    "P48"}},
	{14, {"UNB131", "UNB132", "UNB130", "UNB129"}},
	{19, {"UNB127", "UNB128", "UNB126", "UNB125"}},
	{21, {"UNB123", "UNB124", "P50",    "P51"}},
	{25, {"P56",    "P55",    "UNB118", "UNB117"}},
	{29, {"UNB115", "UNB116", "UNB114", "UNB113"}},
	{31, {"P58",    "P57",    "P59",    "P60"}},
	{36, {"P62",    "P61",    "P64",    "P65"}},
	{38, {"P67",    "P66",    "P69",    "P70"}},
};

static const struct iob_site xc6slx9_iob_left[] =
{
	{  3, {"P1",     "P2"}},
	{  5, {"UNB198", "UNB197"}},
	{  7, {"UNB196", "UNB195"}},
	{  9, {"UNB194", "UNB193"}},
	{ 11, {"P5",     "P6"}},
	{ 12, {"P7",     "P8"}},
	{ 13, {"P9",     "P10"}},
	{ 14, {"P11",    "P12"}},
	{ 28, {"UNB184", "UNB183"}},
	{ 29, {"UNB182", "UNB181"}},
	{ 30, {"UNB180", "UNB179"}},
	{ 31, {"UNB178", "UNB177"}},
	{ 32, {"P14",    "P15"}},
	{ 33, {"P16",    "P17"}},
	{ 37, {"P21",    "P22"}},
	{ 38, {"P23",    "P24"}},
	{ 39, {"UNB168", "UNB167"}},
	{ 42, {"UNB166", "UNB165"}},
	{ 46, {"UNB164", "UNB163"}},
	{ 49, {"P26",    "P27"}},
	{ 52, {"P29",    "P30"}},
	{ 55, {"UNB158", "UNB157"}},
	{ 58, {"UNB156", "UNB155"}},
	{ 61, {"UNB154", "UNB153"}},
	{ 65, {"UNB152", "UNB151"}},
	{ 66, {"UNB150", "UNB149"}},
	{ 67, {"P32",    "P33"}},
	{ 68, {"P34",    "P35"}},
};

static const struct iob_site xc6slx9_iob_right[] =
{
	{  4, {"P105",   "P104"}},
	{  5, {"UNB47",  "UNB48"}},
	{  7, {"UNB49",  "UNB50"}},
	{  9, {"UNB51",  "UNB52"}},
	{ 11, {"P102",   "P101"}},
	{ 12, {"P100",   "P99"}},
	{ 13, {"P98",    "P97"}},
	{ 14, {"UNB59",  "UNB60"}},
	{ 28, {"UNB61",  "UNB62"}},
	{ 29, {"UNB63",  "UNB64"}},
	{ 30, {"UNB65",  "UNB66"}},
	{ 31, {"UNB67",  "UNB68"}},
	{ 32, {"P95",    "P94"}},
	{ 33, {"P93",    "P92"}},
	{ 37, {"P88",    "P87"}},
	{ 38, {"P85",    "P84"}},
	{ 39, {"UNB77",  "UNB78"}},
	{ 42, {"P83",    "P82"}},
	{ 46, {"P81",    "P80"}},
	{ 49, {"P79",    "P78"}},
	{ 52, {"UNB85",  "UNB86"}},
	{ 55, {"UNB87",  "UNB88"}},
	{ 58, {"UNB89",  "UNB90"}},
	{ 61, {"UNB91",  "UNB92"}},
	{ 65, {"UNB93",  "UNB94"}},
	{ 66, {"UNB95",  "UNB96"}},
	{ 67, {"UNB97",  "UNB98"}},
	{ 68, {"P75",    "P74"}},
};

int fpga_find_iob(struct fpga_model* model, const char* sitename,
	int* y, int* x, int* idx)
{
	int i, j;

	for (i = 0; i < sizeof(xc6slx9_iob_top)/sizeof(xc6slx9_iob_top[0]); i++) {
		for (j = 0; j < 4; j++) {
			if (!strcmp(xc6slx9_iob_top[i].name[j], sitename)) {
				*y = TOP_OUTER_ROW;
				*x = xc6slx9_iob_top[i].xy;
				*idx = j;
				return 0;
			}
		}
	}
	for (i = 0; i < sizeof(xc6slx9_iob_bottom)/sizeof(xc6slx9_iob_bottom[0]); i++) {
		for (j = 0; j < 4; j++) {
			if (!strcmp(xc6slx9_iob_bottom[i].name[j], sitename)) {
				*y = model->y_height-BOT_OUTER_ROW;
				*x = xc6slx9_iob_bottom[i].xy;
				*idx = j;
				return 0;
			}
		}
	}
	for (i = 0; i < sizeof(xc6slx9_iob_left)/sizeof(xc6slx9_iob_left[0]); i++) {
		for (j = 0; j < 2; j++) {
			if (!strcmp(xc6slx9_iob_left[i].name[j], sitename)) {
				*y = xc6slx9_iob_left[i].xy;
				*x = LEFT_OUTER_COL;
				*idx = j;
				return 0;
			}
		}
	}
	for (i = 0; i < sizeof(xc6slx9_iob_right)/sizeof(xc6slx9_iob_right[0]); i++) {
		for (j = 0; j < 2; j++) {
			if (!strcmp(xc6slx9_iob_right[i].name[j], sitename)) {
				*y = xc6slx9_iob_right[i].xy;
				*x = model->x_width-RIGHT_OUTER_O;
				*idx = j;
				return 0;
			}
		}
	}
	return -1;
}

const char* fpga_iob_sitename(struct fpga_model* model, int y, int x,
	int idx)
{
	int i;

	if (y == TOP_OUTER_ROW) {
		for (i = 0; i < sizeof(xc6slx9_iob_top)/sizeof(xc6slx9_iob_top[0]); i++) {
			if (xc6slx9_iob_right[i].xy == x) {
				if (idx < 0 || idx > 3) return 0;
				return xc6slx9_iob_top[i].name[idx];
			}
		}
		return 0;
	}
	if (y == model->y_height-BOT_OUTER_ROW) {
		for (i = 0; i < sizeof(xc6slx9_iob_bottom)/sizeof(xc6slx9_iob_bottom[0]); i++) {
			if (xc6slx9_iob_bottom[i].xy == x) {
				if (idx < 0 || idx > 3) return 0;
				return xc6slx9_iob_bottom[i].name[idx];
			}
		}
		return 0;
	}
	if (x == LEFT_OUTER_COL) {
		for (i = 0; i < sizeof(xc6slx9_iob_left)/sizeof(xc6slx9_iob_left[0]); i++) {
			if (xc6slx9_iob_left[i].xy == y) {
				if (idx < 0 || idx > 1) return 0;
				return xc6slx9_iob_left[i].name[idx];
			}
		}
		return 0;
	}
	if (x == model->x_width-RIGHT_OUTER_O) {
		for (i = 0; i < sizeof(xc6slx9_iob_right)/sizeof(xc6slx9_iob_right[0]); i++) {
			if (xc6slx9_iob_right[i].xy == y) {
				if (idx < 0 || idx > 1) return 0;
				return xc6slx9_iob_right[i].name[idx];
			}
		}
		return 0;
	}
	return 0;
}

struct fpga_device* fpga_dev(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, int type_idx)
{
	struct fpga_tile* tile;
	int type_count, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == type) {
			if (type_count == type_idx)
				return &tile->devs[i];
			type_count++;
		}
	}
	return 0;
}

int fpga_dev_typecount(struct fpga_model* model, int y, int x,
	enum fpgadev_type type, int dev_idx)
{
	struct fpga_tile* tile;
	int type_count, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < dev_idx; i++) {
		if (tile->devs[i].type == type)
			type_count++;
	}
	return type_count;
}

#define MAX_LUT_LEN	512

int fpga_set_lut(struct fpga_model* model, struct fpga_device* dev,
	int which_lut, const char* lut_str, int lut_len)
{
	char** ptr;

	if (dev->type != DEV_LOGIC)
		return -1;
	switch (which_lut) {
		case A6_LUT: ptr = &dev->logic.A6_lut; break;
		case B6_LUT: ptr = &dev->logic.B6_lut; break;
		case C6_LUT: ptr = &dev->logic.C6_lut; break;
		case D6_LUT: ptr = &dev->logic.D6_lut; break;
		default: return -1;
	}
	if (!(*ptr)) {
		*ptr = malloc(MAX_LUT_LEN);
		if (!(*ptr)) {
			OUT_OF_MEM();
			return -1;
		}
	}
	if (lut_len == ZTERM) lut_len = strlen(lut_str);
	memcpy(*ptr, lut_str, lut_len);
	(*ptr)[lut_len] = 0;
	return 0;
}

int fpga_conn_dest(struct fpga_model* model, int y, int x,
	const char* name, int dest_idx)
{
	struct fpga_tile* tile;
	int i, rc, connpt_i, num_dests, conn_point_dests_o;

	rc = strarray_find(&model->str, name, &connpt_i);
	if (rc) FAIL(rc);
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == connpt_i)
			break;
	}
	if (i >= tile->num_conn_point_names)
		FAIL(EINVAL);

	conn_point_dests_o = tile->conn_point_names[i*2];
	if (i < tile->num_conn_point_names-1)
		num_dests = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
	else
		num_dests = tile->num_conn_point_dests - conn_point_dests_o;
	if (dest_idx >= num_dests)
		return NO_CONN;
	return conn_point_dests_o + dest_idx;
fail:
	return NO_CONN;
}

#define NUM_CONN_DEST_BUFS	16
#define CONN_DEST_BUF_SIZE	128

const char* fpga_conn_to(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x)
{
	static char conn_dest_buf[NUM_CONN_DEST_BUFS][CONN_DEST_BUF_SIZE];
	static int last_buf = 0;

	struct fpga_tile* tile;
	const char* hash_str;

	tile = YX_TILE(model, y, x);
	if (connpt_dest_idx < 0
	    || connpt_dest_idx >= tile->num_conn_point_dests) {
		HERE();
		return 0;
	}
	*dest_x = tile->conn_point_dests[connpt_dest_idx*3];
	*dest_y = tile->conn_point_dests[connpt_dest_idx*3+1];

	hash_str = strarray_lookup(&model->str,
		tile->conn_point_dests[connpt_dest_idx*3+2]);
	if (!hash_str || (strlen(hash_str) >= CONN_DEST_BUF_SIZE)) {
		HERE();
		return 0;
	}
	last_buf = (last_buf+1)%NUM_CONN_DEST_BUFS;
	strcpy(conn_dest_buf[last_buf], hash_str);

	return conn_dest_buf[last_buf];
}

swidx_t fpga_switch_first(struct fpga_model* model, int y, int x,
	const char* name, int from_to)
{
	struct fpga_tile* tile;
	int rc, i, connpt_o, name_i;

	rc = strarray_find(&model->str, name, &name_i);
	if (rc) FAIL(rc);

	// Finds the first switch either from or to the name given.
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		connpt_o = (from_to == SW_FROM)
			? SWITCH_FROM(tile->switches[i])
			: SWITCH_TO(tile->switches[i]);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
fail:
	return NO_SWITCH;
}

swidx_t fpga_switch_next(struct fpga_model* model, int y, int x, swidx_t last, int from_to)
{
	struct fpga_tile* tile;
	int connpt_o, name_i, i;
	
	tile = YX_TILE(model, y, x);
	connpt_o = (from_to == SW_FROM)
		? SWITCH_FROM(tile->switches[last])
		: SWITCH_TO(tile->switches[last]);
	name_i = tile->conn_point_names[connpt_o*2+1];

	for (i = last+1; i < tile->num_switches; i++) {
		connpt_o = (from_to == SW_FROM)
			? SWITCH_FROM(tile->switches[i])
			: SWITCH_TO(tile->switches[i]);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
}

#define MAX_SW_PARENTS 32

swidx_t fpga_switch_tree(struct fpga_model* model, int y, int x,
	const char* name, int from_to, swidx_t** parents, int* num_parents)
{
	// the static 'parents' always contain the current
	// (last) member as their last element, which is
	// not reported back as a parent.
	static swidx_t s_parents[MAX_SW_PARENTS];
	static int s_num_parents = 0;

	swidx_t idx;

	*parents = s_parents;
	if (name != SW_TREE_NEXT) {
		idx = fpga_switch_first(model, y, x, name, from_to);
		if (idx == NO_SWITCH) return NO_SWITCH;
		s_parents[0] = idx;
		s_num_parents = 1;
		*num_parents = 0;
		return idx;
	}
	if (!s_num_parents) {
		HERE();
		return NO_SWITCH;
	}
	// first check whether there are children
	idx = fpga_switch_first(model, y, x,
		fpga_switch_str(model, y, x, s_parents[s_num_parents-1], !from_to),
		from_to);
	if (idx != NO_SWITCH) {
		if (s_num_parents >= MAX_SW_PARENTS) {
			HERE();
			return NO_SWITCH;
		}
		s_parents[s_num_parents] = idx;
		s_num_parents++;
		*num_parents = s_num_parents - 1;
		return idx;
	}
	do {
		// then check whether there are more members at the same level
		idx = fpga_switch_next(model, y, x, s_parents[s_num_parents-1],
			from_to);
		if (idx != NO_SWITCH) {
			s_parents[s_num_parents-1] = idx;
			*num_parents = s_num_parents - 1;
			return idx;
		}
		// and finally go one level up
	} while (s_num_parents--); // post-decrement
	return NO_SWITCH;
}

#define NUM_CONNPT_BUFS	16
#define CONNPT_BUF_SIZE	128

static const char* connpt_str(struct fpga_model* model, int y, int x, int connpt_o)
{
	// We have a little local ringbuffer to make passing
	// around pointers with unknown lifetime and possible
	// overlap with writing functions more stable.
	static char switch_get_buf[NUM_CONNPT_BUFS][CONNPT_BUF_SIZE];
	static int last_buf = 0;

	const char* hash_str;
	int str_i;

	str_i = YX_TILE(model, y, x)->conn_point_names[connpt_o*2+1];
	hash_str = strarray_lookup(&model->str, str_i);
	if (!hash_str || (strlen(hash_str) >= CONNPT_BUF_SIZE)) {
		HERE();
		return 0;
	}
	last_buf = (last_buf+1)%NUM_CONNPT_BUFS;
	return strcpy(switch_get_buf[last_buf], hash_str);
}

const char* fpga_switch_str(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to)
{
	uint32_t sw = YX_TILE(model, y, x)->switches[swidx];
	return connpt_str(model, y, x,
		(from_to == SW_FROM) ? SWITCH_FROM(sw) : SWITCH_TO(sw));
}

int fpga_switch_is_bidir(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_BIDIRECTIONAL) != 0;
}

int fpga_switch_is_enabled(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_ON) != 0;
}

void fpga_switch_enable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] |= SWITCH_ON;
}

void fpga_switch_disable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] &= ~SWITCH_ON;
}
