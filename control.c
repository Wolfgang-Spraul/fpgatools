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
	int* y, int* x, dev_type_idx_t* idx)
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
	dev_type_idx_t idx)
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

static const char* dev_str[] = FPGA_DEV_STR;

const char* fpgadev_str(enum fpgadev_type type)
{
	if (type < 0 || type >= sizeof(dev_str)/sizeof(*dev_str))
		{ HERE(); return 0; }
	return dev_str[type];
}

enum fpgadev_type fpgadev_str2type(const char* str, int len)
{
	int i;
	for (i = 0; i < sizeof(dev_str)/sizeof(*dev_str); i++) {
		if (dev_str[i]
		    && strlen(dev_str[i]) == len
		    && !str_cmp(dev_str[i], len, str, len))
			return i;
	}
	return DEV_NONE;
}

dev_idx_t fpga_dev_idx(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx)
{
	struct fpga_tile* tile;
	dev_type_idx_t type_count;
	dev_idx_t i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == type) {
			if (type_count == type_idx)
				return i;
			type_count++;
		}
	}
	return NO_DEV;
}

dev_type_idx_t fpga_dev_typeidx(struct fpga_model* model, int y, int x,
	dev_idx_t dev_idx)
{
	struct fpga_tile* tile;
	dev_type_idx_t type_count, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < dev_idx; i++) {
		if (tile->devs[i].type == tile->devs[dev_idx].type)
			type_count++;
	}
	return type_count;
}

static const char* iob_pinw_str[] = IOB_PINW_STR;
static const char* logic_pinw_str[] = LOGIC_PINW_STR;

pinw_idx_t fpgadev_pinw_str2idx(int devtype, const char* str, int len)
{
	int i;

	if (devtype == DEV_IOB) {
		for (i = 0; i < sizeof(iob_pinw_str)/sizeof(*iob_pinw_str); i++) {
			if (strlen(iob_pinw_str[i]) == len
			    && !str_cmp(iob_pinw_str[i], len, str, len))
				return i;
		}
		HERE();
		return PINW_NO_IDX;
	}
	if (devtype == DEV_LOGIC) {
		for (i = 0; i < sizeof(logic_pinw_str)/sizeof(*logic_pinw_str); i++) {
			if (strlen(logic_pinw_str[i]) == len
			    && !str_cmp(logic_pinw_str[i], len, str, len))
				return i;
		}
		HERE();
		return PINW_NO_IDX;
	}
	HERE();
	return PINW_NO_IDX;
}

const char* fpgadev_pinw_idx2str(int devtype, pinw_idx_t idx)
{
	if (devtype == DEV_IOB) {
		if (idx < 0 || idx >= sizeof(iob_pinw_str)/sizeof(*iob_pinw_str)) {
			HERE();
			return 0;
		}
		return iob_pinw_str[idx];
	}
	if (devtype == DEV_LOGIC) {
		if (idx < 0 || idx >= sizeof(logic_pinw_str)/sizeof(*logic_pinw_str)) {
			HERE();
			return 0;
		}
		return logic_pinw_str[idx];
	}
	HERE();
	return 0;
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

int fpga_connpt_find(struct fpga_model* model, int y, int x,
	str16_t name_i, int* connpt_dests_o, int* num_dests)
{
	struct fpga_tile* tile;
	int i;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == name_i)
			break;
	}
	if (i >= tile->num_conn_point_names)
		{ HERE(); goto fail; }
	if (num_dests) {
		*num_dests = (i < tile->num_conn_point_names-1)
			? tile->conn_point_names[(i+1)*2]
				- tile->conn_point_names[i*2]
			: tile->num_conn_point_dests
				- tile->conn_point_names[i*2];
	}
	if (connpt_dests_o)
		*connpt_dests_o = tile->conn_point_names[i*2];
	return i;
fail:
	return NO_CONN;
}

void fpga_conn_dest(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x, str16_t* str_i)
{
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	if (connpt_dest_idx < 0
	    || connpt_dest_idx >= tile->num_conn_point_dests) {
		HERE();
		return;
	}
	*dest_x = tile->conn_point_dests[connpt_dest_idx*3];
	*dest_y = tile->conn_point_dests[connpt_dest_idx*3+1];
	*str_i = tile->conn_point_dests[connpt_dest_idx*3+2];
}

