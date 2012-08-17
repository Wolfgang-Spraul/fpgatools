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
	{ 5, {"P144",   "P143",   "P141",  "P142"}},
	{ 7, {"P140",   "P139",   "P137",  "P138"}},
	{12, {"UNB9",   "UNB10",  "UNB12", "UNB11"}},
	{14, {"UNB13",  "UNB14",  "UNB16", "UNB15"}},
	{19, {"UNB17",  "UNB18",  "UNB20", "UNB19"}},
	{21, {"P134",   "P133",   "P131",  "P132"}},
	{25, {"P127",   "P126",   "P123",  "P124"}},
	{29, {"UNB29",  "UNB30",  "UNB32", "UNB31"}},
	{31, {"UNB33",  "UNB34",  "P120",  "P121"}},
	{36, {"P119",   "P118",   "P116",  "P117"}},
	{38, {"P115",   "P114",   "P111",  "P112"}},
};

static const struct iob_site xc6slx9_iob_bottom[] =
{
	{ 5, {"P38",    "P39",    "P41",    "P40"}},
	{ 7, {"UNB140", "UNB139", "P44",    "P43"}},
	{12, {"P45",    "P46",    "P48",    "P47"}},
	{14, {"UNB132", "UNB131", "UNB129", "UNB130"}},
	{19, {"UNB128", "UNB127", "UNB125", "UNB126"}},
	{21, {"UNB124", "UNB123", "P51",    "P50"}},
	{25, {"P55",    "P56",    "UNB117", "UNB118"}},
	{29, {"UNB116", "UNB115", "UNB113", "UNB114"}},
	{31, {"P57",    "P58",    "P60",    "P59"}},
	{36, {"P61",    "P62",    "P65",    "P64"}},
	{38, {"P66",    "P67",    "P70",    "P69"}},
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
