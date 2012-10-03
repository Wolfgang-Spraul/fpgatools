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

const char* fpga_enum_iob(struct fpga_model* model, int enum_idx,
	int* y, int* x, dev_type_idx_t* type_idx)
{
	if (enum_idx < 0) { HERE(); return 0; }

	if (enum_idx < sizeof(xc6slx9_iob_top)/sizeof(xc6slx9_iob_top[0])*4) {
		*y = TOP_OUTER_ROW;
		*x = xc6slx9_iob_top[enum_idx/4].xy;
		*type_idx = enum_idx%4;
		return xc6slx9_iob_top[enum_idx/4].name[enum_idx%4];
	}
	enum_idx -= sizeof(xc6slx9_iob_top)/sizeof(xc6slx9_iob_top[0])*4;
	if (enum_idx < sizeof(xc6slx9_iob_bottom)/sizeof(xc6slx9_iob_bottom[0])*4) {
		*y = model->y_height - BOT_OUTER_ROW;
		*x = xc6slx9_iob_bottom[enum_idx/4].xy;
		*type_idx = enum_idx%4;
		return xc6slx9_iob_bottom[enum_idx/4].name[enum_idx%4];
	}
	enum_idx -= sizeof(xc6slx9_iob_bottom)/sizeof(xc6slx9_iob_bottom[0])*4;
	if (enum_idx < sizeof(xc6slx9_iob_left)/sizeof(xc6slx9_iob_left[0])*2) {
		*y = xc6slx9_iob_left[enum_idx/2].xy;
		*x = LEFT_OUTER_COL;
		*type_idx = enum_idx%2;
		return xc6slx9_iob_left[enum_idx/2].name[enum_idx%2];
	}
	enum_idx -= sizeof(xc6slx9_iob_left)/sizeof(xc6slx9_iob_left[0])*2;
	if (enum_idx < sizeof(xc6slx9_iob_right)/sizeof(xc6slx9_iob_right[0])*2) {
		*y = xc6slx9_iob_right[enum_idx/2].xy;
		*x = model->x_width-RIGHT_OUTER_O;
		*type_idx = enum_idx%2;
		return xc6slx9_iob_right[enum_idx/2].name[enum_idx%2];
	}
	return 0;
}