swidx_t fpga_switch_first(struct fpga_model* model, int y, int x,
	str16_t name_i, int from_to)
{
	struct fpga_tile* tile;
	int i, connpt_o;

	// Finds the first switch either from or to the name given.
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		connpt_o = SW_I(tile->switches[i], from_to);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
}

static swidx_t fpga_switch_search(struct fpga_model* model, int y, int x,
	swidx_t last, swidx_t search_beg, int from_to)
{
	struct fpga_tile* tile;
	int connpt_o, name_i, i;
	
	tile = YX_TILE(model, y, x);
	connpt_o = SW_I(tile->switches[last], from_to);
	name_i = tile->conn_point_names[connpt_o*2+1];

	for (i = search_beg; i < tile->num_switches; i++) {
		connpt_o = SW_I(tile->switches[i], from_to);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
}

swidx_t fpga_switch_next(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to)
{
	return fpga_switch_search(model, y, x, last, last+1, from_to);
}

swidx_t fpga_switch_backtofirst(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to)
{
	return fpga_switch_search(model, y, x, last, /*search_beg*/ 0, from_to);
}

swidx_t fpga_switch_lookup(struct fpga_model* model, int y, int x,
	str16_t from_str_i, str16_t to_str_i)
{
	int from_connpt_o, to_connpt_o, i;
	struct fpga_tile* tile;

	from_connpt_o = fpga_connpt_find(model, y, x, from_str_i,
		/*dests_o*/ 0, /*num_dests*/ 0);
	to_connpt_o = fpga_connpt_find(model, y, x, to_str_i,
		/*dests_o*/ 0, /*num_dests*/ 0);
	if (from_connpt_o == NO_CONN || to_connpt_o == NO_CONN)
		return NO_SWITCH;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (SW_FROM_I(tile->switches[i]) == from_connpt_o
		    && SW_TO_I(tile->switches[i]) == to_connpt_o)
			return i;
	}
	return NO_SWITCH;
}

#define NUM_CONNPT_BUFS	64
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
	return connpt_str(model, y, x, SW_I(sw, from_to));
}

str16_t fpga_switch_str_i(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to)
{
	struct fpga_tile* tile;
	uint32_t sw;
	int connpt_o;

	tile = YX_TILE(model, y, x);
	sw = tile->switches[swidx];
	connpt_o = SW_I(sw, from_to);
	return tile->conn_point_names[connpt_o*2+1];
}

const char* fpga_switch_print(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
 	enum { NUM_BUFS = 16, BUF_SIZE = 128 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;
	uint32_t sw;

	sw = YX_TILE(model, y, x)->switches[swidx];
	last_buf = (last_buf+1)%NUM_BUFS;

	snprintf(buf[last_buf], sizeof(*buf), "%s %s %s",
		connpt_str(model, y, x, SW_FROM_I(sw)),
		sw & SWITCH_BIDIRECTIONAL ? "<->" : "->",
		connpt_str(model, y, x, SW_TO_I(sw)));
	return buf[last_buf];
}

int fpga_switch_is_bidir(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_BIDIRECTIONAL) != 0;
}

int fpga_switch_is_used(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_USED) != 0;
}

void fpga_switch_enable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] |= SWITCH_USED;
}

int fpga_switch_set_enable(struct fpga_model* model, int y, int x,
	struct sw_set* set)
{
	int i;
	for (i = 0; i < set->len; i++)
		fpga_switch_enable(model, y, x, set->sw[i]);
	return 0;
}

void fpga_switch_disable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] &= ~SWITCH_USED;
}

#define SW_BUF_SIZE	256
#define NUM_SW_BUFS	64

static const char* fmt_swset_el(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to)
{
	static char sw_buf[NUM_SW_BUFS][SW_BUF_SIZE];
	static int last_buf = 0;
	char midstr[64];

	last_buf = (last_buf+1)%NUM_SW_BUFS;

	strcpy(midstr, fpga_switch_is_used(model, y, x, sw) ? "on:" : "");
	if (fpga_switch_is_bidir(model, y, x, sw))
		strcat(midstr, "<->");
	else {
		// a 'to-switch' is actually still a switch that physically
		// points in the other direction (unless it's a bidir switch),
		// so when displaying the 'to-switch', we make the arrow point
		// to the left side to match the physical direction.
		strcat(midstr, (from_to == SW_TO) ? "<-" : "->");
	}
	// fmt_swset_el() prints only the destination side of the switch (!from_to),
	// because it is the significant one in a chain of switches, and if the
	// caller wants the source side they can add it outside.
	snprintf(sw_buf[last_buf], sizeof(sw_buf[0]), "%s%s%s",
		(from_to == SW_FROM) ? "" : fpga_switch_str(model, y, x, sw, SW_FROM),
		midstr,
		(from_to == SW_TO) ? "" : fpga_switch_str(model, y, x, sw, SW_TO));

	return sw_buf[last_buf];
}

#define FMT_SWSET_BUF_SIZE	2048
#define FMT_SWSET_NUM_BUFS	8

const char* fmt_swset(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	static char buf[FMT_SWSET_NUM_BUFS][FMT_SWSET_BUF_SIZE];
	static int last_buf = 0;
	int i, o;

	last_buf = (last_buf+1)%FMT_SWSET_NUM_BUFS;
	o = 0;
	if (set->len) {
		if (from_to == SW_FROM) {
			strcpy(&buf[last_buf][o], fpga_switch_str(model, y, x, set->sw[0], SW_FROM));
			o += strlen(&buf[last_buf][o]);
			for (i = 0; i < set->len; i++) {
				buf[last_buf][o++] = ' ';
				strcpy(&buf[last_buf][o],
					fmt_swset_el(model, y, x, set->sw[i], SW_FROM));
				o += strlen(&buf[last_buf][o]);
			}
		} else { // SW_TO
			for (i = set->len-1; i >= 0; i--) {
				if (i < set->len-1) buf[last_buf][o++] = ' ';
				strcpy(&buf[last_buf][o],
					fmt_swset_el(model, y, x, set->sw[i], SW_TO));
				o += strlen(&buf[last_buf][o]);
			}
			buf[last_buf][o++] = ' ';
			strcpy(&buf[last_buf][o], fpga_switch_str(model, y, x, set->sw[0], SW_TO));
			o += strlen(&buf[last_buf][o]);
		}
	}
	buf[last_buf][o] = 0;
	return buf[last_buf];
}