int fpga_find_iob(struct fpga_model* model, const char* sitename,
	int* y, int* x, dev_type_idx_t* idx)
{
	const char* name;
	int i;

	for (i = 0; (name = fpga_enum_iob(model, i, y, x, idx)); i++) {
		if (!strcmp(name, sitename))
			return 0;
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

const char* fdev_type2str(enum fpgadev_type type)
{
	if (type < 0 || type >= sizeof(dev_str)/sizeof(*dev_str))
		{ HERE(); return 0; }
	return dev_str[type];
}

enum fpgadev_type fdev_str2type(const char* str, int len)
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

struct fpga_device* fdev_p(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx)
{
	dev_idx_t dev_idx = fpga_dev_idx(model, y, x, type, type_idx);
	if (dev_idx == NO_DEV) {
		fprintf(stderr, "#E %s:%i fdev_p() y%02i x%02i type %i/%i not"
			" found\n", __FILE__, __LINE__, y, x, type, type_idx);
		return 0;
	}
	return FPGA_DEV(model, y, x, dev_idx);
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

dev_type_idx_t fdev_typeidx(struct fpga_model* model, int y, int x,
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

pinw_idx_t fdev_pinw_str2idx(int devtype, const char* str, int len)
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

const char* fdev_pinw_idx2str(int devtype, pinw_idx_t idx)
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

const char* fdev_logic_pinstr(pinw_idx_t idx, int ld1_type)
{
 	enum { NUM_BUFS = 16, BUF_SIZE = 16 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;

	last_buf = (last_buf+1)%NUM_BUFS;
	if (ld1_type == LOGIC_M)
		snprintf(buf[last_buf], sizeof(*buf), "%s%s",
			(idx & LD1) ? "M_" : "X_",
			logic_pinw_str[idx&(~LD1)]);
	else if (ld1_type == LOGIC_L)
		snprintf(buf[last_buf], sizeof(*buf), "%s%s",
			(idx & LD1) ? "L_" : "XX_",
			logic_pinw_str[idx&(~LD1)]);
	else {
		HERE();
		buf[last_buf][0] = 0;
	}
	return buf[last_buf];
}

str16_t fdev_logic_pinstr_i(struct fpga_model* model, pinw_idx_t idx, int ld1_type)
{
	int str_i;

	str_i = strarray_find(&model->str, fdev_logic_pinstr(idx, ld1_type));
	if (OUT_OF_U16(str_i))
		{ HERE(); return STRIDX_NO_ENTRY; }
	return str_i;
}

static int reset_required_pins(struct fpga_device* dev)
{
	int rc;

	if (!dev->pinw_req_for_cfg) {
		dev->pinw_req_for_cfg = malloc(dev->num_pinw_total
			* sizeof(*dev->pinw_req_for_cfg));
		if (!dev->pinw_req_for_cfg) FAIL(ENOMEM);
	}
	dev->pinw_req_total = 0;
	dev->pinw_req_in = 0;
	return 0;
fail:
	return rc;
}

void fdev_print_required_pins(struct fpga_model* model, int y, int x,
	int type, int type_idx)
{
	struct fpga_device* dev;
	int i;

	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) { HERE(); return; }

	// We don't want to reset or write the required pins in this
	// function because it is mainly used for debugging purposes
	// and the caller should not suddenly be working with old
	// required pins when the print() function is not called.

	printf("y%02i x%02i %s %i inpin", y, x, fdev_type2str(type), type_idx);
	if (!dev->pinw_req_in)
		printf(" -\n");
	else {
		for (i = 0; i < dev->pinw_req_in; i++)
			printf(" %s", fdev_pinw_idx2str(type, dev->pinw_req_for_cfg[i]));
		printf("\n");
	}

	printf("y%02i x%02i %s %i outpin", y, x, fdev_type2str(type), type_idx);
	if (dev->pinw_req_total <= dev->pinw_req_in)
		printf(" -\n");
	else {
		for (i = dev->pinw_req_in; i < dev->pinw_req_total; i++)
			printf(" %s", fdev_pinw_idx2str(type, dev->pinw_req_for_cfg[i]));
		printf("\n");
	}
}

static void add_req_inpin(struct fpga_device* dev, pinw_idx_t pinw_i)
{
	int i;

	// check for duplicate
	for (i = 0; i < dev->pinw_req_in; i++) {
		if (dev->pinw_req_for_cfg[i] == pinw_i)
			return;
	}
	if (dev->pinw_req_total > dev->pinw_req_in) {
		memmove(&dev->pinw_req_for_cfg[dev->pinw_req_in+1],
			&dev->pinw_req_for_cfg[dev->pinw_req_in],
			(dev->pinw_req_total-dev->pinw_req_in)
			*sizeof(*dev->pinw_req_for_cfg));
	}
	dev->pinw_req_for_cfg[dev->pinw_req_in] = pinw_i;
	dev->pinw_req_in++;
	dev->pinw_req_total++;
}

static void add_req_outpin(struct fpga_device* dev, pinw_idx_t pinw_i)
{
	int i;

	// check for duplicate
	for (i = dev->pinw_req_in; i < dev->pinw_req_total; i++) {
		if (dev->pinw_req_for_cfg[i] == pinw_i)
			return;
	}
	dev->pinw_req_for_cfg[dev->pinw_req_total] = pinw_i;
	dev->pinw_req_total++;
}

//
// logic device
//

int fdev_logic_a2d_out_used(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int used)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].out_used = (used != 0);
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_lut(struct fpga_model* model, int y, int x, int type_idx,
	int lut_a2d, int lut_5or6, const char* lut_str, int lut_len)
{
	struct fpga_device* dev;
	char** lut_ptr;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	lut_ptr = (lut_5or6 == 5)
		? &dev->u.logic.a2d[lut_a2d].lut5
		: &dev->u.logic.a2d[lut_a2d].lut6;
	if (*lut_ptr == 0) {
		*lut_ptr = malloc(MAX_LUT_LEN);
		if (!(*lut_ptr)) FAIL(ENOMEM);
	}
	if (lut_len == ZTERM) lut_len = strlen(lut_str);
	memcpy(*lut_ptr, lut_str, lut_len);
	(*lut_ptr)[lut_len] = 0;

	// todo: the logic by which we auto-enable the direct
	//       output could have more cases, the O6 signal
	//	 could go into the carry chain/XOR/CY, F7/F8, others?
	//	 O5 could go into carry chain/XOR/CY, others?
	//       We need to find out over time what makes sense for
	//	 the caller.
	if (lut_5or6 == 6
	    && dev->u.logic.a2d[lut_a2d].ff_mux != MUX_O6
	    && dev->u.logic.a2d[lut_a2d].out_mux != MUX_O6)
		dev->u.logic.a2d[lut_a2d].out_used = 1;
	if (lut_5or6 == 5
	    && dev->u.logic.a2d[lut_a2d].ff_mux != MUX_O5
	    && !dev->u.logic.a2d[lut_a2d].out_mux)
		dev->u.logic.a2d[lut_a2d].out_mux = MUX_O5;

	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_ff(struct fpga_model* model, int y, int x, int type_idx,
	int lut_a2d, int ff_mux, int srinit)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].ff = FF_FF;
	dev->u.logic.a2d[lut_a2d].ff_mux = ff_mux;
	dev->u.logic.a2d[lut_a2d].ff_srinit = srinit;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_out_mux(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int out_mux)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].out_mux = out_mux;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_cy0(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int cy0)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].cy0 = cy0;
// todo: when cy0 is CY0_O5 and lut5 is empty, set to "0"
// 	 (same for setting ff_mux or out_mux to MUX_O5
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_clk(struct fpga_model* model, int y, int x, int type_idx,
	int clk)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.clk_inv = clk;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_sync(struct fpga_model* model, int y, int x, int type_idx,
	int sync_attr)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.sync_attr = sync_attr;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_ce_used(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.ce_used = 1;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_sr_used(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.sr_used = 1;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_we_mux(struct fpga_model* model, int y, int x,
	int type_idx, int we_mux)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.we_mux = we_mux;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_cout_used(struct fpga_model* model, int y, int x,
	int type_idx, int used)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.cout_used = (used != 0);
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_precyinit(struct fpga_model* model, int y, int x,
	int type_idx, int precyinit)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.precyinit = precyinit;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

//
// iob device
//

int fdev_iob_input(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	strcpy(dev->u.iob.istandard, IO_LVCMOS33);
	dev->u.iob.bypass_mux = BYPASS_MUX_I;
	dev->u.iob.I_mux = IMUX_I;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_iob_output(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	strcpy(dev->u.iob.ostandard, IO_LVCMOS33);
	dev->u.iob.drive_strength = 8;
	dev->u.iob.O_used = 1;
	dev->u.iob.slew = SLEW_QUIETIO;
	dev->u.iob.suspend = SUSP_3STATE;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

static void scan_lut_digits(const char* s, int* digits)
{
	int i;
	
	for (i = 0; i < 6; i++)
		digits[i] = 0;
	if (!s) return;
	// Note special cases "0" and "1" for a lut that
	// always results in 0 or 1.
	for (i = 0; s[i]; i++) {
		if (s[i] != 'A' && s[i] != 'a')
			continue;
		if (s[i+1] >= '1' && s[i+1] <= '6')
			digits[s[++i]-'1']++;
	}
}

int fdev_set_required_pins(struct fpga_model* model, int y, int x, int type,
	int type_idx)
{
	struct fpga_device* dev;
	int digits[6];
	int i, j, rc;

	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);
	if (type == DEV_LOGIC) {
		if (dev->u.logic.clk_inv)
			add_req_inpin(dev, LI_CLK);
		if (dev->u.logic.ce_used)
			add_req_inpin(dev, LI_CE);
		if (dev->u.logic.sr_used)
			add_req_inpin(dev, LI_SR);
		if (dev->u.logic.cout_used) {
			add_req_outpin(dev, LO_COUT);
			if (!dev->u.logic.precyinit)
				add_req_inpin(dev, LI_CIN);
		}
		if (dev->u.logic.precyinit == PRECYINIT_AX)
			add_req_inpin(dev, LI_AX);
		for (i = LUT_A; i <= LUT_D; i++) {
			if (dev->u.logic.a2d[i].out_used) {
				// LO_A..LO_D are in sequence
				add_req_outpin(dev, LO_A+i);
			}
			if (dev->u.logic.a2d[i].out_mux) {
				// LO_AMUX..LO_DMUX are in sequence
				add_req_outpin(dev, LO_AMUX+i);
			}
			if (dev->u.logic.a2d[i].ff) {
				// LO_AQ..LO_DQ are in sequence
				add_req_outpin(dev, LO_AQ+i);
			}
			if (dev->u.logic.a2d[i].ff_mux == MUX_X
			    || dev->u.logic.a2d[i].cy0 == CY0_X) {
				// LI_AX..LI_DX are in sequence
				add_req_inpin(dev, LI_AX+i);
			}
			if (dev->u.logic.a2d[i].lut6) {
				scan_lut_digits(dev->u.logic.a2d[i].lut6, digits);
				for (j = 0; j < 6; j++) {
					if (!digits[j]) continue;
					add_req_inpin(dev, LI_A1+i*6+j);
				}
			}
			if (dev->u.logic.a2d[i].lut5) {
				// A6 must be high/vcc if lut5 is used
				add_req_inpin(dev, LI_A6+i*6);
				scan_lut_digits(dev->u.logic.a2d[i].lut5, digits);
				for (j = 0; j < 6; j++) {
					if (!digits[j]) continue;
					add_req_inpin(dev, LI_A1+i*6+j);
				}
			}
			if ((dev->u.logic.a2d[i].ff_mux == MUX_XOR
			     || dev->u.logic.a2d[i].out_mux == MUX_XOR)
			    && !dev->u.logic.precyinit)
				add_req_inpin(dev, LI_CIN);
		}
		return 0;
	}
	if (type == DEV_IOB) {
		if (dev->u.iob.I_mux)
			add_req_outpin(dev, IOB_OUT_I);
		if (dev->u.iob.O_used)
			add_req_inpin(dev, IOB_IN_O);
	}
	return 0;
fail:
	return rc;
}

void fdev_delete(struct fpga_model* model, int y, int x, int type, int type_idx)
{
	struct fpga_device* dev;
	int i;

	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) { HERE(); return; }
	if (!dev->instantiated) return;
	free(dev->pinw_req_for_cfg);
	dev->pinw_req_for_cfg = 0;
	dev->pinw_req_total = 0;
	dev->pinw_req_in = 0;
	if (dev->type == DEV_LOGIC) {
		for (i = LUT_A; i <= LUT_D; i++) {
			free(dev->u.logic.a2d[i].lut6);
			dev->u.logic.a2d[i].lut6 = 0;
			free(dev->u.logic.a2d[i].lut5);
			dev->u.logic.a2d[i].lut5 = 0;
		}
	}
	dev->instantiated = 0;
	memset(&dev->u, 0, sizeof(dev->u));
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

int fpga_find_conn(struct fpga_model* model, int search_y, int search_x,
	str16_t* pt, int target_y, int target_x, str16_t target_pt)
{
	struct fpga_tile* tile;
	int i, j, dests_end;

	tile = YX_TILE(model, search_y, search_x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		dests_end = (i < tile->num_conn_point_names-1)
			? tile->conn_point_names[(i+1)*2]
			: tile->num_conn_point_dests;
		for (j = tile->conn_point_names[i*2]; j < dests_end; j++) {
			if (tile->conn_point_dests[j*3] == target_x
			    && tile->conn_point_dests[j*3+1] == target_y
			    && tile->conn_point_dests[j*3+2] == target_pt) {
				*pt = tile->conn_point_names[i*2+1];
				return 0;
			}
		}
	}
	*pt = STRIDX_NO_ENTRY;
	return 0;

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
	if (last & FIRST_SW) {
		connpt_o = fpga_connpt_find(model, y, x, last & ~FIRST_SW,
			/*dests_o*/ 0, /*num_dests*/ 0);
		if (connpt_o == NO_CONN) { HERE(); return NO_SWITCH; }
	} else
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
	return fpga_switch_search(model, y, x, last,
		(last & FIRST_SW) ? 0 : last+1, from_to);
}

swidx_t fpga_switch_backtofirst(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to)
{
	return fpga_switch_search(model, y, x, last, /*search_beg*/ 0, from_to);
}

int fpga_swset_fromto(struct fpga_model* model, int y, int x,
	str16_t start_switch, int from_to, struct sw_set* set)
{
	swidx_t idx;

	set->len = 0;
	idx = fpga_switch_first(model, y, x, start_switch, from_to);
	while (idx != NO_SWITCH) {
		if (set->len >= SW_SET_SIZE)
			{ HERE(); return 0; }
		set->sw[set->len++] = idx;
		idx = fpga_switch_next(model, y, x, idx, from_to);
	}
	return 0;
}

int fpga_swset_contains(struct fpga_model* model, int y, int x,
	const struct sw_set* set, int from_to, str16_t connpt)
{
	struct fpga_tile* tile;
	int i;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < set->len; i++) {
		if (CONNPT_STR16(tile, SW_I(set->sw[i], from_to)) == connpt)
			return i;
	}
	return -1;
}

void fpga_swset_remove_connpt(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to, str16_t connpt)
{
	int i;
	while ((i = fpga_swset_contains(model, y, x, set,
			from_to, connpt)) != -1) {
		if (set->len > i+1)
			memmove(&set->sw[i], &set->sw[i+1],
				(set->len-i-1)*sizeof(set->sw[0]));
		set->len--;
	}
}

void fpga_swset_remove_loop(struct fpga_model* model, int y, int x,
	struct sw_set* set, const struct sw_set* parents, int from_to)
{
	int i;
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < parents->len; i++) {
		fpga_swset_remove_connpt(model, y, x, set, !from_to,
			CONNPT_STR16(tile, SW_I(parents->sw[i], from_to)));
	}
}

void fpga_swset_remove_sw(struct fpga_model* model, int y, int x,
	struct sw_set* set, swidx_t sw)
{
	int sw_from_connpt, sw_to_connpt, cur_from_connpt, cur_to_connpt, i;

	sw_from_connpt = SW_I(sw, SW_FROM);
	sw_to_connpt = SW_I(sw, SW_TO);

	for (i = 0; i < set->len; i++) {
		cur_from_connpt = SW_I(set->sw[i], SW_FROM);
		cur_to_connpt = SW_I(set->sw[i], SW_TO);
		if (cur_from_connpt == sw_from_connpt
		    && cur_to_connpt == sw_to_connpt) {
			if ((sw & SWITCH_BIDIRECTIONAL)
				!= (set->sw[i] & SWITCH_BIDIRECTIONAL))
				HERE();
			if (set->len > i+1)
				memmove(&set->sw[i], &set->sw[i+1],
					(set->len-i-1)*sizeof(set->sw[0]));
			set->len--;
			return;
		}
		if (cur_from_connpt == sw_to_connpt
		    && cur_to_connpt == sw_from_connpt)
			HERE();
	}
}

int fpga_swset_level_down(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	int i;

	i = 0;
	while (i < set->len) {
		set->sw[i] = fpga_switch_first(model, y, x,fpga_switch_str_i(
			model, y, x, set->sw[i], !from_to), from_to);
		if (set->sw[i] == NO_SWITCH) {
			if (set->len > i+1)
				memmove(&set->sw[i], &set->sw[i+1],
					(set->len-i-1)*sizeof(set->sw[0]));
			set->len--;
		} else
			i++;
	}
	return 0;
}

void fpga_swset_print(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	int i;
	for (i = 0; i < set->len; i++) {
		printf("swset %i %s\n", i, fpga_switch_print(model,
			y, x, set->sw[i]));
	}
}

int fpga_swset_is_used(struct fpga_model* model, int y, int x,
	swidx_t* sw, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (fpga_switch_is_used(model, y, x, sw[i]))
			return 1;
	}
	return 0;
}

int fpga_switch_same_fromto(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to, swidx_t* same_sw, int *same_len)
{
	swidx_t cur_sw;
	int max_len, rc;

	max_len = *same_len;
	*same_len = 0;
	if (max_len < 1) FAIL(EINVAL);
	cur_sw = fpga_switch_search(model, y, x, sw, /*search_beg*/ 0, from_to);
	// We should at least find sw itself, if not something is wrong...
	if (cur_sw == NO_SWITCH) FAIL(EINVAL);

	same_sw[(*same_len)++] = cur_sw;
	while ((cur_sw = fpga_switch_search(model, y, x, sw, cur_sw+1, from_to)) != NO_SWITCH) {
		same_sw[(*same_len)++] = cur_sw;
		if ((*same_len) >= max_len) FAIL(EINVAL);
	}
	return 0;
fail:
	return rc;
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
	if (fpga_switch_is_bidir(model, y, x, sw))
		strcpy(midstr, "<->");
	else
		strcpy(midstr, "->");
	// fmt_swset_el() prints only the destination side of the
	// switch (!from_to), because it is the significant one in
	// a chain of switches, and if the caller wants the source
	// side they can add it outside.
	snprintf(sw_buf[last_buf], sizeof(sw_buf[0]), "%s%s%s",
		(from_to == SW_FROM) ? ""
			: fpga_switch_str(model, y, x, sw, SW_FROM),
		midstr,
		(from_to == SW_TO) ? ""
			: fpga_switch_str(model, y, x, sw, SW_TO));

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

#undef DBG_ENUM_SWITCH

int construct_sw_chain(struct sw_chain* chain, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth,
	swidx_t* block_list, int block_list_len)
{
	int rc;

#ifdef DBG_ENUM_SWITCH
	printf("construct_sw_chain() %s (%s)\n",
		strarray_lookup(&model->str, start_switch),
		(from_to == SW_FROM) ? "SW_FROM" : "SW_TO");
#endif
	memset(chain, 0, sizeof(*chain));
	chain->model = model;
	chain->y = y;
	chain->x = x;
	chain->from_to = from_to;
	chain->max_depth = (max_depth < 0) ? SW_SET_SIZE : max_depth;
	if (block_list) {
		chain->block_list = block_list;
		chain->block_list_len = block_list_len;
		// internal_block_list is 0 from memset()
	} else {
		chain->internal_block_list = malloc(
			MAX_SWITCHBOX_SIZE * sizeof(*chain->block_list));
		if (!chain->internal_block_list)
			FAIL(ENOMEM);
		chain->block_list = chain->internal_block_list;
		chain->block_list_len = 0;
	}
	// at every level, the first round returns all members
	// at that level, then the second round tries to go
	// one level down for each member. This sorts the
	// returned switches in a nice way.
	chain->first_round = 1;
	chain->set.len = 1;
	chain->set.sw[0] = FIRST_SW | start_switch;
	return 0;
fail:
	return rc;
}

void destruct_sw_chain(struct sw_chain* chain)
{
	free(chain->internal_block_list);
	memset(chain, 0, sizeof(*chain));
}

// returns NO_SWITCH if sw is not found in list, otherwise
// the offset in list where it is found.
static int switch_list_contains(struct fpga_model* model, int y, int x,
	swidx_t* list, int list_len, swidx_t sw)
{
	int sw_from_connpt, sw_to_connpt, cur_from_connpt, cur_to_connpt, i;

	sw_from_connpt = SW_I(sw, SW_FROM);
	sw_to_connpt = SW_I(sw, SW_TO);

	for (i = 0; i < list_len; i++) {
		cur_from_connpt = SW_I(list[i], SW_FROM);
		cur_to_connpt = SW_I(list[i], SW_TO);
		if (cur_from_connpt == sw_from_connpt
		    && cur_to_connpt == sw_to_connpt) {
			if ((sw & SWITCH_BIDIRECTIONAL)
				!= (list[i] & SWITCH_BIDIRECTIONAL))
				HERE();
			return i;
		}
		if (cur_from_connpt == sw_to_connpt
		    && cur_to_connpt == sw_from_connpt)
			HERE();
	}
	return NO_SWITCH;
}

int fpga_switch_chain(struct sw_chain* ch)
{
	swidx_t idx;
	struct fpga_tile* tile;
	int child_from_to, level_down, i;

	if (!ch->set.len)
		{ HERE(); goto internal_error; }
	if (ch->first_round) {
		// first go through all members at present level
		while (1) {
			idx = fpga_switch_next(ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1], ch->from_to);
			if (idx == NO_SWITCH)
				break;
			ch->set.sw[ch->set.len-1] = idx;
			if (!fpga_switch_is_used(ch->model, ch->y, ch->x, idx)
			    && switch_list_contains(ch->model, ch->y, ch->x,
				ch->block_list, ch->block_list_len, idx)
					== NO_SWITCH)
				return 0;
		}
		// if there are no more, start child round
		ch->first_round = 0;
		idx = fpga_switch_backtofirst(ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], ch->from_to);
		if (idx == NO_SWITCH) {
			HERE(); goto internal_error;
		}
#ifdef DBG_ENUM_SWITCH
		printf("back_to_first from %s to %s\n",
			fpga_switch_print(ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1]),
			fpga_switch_print(ch->model, ch->y, ch->x,
				idx));
#endif
		ch->set.sw[ch->set.len-1] = idx;
	}
	// look for children
	tile = YX_TILE(ch->model, ch->y, ch->x);
	while (1) {
		idx = fpga_switch_first(ch->model, ch->y, ch->x,
			fpga_switch_str_i(ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], !ch->from_to),
			ch->from_to);
#ifdef DBG_ENUM_SWITCH
		printf("child_check %s result %s\n", fpga_switch_str(
			ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], !ch->from_to),
			idx == NO_SWITCH ? "NO_SWITCH" : fpga_switch_print(
			ch->model, ch->y, ch->x, idx));
#endif
		if (idx != NO_SWITCH) {
			// There are four reasons why we don't want to
			// go down one level:
			// 1) switch is used
			// 2) switch loop back to parent of chain
			// 3) switch is blocked
			// 4) max depth reached
			level_down = 1;

			child_from_to = SW_I(tile->switches[ch->set.sw[
				ch->set.len-1]], !ch->from_to);
#ifdef DBG_ENUM_SWITCH
			printf(" child_from_to %s\n", connpt_str(
				ch->model, ch->y, ch->x,
				child_from_to));
#endif
			if (fpga_switch_is_used(ch->model, ch->y, ch->x, idx))
				level_down = 0;

			if (level_down) {
				// If we have the same from-switch already
				// among the parents, don't enter into a loop.
				for (i = 0; i < ch->set.len; i++) {
					int parent_connpt = SW_I(tile->switches[
						ch->set.sw[i]], ch->from_to);
#ifdef DBG_ENUM_SWITCH
					printf("  parent connpt %s%s\n", connpt_str(
						ch->model, ch->y, ch->x,
						parent_connpt), parent_connpt
						== child_from_to ? " (match)" : "");
#endif
					if (parent_connpt == child_from_to) {
						level_down = 0;
						break;
					}
				}
			}
			if (level_down) {
				// only go down if not blocked
				level_down = switch_list_contains(ch->model,
					ch->y, ch->x, ch->block_list,
					ch->block_list_len, idx) == NO_SWITCH;
			}
			if (level_down) {
				if (ch->set.len >= SW_SET_SIZE)
					{ HERE(); goto internal_error; }
				if (ch->max_depth < 1
				    || ch->set.len < ch->max_depth) {
					// back to first round at next level
#ifdef DBG_ENUM_SWITCH
					printf(" level_down %s\n",
					  fpga_switch_print(ch->model,
					  ch->y, ch->x, idx));
#endif
					ch->first_round = 1;
					ch->set.sw[ch->set.len] = idx;
					ch->set.len++;
					return 0;
				}
			}
		}
		while (1) {
			// todo: don't know why we catch blocklist duplicates
			//       if ch->set.len > 1 - needs more debugging
			if (switch_list_contains(ch->model, ch->y, ch->x,
				ch->block_list, ch->block_list_len,
				ch->set.sw[ch->set.len-1]) == NO_SWITCH) {
#ifdef DBG_ENUM_SWITCH
				printf(" block_list_add %s\n", fpga_switch_print(
					ch->model, ch->y, ch->x,
					ch->set.sw[ch->set.len-1]));
#endif
				if (ch->block_list_len >= MAX_SWITCHBOX_SIZE)
					{ HERE(); goto internal_error; }
				ch->block_list[ch->block_list_len++]
					= ch->set.sw[ch->set.len-1];
			}

			idx = fpga_switch_next(ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1], ch->from_to);
			if (idx != NO_SWITCH) {
				if (fpga_switch_is_used(ch->model, ch->y, ch->x, idx))
					continue;
#ifdef DBG_ENUM_SWITCH
				printf(" found %s\n", fpga_switch_print(
					ch->model, ch->y, ch->x, idx));
#endif
				ch->set.sw[ch->set.len-1] = idx;
				break;
			}

			if (ch->set.len <= 1) {
				ch->set.len = 0;
				return NO_SWITCH;
			}