int fpga_switch_chain(struct sw_chain* chain)
{
	swidx_t idx;
	struct fpga_tile* tile;
	int child_from_to, i;

	if (chain->start_switch != STRIDX_NO_ENTRY) {
		idx = fpga_switch_first(chain->model, chain->y, chain->x,
			chain->start_switch, chain->from_to);
		chain->start_switch = STRIDX_NO_ENTRY;
		if (idx == NO_SWITCH) {
			HERE(); // unusual and is probably some internal error
			chain->set.len = 0;
			return NO_SWITCH;
		}
		chain->set.sw[0] = idx;
		chain->set.len = 1;

		// at every level, the first round returns all members
		// at that level, then the second round tries to go
		// one level down for each member. This sorts the
		// returned switches in a nice way.
		chain->first_round = 1;
		return 0;
	}
	if (!chain->set.len) {
		HERE(); goto internal_error;
	}
	if (chain->first_round) {
		// first go through all members are present level
		idx = fpga_switch_next(chain->model, chain->y, chain->x,
			chain->set.sw[chain->set.len-1], chain->from_to);
		if (idx != NO_SWITCH) {
			chain->set.sw[chain->set.len-1] = idx;
			return 0;
		}
		// if there are no more, initiate the second round
		// looking for children
		chain->first_round = 0;
		idx = fpga_switch_backtofirst(chain->model, chain->y, chain->x,
			chain->set.sw[chain->set.len-1], chain->from_to);
		if (idx == NO_SWITCH) {
			HERE(); goto internal_error;
		}
		chain->set.sw[chain->set.len-1] = idx;
	}
	// look for children
	tile = YX_TILE(chain->model, chain->y, chain->x);
	while (1) {
		idx = fpga_switch_first(chain->model, chain->y, chain->x,
			fpga_switch_str_i(chain->model, chain->y, chain->x,
			chain->set.sw[chain->set.len-1], !chain->from_to),
			chain->from_to);
		child_from_to = SW_I(tile->switches[chain->set.sw[chain->set.len-1]],
			!chain->from_to);
		if (idx != NO_SWITCH) {
			// If we have the same from-switch already among the
			// parents, don't fall into endless recursion...
			for (i = 0; i < chain->set.len; i++) {
				if (SW_I(tile->switches[chain->set.sw[i]], chain->from_to)
					== child_from_to)
					break;
			}
			if (i >= chain->set.len) {
				if (chain->set.len >= MAX_SW_DEPTH) {
					HERE(); goto internal_error;
				}
				if (chain->max_chain_size < 1
				    || chain->set.len < chain->max_chain_size) {
					// back to first round at new level
					chain->first_round = 1;
					chain->set.sw[chain->set.len] = idx;
					chain->set.len++;
					return 0;
				}
			}
		}
		while (1) {
			chain->set.sw[chain->set.len-1] = fpga_switch_next(
				chain->model, chain->y, chain->x,
				chain->set.sw[chain->set.len-1], chain->from_to);
			if (chain->set.sw[chain->set.len-1] != NO_SWITCH)
				break;
			if (chain->set.len <= 1) {
				chain->set.len = 0;
				return NO_SWITCH;
			}
			chain->set.len--;
		}
	}
internal_error:
	chain->set.len = 0;
	return NO_SWITCH;
}

int fpga_switch_chains(struct sw_chain* chain, int max_sets,
	struct sw_set* sets, int* num_sets)
{
	*num_sets = 0;
	while (*num_sets < max_sets
	       && fpga_switch_chain(chain) != NO_CONN) {
		sets[*num_sets] = chain->set;
	}
	return 0;
}

int fpga_switch_conns(struct sw_conns* conns)
{
	str16_t end_of_chain_str;

	if (conns->start_switch != STRIDX_NO_ENTRY) {
		conns->chain.model = conns->model;
		conns->chain.y = conns->y;
		conns->chain.x = conns->x;
		conns->chain.start_switch = conns->start_switch;
		conns->chain.from_to = SW_FROM;
		conns->chain.max_chain_size = conns->max_switch_depth;

		conns->start_switch = STRIDX_NO_ENTRY;
		conns->num_dests = 0;
		conns->dest_i = 0;
	}
	else if (!conns->chain.set.len) { HERE(); goto internal_error; }

	while (conns->dest_i >= conns->num_dests) {
		fpga_switch_chain(&conns->chain);
		if (conns->chain.set.len == 0)
			return NO_CONN;
		end_of_chain_str = fpga_switch_str_i(conns->model,
			conns->y, conns->x,
			conns->chain.set.sw[conns->chain.set.len-1],
			SW_TO);
		if (end_of_chain_str == STRIDX_NO_ENTRY)
			{ HERE(); goto internal_error; }
		conns->dest_i = 0;
		fpga_connpt_find(conns->model, conns->y, conns->x,
			end_of_chain_str, &conns->connpt_dest_start,
			&conns->num_dests);
		if (conns->num_dests)
			break;
	}
	fpga_conn_dest(conns->model, conns->y, conns->x,
		conns->connpt_dest_start + conns->dest_i,
		&conns->dest_y, &conns->dest_x, &conns->dest_str_i);
	if (conns->dest_str_i == STRIDX_NO_ENTRY)
		{ HERE(); goto internal_error; }
	conns->dest_i++;
	return 0;
		
internal_error:
	conns->chain.set.len = 0;
	return NO_CONN;
}

void printf_swchain(struct fpga_model* model, int y, int x,
	str16_t sw, int max_depth, int from_to)
{
	struct sw_chain chain =
		{ .model = model, .y = y, .x = x, .start_switch = sw,
		  .from_to = from_to, .max_chain_size = max_depth};
	while (fpga_switch_chain(&chain) != NO_CONN) {
		printf("sw %s\n", fmt_swset(model, y, x,
			&chain.set, from_to));
	}
}