#ifdef DBG_ENUM_SWITCH
			printf(" level_up\n");
#endif
			ch->set.len--;
		}
	}
internal_error:
	ch->set.len = 0;
	return NO_SWITCH;
}

int construct_sw_conns(struct sw_conns* conns, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth)
{
	memset(conns, 0, sizeof(*conns));
	construct_sw_chain(&conns->chain, model, y, x, start_switch,
		from_to, max_depth, /*block_list*/ 0, /*block_list_len*/ 0);
	return 0;
}

void destruct_sw_conns(struct sw_conns* conns)
{
	destruct_sw_chain(&conns->chain);
	memset(conns, 0, sizeof(*conns));
}

int fpga_switch_conns(struct sw_conns* conns)
{
	str16_t end_of_chain_str;

	if (!conns->chain.set.len) { HERE(); goto internal_error; }

	// on the first call, both dest_i and num_dests are 0
	while (conns->dest_i >= conns->num_dests) {
		fpga_switch_chain(&conns->chain);
		if (conns->chain.set.len == 0)
			return NO_CONN;
		end_of_chain_str = fpga_switch_str_i(conns->chain.model,
			conns->chain.y, conns->chain.x,
			conns->chain.set.sw[conns->chain.set.len-1],
			!conns->chain.from_to);
		if (end_of_chain_str == STRIDX_NO_ENTRY)
			{ HERE(); goto internal_error; }
		conns->dest_i = 0;
		fpga_connpt_find(conns->chain.model, conns->chain.y,
			conns->chain.x, end_of_chain_str,
			&conns->connpt_dest_start, &conns->num_dests);
		if (conns->num_dests)
			break;
	}
	fpga_conn_dest(conns->chain.model, conns->chain.y, conns->chain.x,
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
	str16_t sw, int from_to, int max_depth, swidx_t* block_list,
	int* block_list_len)
{
	struct sw_chain chain;
	int count = 0;

	printf("printf_swchain() y%02i x%02i %s blist_len %i\n",
		y, x, strarray_lookup(&model->str, sw),
		block_list_len ? *block_list_len : 0);
	if (construct_sw_chain(&chain, model, y, x, sw, from_to, max_depth,
		block_list, block_list_len ? *block_list_len : 0))
		{ HERE(); return; }
	while (fpga_switch_chain(&chain) != NO_CONN) {
		printf("sw %i %s\n", count++, fmt_swset(model, y, x,
			&chain.set, from_to));
	}
	printf("sw - block_list_len %i\n", chain.block_list_len);
	if (block_list_len)
		*block_list_len = chain.block_list_len;
	destruct_sw_chain(&chain);
}

void printf_swconns(struct fpga_model* model, int y, int x,
	str16_t sw, int from_to, int max_depth)
{
	struct sw_conns conns;

	if (construct_sw_conns(&conns, model, y, x, sw, from_to, max_depth))
		{ HERE(); return; }
	while (fpga_switch_conns(&conns) != NO_CONN) {
		printf("sw %s conn y%02i x%02i %s\n", fmt_swset(model, y, x,
			&conns.chain.set, from_to),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&model->str, conns.dest_str_i));
	}
	printf("sw -\n");
	destruct_sw_conns(&conns);
}