void printf_swconns(struct fpga_model* model, int y, int x,
	str16_t sw, int max_depth)
{
	struct sw_conns conns =
		{ .model = model, .y = y, .x = x, .start_switch = sw,
		  .max_switch_depth = max_depth };
	while (fpga_switch_conns(&conns) != NO_CONN) {
		printf("sw %s conn y%02i x%02i %s\n", fmt_swset(model, y, x,
			&conns.chain.set, SW_FROM),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&model->str, conns.dest_str_i));
	}
}

int fpga_switch_to_yx(struct switch_to_yx* p)
{
	struct sw_conns conns = {
		.model = p->model, .y = p->y, .x = p->x,
		.start_switch = p->start_switch,
		.max_switch_depth =
		  (p->flags & SWTO_YX_MAX_SWITCH_DEPTH)
			? p->max_switch_depth : MAX_SW_DEPTH };

	struct sw_set best_set;
	int best_y, best_x, best_distance, distance;
	int best_num_dests;
	str16_t best_connpt;

	best_y = -1;
	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (is_atyx(p->yx_req, p->model, conns.dest_y, conns.dest_x)) {
			if (p->flags & SWTO_YX_TARGET_CONNPT
			    && conns.dest_str_i != p->target_connpt)
				continue;
			if (best_y != -1) {
				distance = abs(conns.dest_y-p->y)
					+abs(conns.dest_x-p->x);
				if (distance > best_distance)
					continue;
				else if (conns.num_dests > best_num_dests)
					continue;
				else if (conns.chain.set.len > best_set.len)
					continue;
			}
			best_set = conns.chain.set;
			best_y = conns.dest_y;
			best_x = conns.dest_x;
			best_num_dests = conns.num_dests;
			best_connpt = conns.dest_str_i;
			best_distance = abs(conns.dest_y-p->y)
				+abs(conns.dest_x-p->x);
			if (!p->flags & SWTO_YX_CLOSEST)
				break;
		}
	}
	if (best_y == -1)
		p->set.len = 0;
	else {
		p->set = best_set;
		p->dest_y = best_y;
		p->dest_x = best_x;
		p->dest_connpt = best_connpt;
	}
	return 0;
}

#define NET_ALLOC_INCREMENT 64

static int fpga_net_useidx(struct fpga_model* model, net_idx_t new_idx)
{
	void* new_ptr;
	int new_array_size, rc;

	if (new_idx <= NO_NET) FAIL(EINVAL);
	if (new_idx > model->highest_used_net)
		model->highest_used_net = new_idx;

	if ((new_idx-1) < model->nets_array_size)
		return 0;

	new_array_size = ((new_idx-1)/NET_ALLOC_INCREMENT+1)*NET_ALLOC_INCREMENT;
	new_ptr = realloc(model->nets, new_array_size*sizeof(*model->nets));
	if (!new_ptr) FAIL(ENOMEM);
	// the memset will set the 'len' of each new net to 0
	memset(new_ptr + model->nets_array_size*sizeof(*model->nets), 0,
		(new_array_size - model->nets_array_size)*sizeof(*model->nets));

	model->nets = new_ptr;
	model->nets_array_size = new_array_size;
	return 0;
fail:
	return rc;
}

int fpga_net_new(struct fpga_model* model, net_idx_t* new_idx)
{
	int rc;

	// highest_used_net is initialized to NO_NET which becomes 1
	rc = fpga_net_useidx(model, model->highest_used_net+1);
	if (rc) return rc;
	*new_idx = model->highest_used_net;
	return 0;
}

int fpga_net_enum(struct fpga_model* model, net_idx_t last, net_idx_t* next)
{
	int i;

	// last can be NO_NET which becomes 1 = the first net index
	for (i = last+1; i <= model->highest_used_net; i++) {
		if (model->nets[i-1].len) {
			*next = i;
			return 0;
		}
	}
	*next = NO_NET;
	return 0;
}