int fpga_switch_to_yx(struct switch_to_yx* p)
{
	struct sw_conns conns;
	struct sw_set best_set;
	int best_y, best_x, best_distance, distance;
	int best_num_dests, rc;
	str16_t best_connpt;

	rc = construct_sw_conns(&conns, p->model, p->y, p->x, p->start_switch,
		p->from_to, (p->flags & SWTO_YX_MAX_SWITCH_DEPTH)
			? p->max_switch_depth : SW_SET_SIZE);
	if (rc) FAIL(rc);

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
	destruct_sw_conns(&conns);
	return 0;
fail:
	return rc;
}

void printf_switch_to_yx_result(struct switch_to_yx* p)
{
	printf("switch_to_yx() from y%02i x%02i connpt %s (%s)\n",
		p->y, p->x, strarray_lookup(&p->model->str, p->start_switch),
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO");
	printf(" %s y%02i x%02i %s via %s\n",
		p->from_to == SW_FROM ? "to" : "from",
		p->dest_y, p->dest_x,
		strarray_lookup(&p->model->str, p->dest_connpt),
		fmt_swset(p->model, p->y, p->x, &p->set, p->from_to));
}

int fpga_switch_to_rel(struct switch_to_rel* p)
{
	struct sw_conns conns;
	int rc;

	rc = construct_sw_conns(&conns, p->model, p->start_y, p->start_x,
		p->start_switch, p->from_to, SW_SET_SIZE);
	if (rc) FAIL(rc);
	p->set.len = 0;
	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (conns.dest_y != p->start_y + p->rel_y
		    || conns.dest_x != p->start_x + p->rel_x)
			continue;
		if (p->target_connpt != STRIDX_NO_ENTRY
		    && conns.dest_str_i != p->target_connpt)
			continue;
		p->set = conns.chain.set;
		p->dest_y = conns.dest_y;
		p->dest_x = conns.dest_x;
		p->dest_connpt = conns.dest_str_i;
		break;
	}
	destruct_sw_conns(&conns);
	return 0;
fail:
	return rc;
}

void printf_switch_to_rel_result(struct switch_to_rel* p)
{
	printf("switch_to_rel() from y%02i x%02i connpt %s (%s)\n",
		p->start_y, p->start_x, strarray_lookup(&p->model->str, p->start_switch),
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO");
	printf(" %s y%02i x%02i %s via %s\n",
		p->from_to == SW_FROM ? "to" : "from",
		p->dest_y, p->dest_x,
		strarray_lookup(&p->model->str, p->dest_connpt),
		fmt_swset(p->model, p->start_y, p->start_x, &p->set, p->from_to));
}

#define NET_ALLOC_INCREMENT 64

static int fnet_useidx(struct fpga_model* model, net_idx_t new_idx)
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

int fnet_new(struct fpga_model* model, net_idx_t* new_idx)
{
	int rc;

	// highest_used_net is initialized to NO_NET which becomes 1
	rc = fnet_useidx(model, model->highest_used_net+1);
	if (rc) return rc;
	*new_idx = model->highest_used_net;
	return 0;
}

void fnet_delete(struct fpga_model* model, net_idx_t net_idx)
{
	struct fpga_net* net;
	int i;

	net = &model->nets[net_idx-1];
	for (i = 0; i < net->len; i++) {
		if (net->el[i].idx & NET_IDX_IS_PINW)
			continue;
		if (!fpga_switch_is_used(model, net->el[i].y, net->el[i].x,
			net->el[i].idx))
			HERE();
		fpga_switch_disable(model, net->el[i].y, net->el[i].x,
			net->el[i].idx);
	}
	model->nets[net_idx-1].len = 0;
	if (model->highest_used_net == net_idx)
		model->highest_used_net--;
}

int fnet_enum(struct fpga_model* model, net_idx_t last, net_idx_t* next)
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

struct fpga_net* fnet_get(struct fpga_model* model, net_idx_t net_i)
{
	if (net_i <= NO_NET
	    || net_i > model->highest_used_net) {
		fprintf(stderr, "%s:%i net_i %i highest_used %i\n", __FILE__,
			__LINE__, net_i, model->highest_used_net);
		return 0;
	}
	return &model->nets[net_i-1];
}

int fnet_add_port(struct fpga_model* model, net_idx_t net_i,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx,
	pinw_idx_t pinw_idx)
{
	struct fpga_net* net;
	int rc;

	rc = fnet_useidx(model, net_i);
	if (rc) FAIL(rc);
	
	net = &model->nets[net_i-1];
	if (net->len >= MAX_NET_LEN)
		FAIL(EINVAL);
	net->el[net->len].y = y;
	net->el[net->len].x = x;
	net->el[net->len].idx = pinw_idx | NET_IDX_IS_PINW;
	net->el[net->len].dev_idx = fpga_dev_idx(model, y, x, type, type_idx);
	if (net->el[net->len].dev_idx == NO_DEV) FAIL(EINVAL);
	net->len++;
	return 0;
fail:
	return rc;
}

int fnet_add_sw(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const swidx_t* switches, int num_sw)
{
	struct fpga_net* net;
	int i, rc;

	rc = fnet_useidx(model, net_i);
	if (rc) FAIL(rc);

	net = &model->nets[net_i-1];
	if (net->len+num_sw > MAX_NET_LEN)
		FAIL(EINVAL);
	for (i = 0; i < num_sw; i++) {
		net->el[net->len].y = y;
		net->el[net->len].x = x;
		if (OUT_OF_U16(switches[i])) FAIL(EINVAL);
		if (fpga_switch_is_used(model, y, x, switches[i]))
			HERE();
		fpga_switch_enable(model, y, x, switches[i]);
		net->el[net->len].idx = switches[i];
		net->len++;
	}
	return 0;
fail:
	return rc;
}

int fnet_remove_sw(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const swidx_t* switches, int num_sw)
{
	struct fpga_net* net;
	int i, j, rc;

	if (net_i > model->highest_used_net)
		FAIL(EINVAL);
	net = &model->nets[net_i-1];
	if (!net->len) {
		HERE();
		return 0;
	}
	for (i = 0; i < num_sw; i++) {
		for (j = 0; j < net->len; j++) {
			if (net->el[j].y == y
			    && net->el[j].x == x
			    && net->el[j].idx == switches[i])
				break;
		}
		if (j >= net->len) {
			// for now we expect the 'to-be-removed'
			// switches to be in the net
			HERE();
			continue;
		}
		if (!fpga_switch_is_used(model, y, x, switches[i]))
			HERE();
		fpga_switch_disable(model, y, x, switches[i]);
		if (net->len > j+1)
			memmove(&net->el[j], &net->el[j+1],
				(net->len-j-1)*sizeof(net->el[0]));
		net->len--;
	}
	return 0;
fail:
	return rc;
}

void fnet_free_all(struct fpga_model* model)
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

   	pin_str = fdev_pinw_idx2str(tile->devs[dev_idx].type, pinw_i);
	if (!pin_str) { HERE(); return; }

	snprintf(buf, sizeof(buf), "net %i %s y%02i x%02i %s %i pin %s\n",
		net_i, in_pin ? "in" : "out", el->y, el->x,
		fdev_type2str(tile->devs[dev_idx].type),
		fdev_typeidx(model, el->y, el->x, dev_idx),
		pin_str);
	fprintf(f, "%s", buf);
}