struct fpga_net* fpga_net_get(struct fpga_model* model, net_idx_t net_i)
{
	if (net_i <= NO_NET
	    || net_i > model->highest_used_net) {
		fprintf(stderr, "%s:%i net_i %i highest_used %i\n", __FILE__,
			__LINE__, net_i, model->highest_used_net);
		return 0;
	}
	return &model->nets[net_i-1];
}

int fpga_net_add_port(struct fpga_model* model, net_idx_t net_i,
	int y, int x, dev_idx_t dev_idx, pinw_idx_t pinw_idx)
{
	struct fpga_net* net;
	int rc;

	rc = fpga_net_useidx(model, net_i);
	if (rc) FAIL(rc);
	
	net = &model->nets[net_i-1];
	if (net->len >= MAX_NET_LEN)
		FAIL(EINVAL);
	net->el[net->len].y = y;
	net->el[net->len].x = x;
	net->el[net->len].idx = pinw_idx | NET_IDX_IS_PINW;
	net->el[net->len].dev_idx = dev_idx;
	net->len++;
	return 0;
fail:
	return rc;
}

int fpga_net_add_switches(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const struct sw_set* set)
{
	struct fpga_net* net;
	int i, rc;

	rc = fpga_net_useidx(model, net_i);
	if (rc) FAIL(rc);

	net = &model->nets[net_i-1];
	if (net->len+set->len > MAX_NET_LEN)
		FAIL(EINVAL);
	for (i = 0; i < set->len; i++) {
		net->el[net->len].y = y;
		net->el[net->len].x = x;
		if (OUT_OF_U16(set->sw[i])) FAIL(EINVAL);
		if (fpga_switch_is_used(model, y, x, set->sw[i]))
			HERE();
		fpga_switch_enable(model, y, x, set->sw[i]);
		net->el[net->len].idx = set->sw[i];
		net->len++;
	}
	return 0;
fail:
	return rc;
}

void fpga_net_free_all(struct fpga_model* model)
{
	free(model->nets);
	model->nets = 0;
	model->nets_array_size = 0;
	model->highest_used_net = 0;
}

static void fprintf_inout_pin(FILE* f, struct fpga_model* model,
	net_idx_t net_i, struct net_el* el)
{
	struct fpga_tile* tile;
	pinw_idx_t pinw_i;
	dev_idx_t dev_idx;
	int in_pin;
	const char* pin_str;
	char buf[1024];

	if (!(el->idx & NET_IDX_IS_PINW))
		{ HERE(); return; }

	tile = YX_TILE(model, el->y, el->x);
	dev_idx = el->dev_idx;
	if (dev_idx < 0 || dev_idx >= tile->num_devs)
		{ HERE(); return; }
	pinw_i = el->idx & NET_IDX_MASK;
	if (pinw_i < 0 || pinw_i >= tile->devs[dev_idx].num_pinw_total)
		{ HERE(); return; }
	in_pin = pinw_i < tile->devs[dev_idx].num_pinw_in;

   	pin_str = fpgadev_pinw_idx2str(tile->devs[dev_idx].type, pinw_i);
	if (!pin_str) { HERE(); return; }

	snprintf(buf, sizeof(buf), "net %i %s y%02i x%02i %s %i pin %s\n",
		net_i, in_pin ? "in" : "out", el->y, el->x,
		fpgadev_str(tile->devs[dev_idx].type),
		fpga_dev_typeidx(model, el->y, el->x, dev_idx),
		pin_str);
	fprintf(f, buf);
}

void fprintf_net(FILE* f, struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net;
	int i;

	net = fpga_net_get(model, net_i);
	if (!net) { HERE(); return; }
	for (i = 0; i < net->len; i++) {
		if (net->el[i].idx & NET_IDX_IS_PINW) {
			fprintf_inout_pin(f, model, net_i, &net->el[i]);
			continue;
		}
		// switch
		fprintf(f, "net %i sw y%02i x%02i %s\n",
			net_i, net->el[i].y, net->el[i].x,
			fpga_switch_print(model, net->el[i].y,
				net->el[i].x, net->el[i].idx));
	}
}