void fnet_printf(FILE* f, struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net;
	int i;

	net = fnet_get(model, net_i);
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

int fnet_autoroute(struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net_p;
	struct fpga_device* dev_p, *out_dev, *in_dev;
	struct switch_to_yx switch_to;
	struct sw_set logic_route_set, iologic_route_set;
	struct switch_to_rel switch_to_rel;
	int i, out_i, in_i, rc;

	// todo: gnd and vcc nets are special and have no outpin
	//       but lots of inpins

	net_p = fnet_get(model, net_i);
	if (!net_p) FAIL(EINVAL);
	out_i = -1;
	in_i = -1;
	for (i = 0; i < net_p->len; i++) {
		if (!(net_p->el[i].idx & NET_IDX_IS_PINW)) {
			// net already routed?
			FAIL(EINVAL);
		}
		dev_p = FPGA_DEV(model, net_p->el[i].y,
			net_p->el[i].x, net_p->el[i].dev_idx);
		if ((net_p->el[i].idx & NET_IDX_MASK) < dev_p->num_pinw_in) {
			// todo: right now we only support 1 inpin
			if (in_i != -1) FAIL(ENOTSUP);
			in_i = i;
			continue;
		}
		if (out_i != -1) FAIL(EINVAL); // at most 1 outpin
		out_i = i;
	}
	// todo: vcc and gnd have no outpin?
	if (out_i == -1 || in_i == -1)
		FAIL(EINVAL);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y,
			net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y,
			net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	if (out_dev->type == DEV_IOB) {
		if (in_dev->type != DEV_LOGIC)
			FAIL(ENOTSUP);

		// switch to ilogic
		switch_to.yx_req = YX_DEV_ILOGIC;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = net_p->el[out_i].y;
		switch_to.x = net_p->el[out_i].x;
		switch_to.start_switch = out_dev->pinw[net_p->el[out_i].idx
			& NET_IDX_MASK];
		switch_to.from_to = SW_FROM;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch to ilogic routing
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_FROM;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch from logic tile to logic routing tile
		switch_to_rel.model = model;
		switch_to_rel.start_y = net_p->el[in_i].y;
		switch_to_rel.start_x = net_p->el[in_i].x;
		switch_to_rel.start_switch =
			in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK];
		switch_to_rel.from_to = SW_TO;
		switch_to_rel.rel_y = 0;
		switch_to_rel.rel_x = -1;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		rc = fpga_switch_to_rel(&switch_to_rel);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.start_y,
			switch_to_rel.start_x, switch_to_rel.set.sw,
			switch_to_rel.set.len);
		if (rc) FAIL(rc);

		// route from ilogic routing to logic routing
		rc = froute_direct(model, switch_to.dest_y, switch_to.dest_x,
			switch_to.dest_connpt,
			switch_to_rel.dest_y, switch_to_rel.dest_x,
			switch_to_rel.dest_connpt,
			&iologic_route_set, &logic_route_set);
		if (rc) FAIL(rc);
		if (!iologic_route_set.len || !logic_route_set.len) FAIL(EINVAL);
		rc = fnet_add_sw(model, net_i, switch_to.dest_y, switch_to.dest_x,
			iologic_route_set.sw, iologic_route_set.len);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, net_p->el[in_i].y, net_p->el[in_i].x-1,
			logic_route_set.sw, logic_route_set.len);
		if (rc) FAIL(rc);

		return 0;
	}
	if (out_dev->type == DEV_LOGIC) {
		if (in_dev->type != DEV_IOB)
			FAIL(ENOTSUP);

		// switch from ologic to iob
		switch_to.yx_req = YX_DEV_OLOGIC;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = net_p->el[in_i].y;
		switch_to.x = net_p->el[in_i].x;
		switch_to.start_switch = in_dev->pinw[net_p->el[in_i].idx
			& NET_IDX_MASK];
		switch_to.from_to = SW_TO;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switches inside ologic to ologic routing
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_TO;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch inside logic tile
		switch_to_rel.model = model;
		switch_to_rel.start_y = net_p->el[out_i].y;
		switch_to_rel.start_x = net_p->el[out_i].x;
		switch_to_rel.start_switch =
			out_dev->pinw[net_p->el[out_i].idx & NET_IDX_MASK];
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.rel_y = 0;
		switch_to_rel.rel_x = -1;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		rc = fpga_switch_to_rel(&switch_to_rel);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.start_y,
			switch_to_rel.start_x, switch_to_rel.set.sw,
			switch_to_rel.set.len);
		if (rc) FAIL(rc);

		// route from logic routing to ilogic routing
		rc = froute_direct(model, switch_to_rel.dest_y,
			switch_to_rel.dest_x, switch_to_rel.dest_connpt,
			switch_to.dest_y, switch_to.dest_x, switch_to.dest_connpt,
			&logic_route_set, &iologic_route_set);
		if (rc) FAIL(rc);
		if (!iologic_route_set.len || !logic_route_set.len) FAIL(EINVAL);
		rc = fnet_add_sw(model, net_i, switch_to.dest_y, switch_to.dest_x,
			iologic_route_set.sw, iologic_route_set.len);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.dest_y,
			switch_to_rel.dest_x, logic_route_set.sw,
			logic_route_set.len);
		if (rc) FAIL(rc);

		return 0;
	}
	FAIL(ENOTSUP);
fail:
	return rc;
}

int fnet_route_to_inpins(struct fpga_model* model, net_idx_t net_i,
	const char* from)
{
	struct fpga_net* net_p;
	struct fpga_device* dev_p;
	str16_t from_i;
	struct sw_set start_set, end_set;
	int i, rc;

	net_p = fnet_get(model, net_i);
	if (!net_p) FAIL(EINVAL);
	from_i = strarray_find(&model->str, from);
	if (from_i == STRIDX_NO_ENTRY) FAIL(EINVAL);

	for (i = 0; i < net_p->len; i++) {
		if (!(net_p->el[i].idx & NET_IDX_IS_PINW))
			// skip existing switch
			continue;
		dev_p = FPGA_DEV(model, net_p->el[i].y,
			net_p->el[i].x, net_p->el[i].dev_idx);
		if ((net_p->el[i].idx & NET_IDX_MASK) >= dev_p->num_pinw_in)
			// skip outpin
			continue;
		rc = froute_direct(model, net_p->el[i].y, net_p->el[i].x-1,
			from_i, net_p->el[i].y, net_p->el[i].x,
			dev_p->pinw[net_p->el[i].idx & NET_IDX_MASK],
			&start_set, &end_set);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, net_p->el[i].y,
			net_p->el[i].x-1, start_set.sw, start_set.len);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, net_p->el[i].y,
			net_p->el[i].x, end_set.sw, end_set.len);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

int froute_direct(struct fpga_model* model, int start_y, int start_x,
	str16_t start_pt, int end_y, int end_x, str16_t end_pt,
	struct sw_set* start_set, struct sw_set* end_set)
{
	struct sw_conns conns;
	struct sw_set end_switches;
	int i, rc;

	rc = fpga_swset_fromto(model, end_y, end_x, end_pt, SW_TO, &end_switches);
	if (rc) FAIL(rc);
	if (!end_switches.len) FAIL(EINVAL);

	rc = construct_sw_conns(&conns, model, start_y, start_x, start_pt,
		SW_FROM, /*max_depth*/ 1);
	if (rc) FAIL(rc);

	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (conns.dest_y != end_y
		    || conns.dest_x != end_x) continue;
		for (i = 0; i < end_switches.len; i++) {
			if (conns.dest_str_i == fpga_switch_str_i(model, end_y,
			    end_x, end_switches.sw[i], SW_FROM)) {
				*start_set = conns.chain.set;
				end_set->len = 1;
				end_set->sw[0] = end_switches.sw[i];
				return 0;
			}
		}
	}
	destruct_sw_conns(&conns);
	start_set->len = 0;
	end_set->len = 0;
	return 0;
fail:
	return rc;
}
