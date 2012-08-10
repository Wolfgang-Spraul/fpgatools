//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

static int init_tiles(struct fpga_model* model);
static int init_wires(struct fpga_model* model);
static int init_ports(struct fpga_model* model);
static int init_devices(struct fpga_model* model);
static int init_switches(struct fpga_model* model);

static int run_gclk(struct fpga_model* model);
static int run_gclk_horiz_regs(struct fpga_model* model);
static int run_gclk_vert_regs(struct fpga_model* model);
static int run_logic_inout(struct fpga_model* model);
static int run_io_wires(struct fpga_model* model);
static int run_direction_wires(struct fpga_model* model);

static const char* pf(const char* fmt, ...);
static const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name);
static char next_non_whitespace(const char* s);
static char last_major(const char* str, int cur_o);
int has_connpt(struct fpga_model* model, int y, int x, const char* name);
static int add_connpt_name(struct fpga_model* model, int y, int x, const char* connpt_name);

static int has_device(struct fpga_model* model, int y, int x, int dev);
static int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2);

typedef int (*add_conn_f)(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
#define NOPREF_BI_F	add_conn_bi
#define PREF_BI_F	add_conn_bi_pref
#define NOPREF_UNI_F	add_conn_uni
#define PREF_UNI_F	add_conn_uni_pref

static int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
int add_conn_uni_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_bi_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func, int y1, int x1, const char* name1, int start1, int last1, int y2, int x2, const char* name2, int start2);

// COUNT_DOWN can be OR'ed to start_count to make
// the enumerated wires count from start_count down.
#define COUNT_DOWN		0x100
#define COUNT_MASK		0xFF

struct w_point // wire point
{
	const char* name;
	int start_count; // if there is a %i in the name, this is the start number
	int y, x;
};

#define NO_INCREMENT 0

struct w_net
{
	// if !last_inc, no incrementing will happen (NO_INCREMENT)
	// if last_inc > 0, incrementing will happen to
	// the %i in the name from 0:last_inc, for a total
	// of last_inc+1 wires.
	int last_inc;
	struct w_point pts[40];
};

static int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net);

static int add_switch(struct fpga_model* model, int y, int x, const char* from,
	const char* to, int is_bidirectional);

struct seed_data
{
	int x_flags;
	const char* str;
};

static void seed_strx(struct fpga_model* model, struct seed_data* data);

int fpga_build_model(struct fpga_model* model, int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring)
{
	int rc;

	memset(model, 0, sizeof(*model));
	model->cfg_rows = fpga_rows;
	strncpy(model->cfg_columns, columns, sizeof(model->cfg_columns)-1);
	strncpy(model->cfg_left_wiring, left_wiring,
		sizeof(model->cfg_left_wiring)-1);
	strncpy(model->cfg_right_wiring, right_wiring,
		sizeof(model->cfg_right_wiring)-1);
	strarray_init(&model->str, STRIDX_64K);

	// The order of tiles, then devices, then ports, then
	// connections and finally switches is important so
	// that the codes can build upon each other.

	rc = init_tiles(model);
	if (rc) return rc;

	rc = init_devices(model);
	if (rc) return rc;

	rc = init_ports(model);
	if (rc) return rc;

	rc = init_wires(model);
	if (rc) return rc;

return 0;

	rc = init_switches(model);
	if (rc) return rc;

	return 0;
}

void fpga_free_model(struct fpga_model* model)
{
	if (!model) return;
	free(model->tmp_str);
	strarray_free(&model->str);
	free(model->tiles);
	memset(model, 0, sizeof(*model));
}

// The wires are ordered clockwise. Order is important for
// wire_to_NESW4().
enum wire_type
{
	FIRST_LEN1 = 1,
	W_NL1 = FIRST_LEN1,
	W_NR1,
	W_EL1,
	W_ER1,
	W_SL1,
	W_SR1,
	W_WL1,
	W_WR1,
	LAST_LEN1 = W_WR1,

	FIRST_LEN2,
	W_NN2 = FIRST_LEN2,
	W_NE2,
	W_EE2,
	W_SE2,
	W_SS2,
	W_SW2,
	W_WW2,
	W_NW2,
	LAST_LEN2 = W_NW2,

	FIRST_LEN4,
	W_NN4 = FIRST_LEN4,
	W_NE4,
	W_EE4,
	W_SE4,
	W_SS4,
	W_SW4,
	W_WW4,
	W_NW4,
	LAST_LEN4 = W_NW4
};

static const char* wire_base(enum wire_type w)
{
	switch (w) {
		case W_NL1: return "NL1";
		case W_NR1: return "NR1";
		case W_EL1: return "EL1";
		case W_ER1: return "ER1";
		case W_SL1: return "SL1";
		case W_SR1: return "SR1";
		case W_WL1: return "WL1";
		case W_WR1: return "WR1";

		case W_NN2: return "NN2";
		case W_NE2: return "NE2";
		case W_EE2: return "EE2";
		case W_SE2: return "SE2";
		case W_SS2: return "SS2";
		case W_SW2: return "SW2";
		case W_WW2: return "WW2";
		case W_NW2: return "NW2";

		case W_NN4: return "NN4";
		case W_NE4: return "NE4";
		case W_EE4: return "EE4";
		case W_SE4: return "SE4";
		case W_SS4: return "SS4";
		case W_SW4: return "SW4";
		case W_WW4: return "WW4";
		case W_NW4: return "NW4";
	}
	ABORT(1);
}

#define W_CLOCKWISE(w)			rotate_wire((w), 1)
#define W_CLOCKWISE_2(w)		rotate_wire((w), 2)
#define W_COUNTER_CLOCKWISE(w)		rotate_wire((w), -1)
#define W_COUNTER_CLOCKWISE_2(w)	rotate_wire((w), -2)

#define W_IS_LEN1(w)			((w) >= FIRST_LEN1 && (w) <= LAST_LEN1)
#define W_IS_LEN2(w)			((w) >= FIRST_LEN2 && (w) <= LAST_LEN2)
#define W_IS_LEN4(w)			((w) >= FIRST_LEN4 && (w) <= LAST_LEN4)

#define W_TO_LEN1(w)			wire_to_len(w, FIRST_LEN1)
#define W_TO_LEN2(w)			wire_to_len(w, FIRST_LEN2)
#define W_TO_LEN4(w)			wire_to_len(w, FIRST_LEN4)

int rotate_num(int cur, int off, int first, int last)
{
	if (cur+off > last)
		return first + (cur+off-last-1) % ((last+1)-first);
	if (cur+off < first)
		return last - (first-(cur+off)-1) % ((last+1)-first);
	return cur+off;
}

enum wire_type rotate_wire(enum wire_type cur, int off)
{
	if (W_IS_LEN1(cur))
		return rotate_num(cur, off, FIRST_LEN1, LAST_LEN1);
	if (W_IS_LEN2(cur))
		return rotate_num(cur, off, FIRST_LEN2, LAST_LEN2);
	if (W_IS_LEN4(cur))
		return rotate_num(cur, off, FIRST_LEN4, LAST_LEN4);
	ABORT(1);
}

enum wire_type wire_to_len(enum wire_type w, int first_len)
{
	if (W_IS_LEN1(w))
		return w-FIRST_LEN1 + first_len;
	if (W_IS_LEN2(w))
		return w-FIRST_LEN2 + first_len;
	if (W_IS_LEN4(w))
		return w-FIRST_LEN4 + first_len;
	ABORT(1);
}

enum wire_type wire_to_NESW4(enum wire_type w)
{
	// normalizes any of the 8 directions to just N/E/S/W
	// by going back to an even number.
	w = W_TO_LEN4(w);
	return w - (w-FIRST_LEN4)%2;
}
	
// longest should be something like "WW2E_N3"?
typedef char WIRE_NAME[8];

struct one_switch
{
	WIRE_NAME from;
	WIRE_NAME to;
};

struct set_of_switches
{
	int num_s;
	struct one_switch s[64];
};

void add_switch_range(struct set_of_switches* dest,
	enum wire_type end_wire, int end_from, int end_to,
	enum wire_type beg_wire, int beg_from)
{
	int i;
	for (i = end_from; i <= end_to; i++) {
		sprintf(dest->s[dest->num_s].from, "%sE%i", wire_base(end_wire), i);
		sprintf(dest->s[dest->num_s].to, "%sB%i", wire_base(beg_wire), beg_from + (i-end_from));
		dest->num_s++;
	}
}

void add_switch_E3toB0(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s = wire_base(end_wire);
	const char* beg_wire_s = wire_base(beg_wire);

	sprintf(dest->s[dest->num_s].from, "%sE3", end_wire_s);
	sprintf(dest->s[dest->num_s].to, "%sB0", beg_wire_s);
	dest->num_s++;
	add_switch_range(dest, end_wire, 0, 2, beg_wire, 1);
}

void add_switch_E0toB3(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s = wire_base(end_wire);
	const char* beg_wire_s = wire_base(beg_wire);

	sprintf(dest->s[dest->num_s].from, "%sE0", end_wire_s);
	sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
	dest->num_s++;
	add_switch_range(dest, end_wire, 1, 3, beg_wire, 0);
}

int add_switches(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s, *beg_wire_s;
	int i;

	end_wire_s = wire_base(end_wire);
	beg_wire_s = wire_base(beg_wire);

	//
	// First the directional routing at the end of len-1 wires.
	//

	if (W_IS_LEN1(end_wire)) {
		if (end_wire == W_WL1) {
			if (beg_wire == W_NL1) {
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					0, 2, beg_wire, 0);
				return 0;
			}
			if (beg_wire == W_WR1) {
				add_switch_range(dest, end_wire,
					0, 1, beg_wire, 2);
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB1",
					beg_wire_s);
				dest->num_s++;
				sprintf(dest->s[dest->num_s].from, "%sE2",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_NN2 || beg_wire == W_NW2) {
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					0, 2, beg_wire, 1);
				return 0;
			}
			if (beg_wire == W_SR1) {
				add_switch_E3toB0(dest, end_wire, beg_wire);
				return 0;
			}
			if (beg_wire == W_WL1) {
				add_switch_E0toB3(dest, end_wire, beg_wire);
				return 0;
			}
			add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
			return 0;
		}
		if (end_wire == W_WR1) {
			if (beg_wire == W_SW2 || beg_wire == W_WW2) {
				add_switch_range(dest, end_wire,
					1, 3, beg_wire, 0);
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_SR1) {
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					1, 3, beg_wire, 1);
				return 0;
			}
			if (beg_wire == W_WL1) {
				add_switch_range(dest, end_wire,
					2, 3, beg_wire, 0);
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB2",
					beg_wire_s);
				dest->num_s++;
				sprintf(dest->s[dest->num_s].from, "%sE1",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_WR1) {
				add_switch_E3toB0(dest, end_wire, beg_wire);
				return 0;
			}
			if (beg_wire == W_NL1) {
				add_switch_E0toB3(dest, end_wire, beg_wire);
				return 0;
			}
			add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
			return 0;
		}
		if (beg_wire == W_WR1 || beg_wire == W_ER1
		    || beg_wire == W_SR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_NL1 || beg_wire == W_EL1
		    || beg_wire == W_WL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	//
	// The rest of the function is for directional routing
	// at the end of len-2 and len-4 wires.
	//

	if (end_wire == W_WW2) {
		if (beg_wire == W_NL1) {
			add_switch_range(dest, end_wire, 0, 2, beg_wire, 0);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		if (beg_wire == W_WR1) {
			add_switch_range(dest, end_wire, 0, 1, beg_wire, 2);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB1",
				beg_wire_s);
			dest->num_s++;
			sprintf(dest->s[dest->num_s].from, "%sE2",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		if (beg_wire == W_WL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_SR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_WW4 || beg_wire == W_NN2
		    || beg_wire == W_NW2 || beg_wire == W_NE4
		    || beg_wire == W_NN4 || beg_wire == W_NW4) {
			add_switch_range(dest, end_wire, 0, 2, beg_wire, 1);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	if (end_wire == W_NW2 || end_wire == W_NW4
	    || end_wire == W_WW4) {
		if (beg_wire == W_NL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_WR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_SR1) {
			sprintf(dest->s[dest->num_s].from, "%sE_S0",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			add_switch_range(dest, end_wire, 1, 3, beg_wire, 1);
			return 0;
		}
		if (beg_wire == W_WL1) {
			sprintf(dest->s[dest->num_s].from, "%sE_S0",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB2",
				beg_wire_s);
			dest->num_s++;
			sprintf(dest->s[dest->num_s].from, "%sE1", end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
			dest->num_s++;
			add_switch_range(dest, end_wire, 2, 3, beg_wire, 0);
			return 0;
		}
		if (beg_wire == W_SS4 || beg_wire == W_SW2
		    || beg_wire == W_SW4 || beg_wire == W_WW2) {
			add_switch_range(dest, end_wire, 1, 3, beg_wire, 0);
			sprintf(dest->s[dest->num_s].from, "%sE_S0", end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
			dest->num_s++;
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	if ((end_wire == W_SS2 || end_wire == W_SS4
	     || end_wire == W_SW2 || end_wire == W_SW4)
	    && (beg_wire == W_NW4 || beg_wire == W_WW4)) {
		for (i = 0; i <= 2; i++) {
			sprintf(dest->s[dest->num_s].from, "%sE%i",
				end_wire_s, i);
			sprintf(dest->s[dest->num_s].to, "%sB%i",
				beg_wire_s, i+1);
			dest->num_s++;
		}
		sprintf(dest->s[dest->num_s].from, "%sE_N3", end_wire_s);
		sprintf(dest->s[dest->num_s].to, "%sB0", beg_wire_s);
		dest->num_s++;
		return 0;
	}

	if (beg_wire == W_WR1 || beg_wire == W_ER1 || beg_wire == W_SR1) {
		add_switch_E3toB0(dest, end_wire, beg_wire);
		return 0;
	}

	if (beg_wire == W_WL1 || beg_wire == W_EL1 || beg_wire == W_NL1) {
		add_switch_E0toB3(dest, end_wire, beg_wire);
		return 0;
	}

	add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
	return 0;
}

static int build_dirwire_switches(struct set_of_switches* dest,
	enum wire_type src_wire)
{
	enum wire_type cur;
	int i, rc;

	dest->num_s = 0;
	if (W_IS_LEN2(src_wire) || W_IS_LEN4(src_wire)) {
		cur = W_COUNTER_CLOCKWISE_2(wire_to_NESW4(src_wire));
		for (i = 0; i < 6; i++) {
			rc = add_switches(dest, src_wire, cur);
			if (rc) goto xout;
			cur = W_CLOCKWISE(cur);
		}
	}
	cur = W_COUNTER_CLOCKWISE(W_TO_LEN2(wire_to_NESW4(src_wire)));
	for (i = 0; i < 4; i++) {
		rc = add_switches(dest, src_wire, cur);
		if (rc) goto xout;
		cur = W_CLOCKWISE(cur);
	}
	cur = W_COUNTER_CLOCKWISE(W_TO_LEN1(wire_to_NESW4(src_wire)));
	for (i = 0; i < 4; i++) {
		rc = add_switches(dest, src_wire, cur);
		if (rc) goto xout;
		cur = W_CLOCKWISE(cur);
	}
	return 0;
xout:
	return rc;
}

// The LWF flags are OR'ed into the logic_wire enum
#define LWF_SOUTH0		0x0100
#define LWF_NORTH3		0x0200
#define LWF_BIDIR		0x0400
#define LWF_FAN_B		0x0800
#define LWF_WIRE_MASK		0x00FF // namespace for the enums

enum logicin_wire {
    /*  0 */	X_A1 = 0,
		      X_A2, X_A3, X_A4, X_A5, X_A6, X_AX,
    /*  7 */	X_B1, X_B2, X_B3, X_B4, X_B5, X_B6, X_BX,
    /* 14 */	X_C1, X_C2, X_C3, X_C4, X_C5, X_C6, X_CE, X_CX,
    /* 22 */	X_D1, X_D2, X_D3, X_D4, X_D5, X_D6, X_DX,
    /* 29 */	M_A1, M_A2, M_A3, M_A4, M_A5, M_A6, M_AX, M_AI,
    /* 37 */	M_B1, M_B2, M_B3, M_B4, M_B5, M_B6, M_BX, M_BI,
    /* 45 */	M_C1, M_C2, M_C3, M_C4, M_C5, M_C6, M_CE, M_CX, M_CI,
    /* 54 */	M_D1, M_D2, M_D3, M_D4, M_D5, M_D6, M_DX, M_DI,
    /* 62 */	M_WE
};

enum logicout_wire {
    /*  0 */	X_A = 0,
		     X_AMUX, X_AQ, X_B, X_BMUX, X_BQ,
    /*  6 */	X_C, X_CMUX, X_CQ, X_D, X_DMUX, X_DQ,
    /* 12 */	M_A, M_AMUX, M_AQ, M_B, M_BMUX, M_BQ,
    /* 18 */	M_C, M_CMUX, M_CQ, M_D, M_DMUX, M_DQ
};

// The extra wires must not overlap with logicin_wire or logicout_wire
// namespaces so that they can be combined with either of them.
enum extra_wires {
	UNDEF = 100,
	FAN_B,
	GFAN0,
	GFAN1,
	LOGICIN20,
	LOGICIN21,
	LOGICIN44,
	LOGICIN52,
	LOGICIN_N21,
	LOGICIN_N28,
	LOGICIN_N52,
	LOGICIN_N60,
	LOGICIN_S20,
	LOGICIN_S36,
	LOGICIN_S44,
	LOGICIN_S62
};

static const char* logicin_s(int wire, int routing_io)
{
	if (routing_io && ((wire & LWF_WIRE_MASK) == X_A5 || (wire & LWF_WIRE_MASK) == X_B4))
		return pf("INT_IOI_LOGICIN_B%i", wire & LWF_WIRE_MASK);
	return pf("LOGICIN_B%i", wire & LWF_WIRE_MASK);
}

int add_logicio_extra(struct fpga_model* model, int y, int x, int routing_io)
{
	// 16 groups of 4. The order inside the group does not matter,
	// but the order of the groups must match the order in src_w.
	static int dest_w[] = {
		/* group  0 */ M_D1, X_A1, X_CE, X_BX  | LWF_BIDIR,
		/* group  1 */ M_B2, M_WE, X_C2, M_AX  | LWF_BIDIR,
		/* group  2 */ M_C1, M_AI, X_B1, X_AX  | LWF_BIDIR,
		/* group  3 */ M_A2, M_BI, X_D2, M_BX  | LWF_BIDIR,
		/* group  4 */ M_C2, M_DX, X_B2, FAN_B | LWF_BIDIR,
		/* group  5 */ M_A1, X_CX, X_D1, M_CE  | LWF_BIDIR,
		/* group  6 */ M_CX, M_D2, X_A2, M_CI  | LWF_BIDIR,
		/* group  7 */ M_B1, X_C1, X_DX, M_DI  | LWF_BIDIR,
		/* group  8 */ M_A5, M_C4, X_B5, X_D4,
		/* group  9 */ M_A6, M_C3, X_B6, X_D3,
		/* group 10 */ M_B5, M_D4, X_A5, X_C4,
		/* group 11 */ M_B6, M_D3, X_A6, X_C3,
		/* group 12 */ M_B4, M_D5, X_A4, X_C5,
		/* group 13 */ M_B3, M_D6, X_A3, X_C6,
		/* group 14 */ M_A3, M_C6, X_B3, X_D6,
		/* group 15 */ M_A4, M_C5, X_B4, X_D5,
	};
	// 16 groups of 5. Order of groups in sync with in_w.
	// Each dest_w group can only have 1 bidir wire, which is
	// flagged there. The flag in src_w signals whether that one
	// bidir line in dest_w is to be driven as bidir or not.
	static int src_w[] = {
		/* group  0 */ GFAN0,   	  M_AX,			M_CI | LWF_BIDIR,  M_DI | LWF_BIDIR, LOGICIN_N28,
		/* group  1 */ GFAN0 | LWF_BIDIR, LOGICIN20,   		M_CI | LWF_BIDIR,  LOGICIN_N52,	     LOGICIN_N28,
		/* group  2 */ GFAN0 | LWF_BIDIR, M_CE | LWF_BIDIR,	LOGICIN_N21,	   LOGICIN44,        LOGICIN_N60,
		/* group  3 */ GFAN0,  	          FAN_B | LWF_BIDIR,	X_AX,   	   M_CE | LWF_BIDIR, LOGICIN_N60,
		/* group  4 */ GFAN1,     	  M_BX | LWF_BIDIR,	LOGICIN21,	   LOGICIN_S44,      LOGICIN_S36,
		/* group  5 */ GFAN1 | LWF_BIDIR, FAN_B,		M_BX | LWF_BIDIR,  X_AX | LWF_BIDIR, LOGICIN_S36,
		/* group  6 */ GFAN1 | LWF_BIDIR, M_AX | LWF_BIDIR,	X_BX | LWF_BIDIR,  M_DI,             LOGICIN_S62,
		/* group  7 */ GFAN1,             LOGICIN52,		X_BX | LWF_BIDIR,  LOGICIN_S20,	     LOGICIN_S62,

		/* group  8 */ M_AX,      M_CI,        M_DI,        LOGICIN_N28, UNDEF,
		/* group  9 */ LOGICIN20, M_CI,        LOGICIN_N52, LOGICIN_N28, UNDEF,
		/* group 10 */ FAN_B,     X_AX,        M_CE,        LOGICIN_N60, UNDEF,
		/* group 11 */ M_CE,      LOGICIN_N21, LOGICIN44,   LOGICIN_N60, UNDEF,
		/* group 12 */ FAN_B,     M_BX,        X_AX,        LOGICIN_S36, UNDEF,
		/* group 13 */ M_BX,      LOGICIN21,   LOGICIN_S44, LOGICIN_S36, UNDEF,
		/* group 14 */ LOGICIN52, X_BX,        LOGICIN_S20, LOGICIN_S62, UNDEF,
		/* group 15 */ M_AX,      X_BX,        M_DI,        LOGICIN_S62, UNDEF
	};
	char from_str[32], to_str[32];
	int i, j, cur_dest_w, is_bidir, rc;

	for (i = 0; i < sizeof(src_w)/sizeof(src_w[0]); i++) {
		for (j = 0; j < 4; j++) {

			cur_dest_w = dest_w[(i/5)*4 + j];
			is_bidir = (cur_dest_w & LWF_BIDIR) && (src_w[i] & LWF_BIDIR);
			if ((cur_dest_w & LWF_WIRE_MASK) == FAN_B)
				strcpy(to_str, "FAN_B");
			else
				strcpy(to_str, logicin_s(cur_dest_w, routing_io));

			switch (src_w[i] & LWF_WIRE_MASK) {
				case UNDEF: continue;
				default:
					snprintf(from_str, sizeof(from_str), "LOGICIN_B%i",
						src_w[i] & LWF_WIRE_MASK);
					break;
				case GFAN0:
				case GFAN1:
					if (routing_io) {
						is_bidir = 0;
						strcpy(from_str, "VCC_WIRE");
					} else {
						strcpy(from_str, (src_w[i] & LWF_WIRE_MASK)
							== GFAN0 ? "GFAN0" : "GFAN1");
					}
					break;
				case FAN_B:		strcpy(from_str, "FAN_B"); break;
				case LOGICIN20:		strcpy(from_str, "LOGICIN20"); break;
				case LOGICIN21:		strcpy(from_str, "LOGICIN21"); break;
				case LOGICIN44:		strcpy(from_str, "LOGICIN44"); break;
				case LOGICIN52:		strcpy(from_str, "LOGICIN52"); break;
				case LOGICIN_N21:	strcpy(from_str, "LOGICIN_N21"); break;
				case LOGICIN_N28:	strcpy(from_str, "LOGICIN_N28"); break;
				case LOGICIN_N52:	strcpy(from_str, "LOGICIN_N52"); break;
				case LOGICIN_N60:	strcpy(from_str, "LOGICIN_N60"); break;
				case LOGICIN_S20:	strcpy(from_str, "LOGICIN_S20"); break;
				case LOGICIN_S36:	strcpy(from_str, "LOGICIN_S36"); break;
				case LOGICIN_S44:	strcpy(from_str, "LOGICIN_S44"); break;
				case LOGICIN_S62:	strcpy(from_str, "LOGICIN_S62"); break;
			}
			rc = add_switch(model, y, x, from_str, to_str, is_bidir);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

int add_logicout_switches(struct fpga_model* model, int y, int x, int routing_io)
{
	// 8 groups of 3. The order inside the group does not matter,
	// but the order of the groups does.
	static int out_wires[] = {
		/* group 0 */ M_A,    M_CMUX, X_AQ,
		/* group 1 */ M_AQ,   X_A,    X_CMUX,
		/* group 2 */ M_BQ,   X_B,    X_DMUX,
		/* group 3 */ M_B,    M_DMUX, X_BQ,
		/* group 4 */ M_AMUX, M_C,    X_CQ,
		/* group 5 */ M_CQ,   X_AMUX, X_C,
		/* group 6 */ M_BMUX, M_D,    X_DQ,
		/* group 7 */ M_DQ,   X_BMUX, X_D
	};
	// Those are the logicout wires going back into logicin, for
	// each group of out wires. Again the order inside the groups
	// does not matter, but the group order must match the out wire
	// group order.
	static int logicin_wires[] = {
		/* group 0 */ M_AI, M_B6, M_C1, M_D3, X_A6, X_AX, X_B1, X_C3,
		/* group 1 */ M_A6, M_AX, M_B2, M_C3, M_WE, X_B6, X_C2, X_D3,
		/* group 2 */ M_A2, M_B5, M_BI, M_BX, M_D4, X_A5, X_C4, X_D2,
		/* group 3 */ M_A5, M_C4, M_D1, X_A1, X_B5, X_BX, X_CE, X_D4,
		/* group 4 */ M_A1, M_B4, M_CE, M_D5, X_A4, X_C5, X_CX, X_D1,
		/* group 5 */ M_A4, M_C5, M_CI, M_CX, M_D2, X_A2, X_B4, X_D5,
		/* group 6 */ M_A3, M_B1, M_C6, M_DI, X_B3, X_C1, X_D6, X_DX,
		/* group 7 */ M_B3, M_C2, M_D6, M_DX, X_A3, X_B2, X_C6, FAN_B
	};
	enum wire_type wire;
	char from_str[32], to_str[32];
	int i, j, rc;

	for (i = 0; i < sizeof(out_wires)/sizeof(out_wires[0]); i++) {
		// out to dirwires
		snprintf(from_str, sizeof(from_str), "LOGICOUT%i",
			out_wires[i]);
		wire = W_NN2;
		do {
			// len 2
			snprintf(to_str, sizeof(to_str), "%sB%i",
				wire_base(wire), i/(2*3));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;

			// len 4
			snprintf(to_str, sizeof(to_str), "%sB%i",
				wire_base(W_TO_LEN4(wire)), i/(2*3));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;

			wire = W_CLOCKWISE(wire);
		} while (wire != W_NN2); // one full turn

		// NR1, SL1
		snprintf(to_str, sizeof(to_str), "NR1B%i", i/(2*3));
		rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
		if (rc) goto xout;
		snprintf(to_str, sizeof(to_str), "SL1B%i", i/(2*3));
		rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
		if (rc) goto xout;

		// ER1, SR1, WR1 (+1)
		// NL1, EL1, WL1 (+3)
		{
			static const char* plus1[] = {"ER1B%i", "SR1B%i", "WR1B%i"};
			static const char* plus3[] = {"NL1B%i", "EL1B%i", "WL1B%i"};
			for (j = 0; j < 3; j++) {
				snprintf(to_str, sizeof(to_str), plus1[j], (i/(2*3)+1)%4);
				rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
				if (rc) goto xout;

				snprintf(to_str, sizeof(to_str), plus3[j], (i/(2*3)+3)%4);
				rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
				if (rc) goto xout;
			}
		}
			
		// back to logicin
		for (j = 0; j < 8; j++) {
			if (logicin_wires[(i/3)*8 + j] == FAN_B)
				strcpy(to_str, "FAN_B");
			else
				strcpy(to_str, logicin_s(logicin_wires[(i/3)*8 + j], routing_io));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int add_logicin_switch(struct fpga_model* model, int y, int x,
	enum wire_type dirwire, int dirwire_num,
	int logicin_num)
{
	char from_str[32], to_str[32];
	int rc;

	if (dirwire_num == 0 && logicin_num & LWF_SOUTH0)
		snprintf(from_str, sizeof(from_str), "%sE_S0",
			wire_base(dirwire));
	else if (dirwire_num == 3 && logicin_num & LWF_NORTH3)
		snprintf(from_str, sizeof(from_str), "%sE_N3",
			wire_base(dirwire));
	else
		snprintf(from_str, sizeof(from_str), "%sE%i",
			wire_base(dirwire), dirwire_num);
	if ((logicin_num & LWF_WIRE_MASK) == FAN_B)
		strcpy(to_str, "FAN_B");
	else {
		struct fpga_tile* tile = YX_TILE(model, y, x);
		int routing_io = (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L);
		strcpy(to_str, logicin_s(logicin_num, routing_io));
	}
	rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

// This function adds the switches for all dirwires in the
// quarter belonging to dirwire. So dirwire should only be
// one of W_NN2, W_EE2, W_SS2 or W_WW2 - the rest is handled
// inside the function.
static int add_logicin_switch_quart(struct fpga_model* model, int y, int x,
	enum wire_type dirwire, int dirwire_num,
	int logicin_num)
{
	enum wire_type len1;
	int rc;

	rc = add_logicin_switch(model, y, x, dirwire, dirwire_num,
		logicin_num);
	if (rc) goto xout;
	len1 = W_COUNTER_CLOCKWISE(W_TO_LEN1(dirwire));
	rc = add_logicin_switch(model, y, x, len1,
		dirwire_num, logicin_num);
	if (rc) goto xout;

	if (dirwire == W_WW2) {
		int nw_num = dirwire_num+1;
		if (nw_num > 3)
			nw_num = 0;
		rc = add_logicin_switch(model, y, x, W_NW2, nw_num,
			logicin_num);
		if (rc) goto xout;
		rc = add_logicin_switch(model, y, x, W_NL1, nw_num,
			logicin_num);
		if (rc) goto xout;
	} else {
		rc = add_logicin_switch(model, y, x, W_CLOCKWISE(dirwire),
			dirwire_num, logicin_num);
		if (rc) goto xout;
		len1 = rotate_wire(len1, 3);
		rc = add_logicin_switch(model, y, x, len1,
			dirwire_num, logicin_num);
		if (rc) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int loop_and_rotate_over_wires(struct fpga_model* model, int y, int x,
	int* wires, int num_wires, int early_decrement)
{
	int i, rc;

	//
	// We loop over the wires times 4 because each wire will
	// be processed at NN, EE, SS and WW.
	//
	// i/4        position in the wire array
	// 3-(i/4)%4  num of wire 0:3 for current element in the wire array
	// i%4        NN (0) - EE (1) - SS (2) - WW (3)
	//

	for (i = 0; i < num_wires*4; i++) {
		rc = add_logicin_switch_quart(model, y, x, FIRST_LEN2+(i%4)*2,
			3-((i+early_decrement)/4)%4, wires[i/4]);
		if (rc) goto xout;
	}
	return 0;
xout:
	return rc;
}

int add_logicin_switches(struct fpga_model* model, int y, int x)
{
	int rc;
	{ static int decrement_at_NN[] =
		{ M_DI | LWF_SOUTH0, M_CI, X_CE, M_WE,
		  M_B1 | LWF_SOUTH0, X_A2, X_A1, M_B2,
		  M_C6 | LWF_SOUTH0, M_C5, M_C4, M_C3,
		  X_D6 | LWF_SOUTH0, X_D5, X_D4, X_D3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_NN,
		sizeof(decrement_at_NN)/sizeof(decrement_at_NN[0]),
		0 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_EE[] =
		{ M_CX, X_BX, M_AX, X_DX | LWF_SOUTH0,
		  M_D2, M_D1, X_C2, X_C1 | LWF_SOUTH0,
		  M_A4, M_A5, M_A6, M_A3 | LWF_SOUTH0,
		  X_B4, X_B5, X_B6, X_B3 | LWF_SOUTH0 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_EE,
		sizeof(decrement_at_EE)/sizeof(decrement_at_EE[0]),
		3 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_SS[] =
		{ FAN_B, M_CE, M_BI, M_AI | LWF_NORTH3,
		   X_B2, M_A1, M_A2, X_B1 | LWF_NORTH3,
		   X_C6, X_C5, X_C4, X_C3 | LWF_NORTH3,
		   M_D6, M_D5, M_D4, M_D3 | LWF_NORTH3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_SS,
		sizeof(decrement_at_SS)/sizeof(decrement_at_SS[0]),
		2 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_WW[] =
		{ M_DX, X_CX, M_BX, X_AX | LWF_NORTH3,
		  M_C2, X_D1, X_D2, M_C1 | LWF_NORTH3,
		  X_A3, X_A4, X_A5, X_A6 | LWF_NORTH3,
		  M_B3, M_B4, M_B5, M_B6 | LWF_NORTH3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_WW,
		sizeof(decrement_at_WW)/sizeof(decrement_at_WW[0]),
		1 /* early_decrement */);
	if (rc) goto xout; }
	return 0;
xout:
	return rc;
}

static int init_routing_switches(struct fpga_model* model);

static int init_switches(struct fpga_model* model)
{
	int rc;

	rc = init_routing_switches(model);
	if (rc) goto xout;
// todo: IO_B, IO_TERM_B, IO_LOGIC_TERM_B, IO_OUTER_B, IO_INNER_B, LOGIC_XM
	return 0;
xout:
	return rc;
}

static int init_routing_switches(struct fpga_model* model)
{
	int x, y, i, j, routing_io, rc;
	struct set_of_switches dir_EB_switches;
	enum wire_type wire;
	struct fpga_tile* tile;
	const char* gfan_s, *gclk_s;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y))
				continue;
			tile = YX_TILE(model, y, x);
			routing_io = (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L);
			gfan_s = routing_io ? "INT_IOI_GFAN%i" : "GFAN%i";

			// GND
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, x, "GND_WIRE",
					pf(gfan_s, i), 0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, y, x, "GND_WIRE", "SR1",
				0 /* bidir */);
			if (rc) goto xout;

			// VCC
		 	{ int vcc_dest[] = {
				X_A3, X_A4, X_A5, X_A6, X_B3, X_B4, X_B5, X_B6,
				X_C3, X_C4, X_C5, X_C6, X_D3, X_D4, X_D5, X_D6,
				M_A3, M_A4, M_A5, M_A6, M_B3, M_B4, M_B5, M_B6,
				M_C3, M_C4, M_C5, M_C6, M_D3, M_D4, M_D5, M_D6 };

			for (i = 0; i < sizeof(vcc_dest)/sizeof(vcc_dest[0]); i++) {
				rc = add_switch(model, y, x, "VCC_WIRE",
					logicin_s(vcc_dest[i], routing_io),
					0 /* bidir */);
				if (rc) goto xout;
			}}

			// KEEP1
			for (i = X_A1; i <= M_WE; i++) {
				rc = add_switch(model, y, x, "KEEP1_WIRE",
					logicin_s(i, routing_io),
					0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, y, x, "KEEP1_WIRE", "FAN_B",
				0 /* bidir */);
			if (rc) goto xout;

			// VCC and KEEP1 to CLK0:1, SR0:1, GFAN0:1
			{ static const char* src[] = {"VCC_WIRE", "KEEP1_WIRE"};
			for (i = 0; i <= 1; i++)
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, src[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, src[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x,
						src[i],
						pf(gfan_s, j),
						0 /* bidir */);
					if (rc) goto xout;
				}
			}

			// GCLK0:15 -> CLK0:1, GFAN0:1/SR0:1
			if (tile->type == ROUTING_BRK
			    || tile->type == BRAM_ROUTING_BRK)
				gclk_s = "GCLK%i_BRK";
			else
				gclk_s = "GCLK%i";
			for (i = 0; i <= 15; i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf(gclk_s, i),
						pf("CLK%i", j),
						0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x,
						pf(gclk_s, i),
						(i < 8)
						  ? pf(gfan_s, j)
						  : pf("SR%i", j),
						0 /* bidir */);
					if (rc) goto xout;
				}
			}

			// FAN_B to SR0:1
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, x, "FAN_B",
					pf("SR%i", i), 0 /* bidir */);
				if (rc) goto xout;
			}

			// some logicin wires are singled out
			{ int logic_singles[] = {X_CE, X_CX, X_DX,
				M_AI, M_BI, M_CX, M_DX, M_WE};
			for (i = 0; i < sizeof(logic_singles)/sizeof(logic_singles[0]); i++) {
				rc = add_switch(model, y, x, pf("LOGICIN_B%i", logic_singles[i]),
					pf("LOGICIN%i", logic_singles[i]), 0 /* bidir */);
				if (rc) goto xout;
			}}

			// connecting directional wires endpoints to logicin
			rc = add_logicin_switches(model, y, x);
			if (rc) goto xout;

			// connecting logicout back to directional wires
			// beginning points (and some back to logicin)
			rc = add_logicout_switches(model, y, x, routing_io);
			if (rc) goto xout;

			// there are extra wires to send signals to logicin, or
			// to share/multiply logicin signals
			rc = add_logicio_extra(model, y, x, routing_io);
			if (rc) goto xout;

			// extra wires going to SR, CLK and GFAN
			{ int to_sr[] = {X_BX, M_BX, M_DI};
			for (i = 0; i < sizeof(to_sr)/sizeof(to_sr[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_sr[i]),
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ int to_clk[] = {M_BX, M_CI};
			for (i = 0; i < sizeof(to_clk)/sizeof(to_clk[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_clk[i]),
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ int to_gf[] = {M_AX, X_AX, M_CE, M_CI};
			for (i = 0; i < sizeof(to_gf)/sizeof(to_gf[0]); i++) {
				for (j = 0; j <= 1; j++) {
					int bidir = !routing_io
					  && ((!j && i < 2) || (j && i >= 2));
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_gf[i]),
						pf(gfan_s, j), bidir);
					if (rc) goto xout;
				}
			}}

			// connecting the directional wires from one's end
			// to another one's beginning
			wire = W_NN2;
			do {
				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN1(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN2(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN4(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				wire = W_CLOCKWISE(wire);
			} while (wire != W_NN2); // one full turn

			// and finally, some end wires go to CLK, SR and GFAN
			{ static const char* from[] = {"NR1E2", "WR1E2"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, from[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"ER1E1", "SR1E1"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, from[i],
						pf(gfan_s, j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"NR1E1", "WR1E1"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf(gfan_s, j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"ER1E2", "SR1E2"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
		}
	}
	return 0;
xout:
	return rc;
}

static int init_devices(struct fpga_model* model)
{
	int x, y, i, j;
	struct fpga_tile* tile;

	// DCM, PLL_ADV
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs
		tile = YX_TILE(model, y-1, model->center_x-CENTER_CMTPLL_O);
		if (i%2) {
			tile->devices[tile->num_devices++].type = DEV_DCM;
			tile->devices[tile->num_devices++].type = DEV_DCM;
		} else
			tile->devices[tile->num_devices++].type = DEV_PLL_ADV;
	}

	// BSCAN
	tile = YX_TILE(model, TOP_IO_TILES, model->x_width-RIGHT_IO_DEVS_O);
	tile->devices[tile->num_devices++].type = DEV_BSCAN;
	tile->devices[tile->num_devices++].type = DEV_BSCAN;

	// BSCAN, OCT_CALIBRATE
	tile = YX_TILE(model, TOP_IO_TILES+1, model->x_width-RIGHT_IO_DEVS_O);
	tile->devices[tile->num_devices++].type = DEV_BSCAN;
	tile->devices[tile->num_devices++].type = DEV_BSCAN;
	tile->devices[tile->num_devices++].type = DEV_OCT_CALIBRATE;

	// ICAP, SPI_ACCESS, OCT_CALIBRATE
	tile = YX_TILE(model, model->y_height-BOT_IO_TILES-1,
		model->x_width-RIGHT_IO_DEVS_O);
	tile->devices[tile->num_devices++].type = DEV_ICAP;
	tile->devices[tile->num_devices++].type = DEV_SPI_ACCESS;
	tile->devices[tile->num_devices++].type = DEV_OCT_CALIBRATE;

	// STARTUP, POST_CRC_INTERNAL, SLAVE_SPI, SUSPEND_SYNC
	tile = YX_TILE(model, model->y_height-BOT_IO_TILES-2,
		model->x_width-RIGHT_IO_DEVS_O);
	tile->devices[tile->num_devices++].type = DEV_STARTUP;
	tile->devices[tile->num_devices++].type = DEV_POST_CRC_INTERNAL;
	tile->devices[tile->num_devices++].type = DEV_SLAVE_SPI;
	tile->devices[tile->num_devices++].type = DEV_SUSPEND_SYNC;

	// BUFGMUX
	tile = YX_TILE(model, model->center_y, model->center_x);
	for (i = 0; i < 16; i++)
		tile->devices[tile->num_devices++].type = DEV_BUFGMUX;

	// BUFIO, BUFIO_FB, BUFPLL, BUFPLL_MCB
	tile = YX_TILE(model, TOP_OUTER_ROW, model->center_x-CENTER_CMTPLL_O);
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL_MCB;
	for (j = 0; j < 8; j++) {
		tile->devices[tile->num_devices++].type = DEV_BUFIO;
		tile->devices[tile->num_devices++].type = DEV_BUFIO_FB;
	}
	tile = YX_TILE(model, model->center_y, LEFT_OUTER_COL);
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL_MCB;
	for (j = 0; j < 8; j++) {
		tile->devices[tile->num_devices++].type = DEV_BUFIO;
		tile->devices[tile->num_devices++].type = DEV_BUFIO_FB;
	}
	tile = YX_TILE(model, model->center_y, model->x_width - RIGHT_OUTER_O);
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL_MCB;
	for (j = 0; j < 8; j++) {
		tile->devices[tile->num_devices++].type = DEV_BUFIO;
		tile->devices[tile->num_devices++].type = DEV_BUFIO_FB;
	}
	tile = YX_TILE(model, model->y_height - BOT_OUTER_ROW, model->center_x-CENTER_CMTPLL_O);
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL;
	tile->devices[tile->num_devices++].type = DEV_BUFPLL_MCB;
	for (j = 0; j < 8; j++) {
		tile->devices[tile->num_devices++].type = DEV_BUFIO;
		tile->devices[tile->num_devices++].type = DEV_BUFIO_FB;
	}
	
	// BUFH
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs
		tile = YX_TILE(model, y, model->center_x);
		for (j = 0; j < 32; j++)
			tile->devices[tile->num_devices++].type = DEV_BUFH;
	}

	// BRAM
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				tile = YX_TILE(model, y, x);
				if (tile->flags & TF_BRAM_DEV) {
					tile->devices[tile->num_devices++].type = DEV_BRAM16;
					tile->devices[tile->num_devices++].type = DEV_BRAM8;
					tile->devices[tile->num_devices++].type = DEV_BRAM8;
				}
			}
		}
	}

	// MACC
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				tile = YX_TILE(model, y, x);
				if (tile->flags & TF_MACC_DEV)
					tile->devices[tile->num_devices++].type = DEV_MACC;
			}
		}
	}

	// ILOGIC/OLOGIC/IODELAY
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_LOGIC_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x-1)) {
			for (i = 0; i <= 1; i++) {
				tile = YX_TILE(model, TOP_IO_TILES+i, x);
				for (j = 0; j <= 1; j++) {
					tile->devices[tile->num_devices++].type = DEV_ILOGIC;
					tile->devices[tile->num_devices++].type = DEV_OLOGIC;
					tile->devices[tile->num_devices++].type = DEV_IODELAY;
				}
				tile = YX_TILE(model, model->y_height-BOT_IO_TILES-i-1, x);
				for (j = 0; j <= 1; j++) {
					tile->devices[tile->num_devices++].type = DEV_ILOGIC;
					tile->devices[tile->num_devices++].type = DEV_OLOGIC;
					tile->devices[tile->num_devices++].type = DEV_IODELAY;
				}
			}
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_LEFT_WIRED, model, y)) {
			tile = YX_TILE(model, y, LEFT_IO_DEVS);
			for (j = 0; j <= 1; j++) {
				tile->devices[tile->num_devices++].type = DEV_ILOGIC;
				tile->devices[tile->num_devices++].type = DEV_OLOGIC;
				tile->devices[tile->num_devices++].type = DEV_IODELAY;
			}
		}
		if (is_aty(Y_RIGHT_WIRED, model, y)) {
			tile = YX_TILE(model, y, model->x_width-RIGHT_IO_DEVS_O);
			for (j = 0; j <= 1; j++) {
				tile->devices[tile->num_devices++].type = DEV_ILOGIC;
				tile->devices[tile->num_devices++].type = DEV_OLOGIC;
				tile->devices[tile->num_devices++].type = DEV_IODELAY;
			}
		}
	}

	// IOB
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_OUTER_LEFT, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_LEFT_WIRED, model, y)) {
					tile = YX_TILE(model, y, x);
					tile->devices[tile->num_devices++].type = DEV_IOBM;
					tile->devices[tile->num_devices++].type = DEV_IOBS;
				}
			}
		}
		if (is_atx(X_OUTER_RIGHT, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_RIGHT_WIRED, model, y)) {
					tile = YX_TILE(model, y, x);
					tile->devices[tile->num_devices++].type = DEV_IOBM;
					tile->devices[tile->num_devices++].type = DEV_IOBS;
				}
			}
		}
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x)) {
			tile = YX_TILE(model, TOP_OUTER_ROW, x);
			tile->devices[tile->num_devices++].type = DEV_IOBM;
			tile->devices[tile->num_devices++].type = DEV_IOBM;
			tile->devices[tile->num_devices++].type = DEV_IOBS;
			tile->devices[tile->num_devices++].type = DEV_IOBS;

			tile = YX_TILE(model, model->y_height-BOT_OUTER_ROW, x);
			tile->devices[tile->num_devices++].type = DEV_IOBM;
			tile->devices[tile->num_devices++].type = DEV_IOBM;
			tile->devices[tile->num_devices++].type = DEV_IOBS;
			tile->devices[tile->num_devices++].type = DEV_IOBS;
		}
	}

	// TIEOFF
	tile = YX_TILE(model, model->center_y, LEFT_OUTER_COL);
	tile->devices[tile->num_devices++].type = DEV_TIEOFF;
	tile = YX_TILE(model, model->center_y, model->x_width-RIGHT_OUTER_O);
	tile->devices[tile->num_devices++].type = DEV_TIEOFF;
	tile = YX_TILE(model, TOP_OUTER_ROW, model->center_x-1);
	tile->devices[tile->num_devices++].type = DEV_TIEOFF;
	tile = YX_TILE(model, model->y_height-BOT_OUTER_ROW, model->center_x-CENTER_CMTPLL_O);
	tile->devices[tile->num_devices++].type = DEV_TIEOFF;

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_LEFT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_LEFT_WIRED, model, y)) {
					tile = YX_TILE(model, y, x);
					tile->devices[tile->num_devices++].type = DEV_TIEOFF;
				}
			}
		}
		if (is_atx(X_RIGHT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_RIGHT_WIRED, model, y)) {
					tile = YX_TILE(model, y, x);
					tile->devices[tile->num_devices++].type = DEV_TIEOFF;
				}
			}
		}
		if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				tile = YX_TILE(model, y, x);
				if (tile->flags & TF_PLL_DEV)
					tile->devices[tile->num_devices++].type = DEV_TIEOFF;
				
			}
		}
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				tile = YX_TILE(model, y, x);
				tile->devices[tile->num_devices++].type = DEV_TIEOFF;
			}
		}
		if (is_atx(X_LOGIC_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x-1)) {
			for (i = 0; i <= 1; i++) {
				tile = YX_TILE(model, TOP_IO_TILES+i, x);
				tile->devices[tile->num_devices++].type = DEV_TIEOFF;
				tile = YX_TILE(model, model->y_height-BOT_IO_TILES-i-1, x);
				tile->devices[tile->num_devices++].type = DEV_TIEOFF;
			}
		}
	}
	// LOGIC
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_LOGIC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				tile = YX_TILE(model, y, x);
				if (tile->flags & TF_LOGIC_XM_DEV) {
					tile->devices[tile->num_devices++].type = DEV_LOGIC_X;
					tile->devices[tile->num_devices++].type = DEV_LOGIC_M;
				}
				if (tile->flags & TF_LOGIC_XL_DEV) {
					tile->devices[tile->num_devices++].type = DEV_LOGIC_X;
					tile->devices[tile->num_devices++].type = DEV_LOGIC_L;
				}
			}
		}
	}
	return 0;
}

static int add_io_connpts(struct fpga_model* model, int y, int x, const char* prefix, int num_devs)
{
	int i, rc;

	for (i = 0; i < num_devs; i++) {
		rc = add_connpt_name(model, y, x, pf("%s_O%i_PINW", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_IBUF%i_PINW", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_T%i_PINW", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_PADOUT%i", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_DIFFI_IN%i", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_DIFFO_IN%i", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_DIFFO_OUT%i", prefix, i));
		if (rc) goto xout;
		rc = add_connpt_name(model, y, x, pf("%s_PCI_RDY%i", prefix, i));
		if (rc) goto xout;
	}
	return 0;
xout:
	return rc;
}

enum which_side
{
	TOP_S, BOTTOM_S, RIGHT_S, LEFT_S
};

static int init_iologic_ports(struct fpga_model* model, int y, int x, enum which_side side)
{
	static const char* prefix, *suffix1, *suffix2;
	int rc, i;

	switch (side) {
		case TOP_S: prefix = "TIOI"; break;
		case BOTTOM_S: prefix = "BIOI"; break;
		case LEFT_S: prefix = "LIOI"; break;
		case RIGHT_S: prefix = "RIOI"; break;
		default: ABORT(1);
	}
	if (side == LEFT_S || side == RIGHT_S) {
		suffix1 = "_M";
		suffix2 = "_S";
	} else {
		suffix1 = "_STUB";
		suffix2 = "_S_STUB";
	}

	for (i = X_A /* 0 */; i <= M_DQ /* 23 */; i++) {
		rc = add_connpt_name(model, y, x, pf("IOI_INTER_LOGICOUT%i", i));
		if (rc) goto xout;
	}
	rc = add_connpt_name(model, y, x, pf("%s_GND_TIEOFF", prefix));
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, pf("%s_VCC_TIEOFF", prefix));
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, pf("%s_KEEP1_STUB", prefix));
	if (rc) goto xout;
	for (i = 0; i <= 4; i++) {
			rc = add_connpt_2(model, y, x, pf("AUXADDR%i_IODELAY", i), suffix1, suffix2);
		if (rc) goto xout;
	}
	rc = add_connpt_2(model, y, x, "AUXSDOIN_IODELAY", suffix1, suffix2);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "AUXSDO_IODELAY", suffix1, suffix2);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "MEMUPDATE_IODELAY", suffix1, suffix2);
	if (rc) goto xout;

	rc = add_connpt_name(model, y, x, "OUTN_IODELAY_SITE");
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "STUB_OUTN_IODELAY_S");
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "OUTP_IODELAY_SITE");
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "STUB_OUTP_IODELAY_S");
	if (rc) goto xout;

	for (i = 1; i <= 4; i++) {
		rc = add_connpt_2(model, y, x, pf("Q%i_ILOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("D%i_OLOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("T%i_OLOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("SHIFTIN%i_OLOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("SHIFTOUT%i_OLOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
	}
	for (i = 0; i <= 1; i++) {
		rc = add_connpt_2(model, y, x, pf("CFB%i_ILOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("CLK%i_ILOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("CLK%i_OLOGIC_SITE", i), "", "_S");
		if (rc) goto xout;
	}
	{
		static const char* mcb_2[] = {
			"BITSLIP_ILOGIC_SITE", "BUSY_IODELAY_SITE",
			"CAL_IODELAY_SITE", "CE0_ILOGIC_SITE",
			"CE_IODELAY_SITE", "CIN_IODELAY_SITE",
			"CLKDIV_ILOGIC_SITE", "CLKDIV_OLOGIC_SITE",
			"CLK_IODELAY_SITE", "DATAOUT_IODELAY_SITE",
			"DDLY2_ILOGIC_SITE", "DDLY_ILOGIC_SITE",
			"DFB_ILOGIC_SITE", "D_ILOGIC_IDATAIN_IODELAY",
			"D_ILOGIC_SITE", "DOUT_IODELAY_SITE",
			"FABRICOUT_ILOGIC_SITE", "IDATAIN_IODELAY_SITE",
			"INCDEC_ILOGIC_SITE", "INC_IODELAY_SITE",
			"IOCE_ILOGIC_SITE", "IOCE_OLOGIC_SITE",
			"IOCLK1_IODELAY_SITE", "IOCLK_IODELAY_SITE",
			"LOAD_IODELAY_SITE", "OCE_OLOGIC_SITE",
			"ODATAIN_IODELAY_SITE", "OFB_ILOGIC_SITE",
			"OQ_OLOGIC_SITE", "RCLK_IODELAY_SITE",
			"READEN_IODELAY_UNUSED_SITE", "REV_ILOGIC_SITE",
			"REV_OLOGIC_SITE", "RST_IODELAY_SITE",
			"SHIFTIN_ILOGIC_SITE", "SHIFTOUT_ILOGIC_SITE",
			"SR_ILOGIC_SITE", "SR_OLOGIC_SITE",
			"TCE_OLOGIC_SITE", "TFB_ILOGIC_SITE",
			"T_IODELAY_SITE", "TOUT_IODELAY_SITE",
			"TQ_OLOGIC_SITE", "TRAIN_OLOGIC_SITE",
			"VALID_ILOGIC_SITE", "" };

		for (i = 0; mcb_2[i][0]; i++) {
			rc = add_connpt_2(model, y, x, mcb_2[i], "", "_S");
		}
	}
	rc = add_connpt_name(model, y, x, "DATAOUT2_IODELAY_SITE");
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "DATAOUT2_IODELAY2_SITE_S");
	if (rc) goto xout;

	for (i = 0; i <= 2; i++) {
		rc = add_connpt_2(model, y, x, pf("IOI_CLK%iINTER", i),
			"_M", "_S");
		if (rc) goto xout;
	}
	for (i = 0; i <= 1; i++) {
		rc = add_connpt_2(model, y, x, pf("IOI_CLKDIST_IOCE%i", i),
			"_M", "_S");
		if (rc) goto xout;
	}
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK0_ILOGIC", "_M", "_S");
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK0_OLOGIC", "_M", "_S");
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK1", "_M", "_S");
	if (rc) goto xout;

	if (side == TOP_S || side == BOTTOM_S) {
		static const char* mcb_2[] = {
			"IOI_MCB_DQIEN", "IOI_MCB_INBYP",
			"IOI_MCB_IN", "IOI_MCB_OUTN",
			"IOI_MCB_OUTP", "" };
		static const char* mcb_1[] = {
			"IOI_MCB_DRPADD", "IOI_MCB_DRPBROADCAST",
			"IOI_MCB_DRPCLK", "IOI_MCB_DRPCS",
			"IOI_MCB_DRPSDI", "IOI_MCB_DRPSDO",
			"IOI_MCB_DRPTRAIN", "" };

		for (i = 0; mcb_2[i][0]; i++) {
			rc = add_connpt_2(model, y, x, mcb_2[i], "_M", "_S");
			if (rc) goto xout;
		}
		for (i = 0; mcb_1[i][0]; i++) {
			rc = add_connpt_name(model, y, x, mcb_1[i]);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int init_ports(struct fpga_model* model)
{
	int x, y, i, j, k, row_num, row_pos, rc;

	// inner and outer IO tiles (ILOGIC/ILOGIC/IODELAY)
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (has_device(model, TOP_OUTER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, TOP_OUTER_IO, x, TOP_S);
			if (rc) goto xout;
		}
		if (has_device(model, TOP_INNER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, TOP_INNER_IO, x, TOP_S);
			if (rc) goto xout;
		}
		if (has_device(model, model->y_height - BOT_INNER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, model->y_height - BOT_INNER_IO, x, BOTTOM_S);
			if (rc) goto xout;
		}
		if (has_device(model, model->y_height - BOT_OUTER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, model->y_height - BOT_OUTER_IO, x, BOTTOM_S);
			if (rc) goto xout;
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, y, LEFT_IO_DEVS, LEFT_S);
			if (rc) goto xout;
		}
		if (has_device(model, y, model->x_width - RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, y, model->x_width - RIGHT_IO_DEVS_O, RIGHT_S);
			if (rc) goto xout;
		}
	}

	// IO tiles
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (YX_TILE(model, 0, x)->type == IO_T) {
			rc = add_io_connpts(model, 0 /* y */, x, "TIOB",
				4 /* num_devs */);
			if (rc) goto xout;
		}
		if (YX_TILE(model, model->y_height - BOT_OUTER_ROW, x)->type == IO_B) {
			rc = add_io_connpts(model, model->y_height
				- BOT_OUTER_ROW, x, "BIOB", 4 /* num_devs */);
			if (rc) goto xout;
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (YX_TILE(model, y, 0)->type == IO_L) {
			rc = add_io_connpts(model, y, 0 /* x */, "LIOB",
				2 /* num_devs */);
			if (rc) goto xout;
		}
		if (YX_TILE(model, y, model->x_width - RIGHT_OUTER_O)->type == IO_R) {
			rc = add_io_connpts(model, y, model->x_width
				- RIGHT_OUTER_O, "RIOB", 2 /* num_devs */);
			if (rc) goto xout;
		}
	}

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				rc = add_connpt_name(model, y, x, "VCC_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "GND_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "KEEP1_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN_B");
				if (rc) goto xout;

				if (!is_atyx(YX_IO_ROUTING, model, y, x)) {
					for (i = 0; i <= 1; i++) {
						rc = add_connpt_name(model, y, x, pf("GFAN%i", i));
						if (rc) goto xout;
					}
				} else {
					if (!is_atx(X_CENTER_ROUTING_COL, model, x)
					    || is_aty(Y_TOPBOT_IO_RANGE, model, y)) {
						// In the center those 2 wires are connected
						// to the PLL, but elsewhere? Not clear what they
						// connect to...
						rc = add_connpt_name(model, y, x,
							logicin_s(X_A5, 1 /* routing_io */));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x,
							logicin_s(X_B4, 1 /* routing_io */));
						if (rc) goto xout;
					}
				}
			}
		}
		if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_BRAM_DEV) {

					static const char* pass_str[3] = {"RAMB16BWER", "RAMB8BWER_0", "RAMB8BWER_1"};
					// pass 0 is ramb16, pass 1 and 2 are for ramb8
					for (i = 0; i <= 2; i++) {
						for (j = 'A'; j <= 'B'; j++) {
							rc = add_connpt_name(model, y, x, pf("%s_CLK%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_EN%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_REGCE%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_RST%c", pass_str[i], j));
							if (rc) goto xout;
							for (k = 0; k <= (!i ? 3 : 1); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DIP%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DOP%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_WE%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 13 : 12); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_ADDR%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 31 : 15); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DI%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DO%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
						}
					}
				}
			}
		}
		if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_MACC_DEV) {
					static const char* pref[] = {"CE", "RST", ""};
					static const char* seq[] = {"A", "B", "C", "D", "M", "P", "OPMODE", ""};

					is_in_row(model, y, &row_num, &row_pos);
					if (!row_num && row_pos == LAST_POS_IN_ROW) {
						rc = add_connpt_name(model, y, x, "CARRYIN_DSP48A1_SITE");
						if (rc) goto xout;
						for (i = 0; i <= 47; i++) {
							rc = add_connpt_name(model, y, x, pf("PCIN%i_DSP48A1_SITE", i));
							if (rc) goto xout;
						}
					}

					rc = add_connpt_name(model, y, x, "CLK_DSP48A1_SITE");
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUT_DSP48A1_SITE");
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUTF_DSP48A1_SITE");
					if (rc) goto xout;

					for (i = 0; pref[i][0]; i++) {
						rc = add_connpt_name(model, y, x, pf("%sCARRYIN_DSP48A1_SITE", pref[i]));
						if (rc) goto xout;
						for (j = 0; seq[j][0]; j++) {
							rc = add_connpt_name(model, y, x, pf("%s%s_DSP48A1_SITE", pref[i], seq[j]));
							if (rc) goto xout;
						}
					}
						
					for (i = 0; i <= 17; i++) {
						rc = add_connpt_name(model, y, x, pf("A%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("B%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("D%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("BCOUT%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 47; i++) {
						rc = add_connpt_name(model, y, x, pf("C%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("P%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("PCOUT%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 35; i++) {
						rc = add_connpt_name(model, y, x, pf("M%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 7; i++) {
						rc = add_connpt_name(model, y, x, pf("OPMODE%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
				}
			}
		}
		if (is_atx(X_LOGIC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & (TF_LOGIC_XM_DEV|TF_LOGIC_XL_DEV)) {
					const char* pref[2];

					if (YX_TILE(model, y, x)->flags & TF_LOGIC_XM_DEV) {
						// The first SLICEM on the bottom has a given C_IN port.
						if (is_aty(Y_INNER_BOTTOM, model, y+3)) {
							rc = add_connpt_name(model, y, x, "M_CIN");
							if (rc) goto xout;
						}
						rc = add_connpt_name(model, y, x, "M_COUT");
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, "M_WE");
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							rc = add_connpt_name(model, y, x, pf("M_%cI", i));
							if (rc) goto xout;
						}
						pref[0] = "M";
						pref[1] = "X";
					} else { // LOGIC_XL
						rc = add_connpt_name(model, y, x, "XL_COUT");
						if (rc) goto xout;
						pref[0] = "L";
						pref[1] = "XX";
					}
					for (k = 0; k <= 1; k++) {
						rc = add_connpt_name(model, y, x, pf("%s_CE", pref[k], i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_SR", pref[k], i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_CLK", pref[k], i));
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							for (j = 1; j <= 6; j++) {
								rc = add_connpt_name(model, y, x, pf("%s_%c%i", pref[k], i, j));
								if (rc) goto xout;
							}
							rc = add_connpt_name(model, y, x, pf("%s_%c", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cMUX", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cQ", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cX", pref[k], i));
							if (rc) goto xout;
						}
					}
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int init_wires(struct fpga_model* model)
{
	int rc;

	rc = run_io_wires(model);
	if (rc) goto xout;
return 0;

	rc = run_direction_wires(model);
	if (rc) goto xout;

	rc = run_logic_inout(model);
	if (rc) goto xout;

	rc = run_gclk(model);
	if (rc) goto xout;

	return 0;
xout:
	return rc;
}

int run_io_wires(struct fpga_model* model)
{
	static const char* s[] = { "IBUF", "O", "T", "" };
	int x, y, i, rc;

	//
	// 1. input wires from IBUF into the chip "IBUF"
	// 2. output wires from the chip into O "O"
	// 3. T wires "T"
	//

	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {

		y = 0;
		if (has_device(model, y, x, DEV_IOBM)) {
			for (i = 0; s[i][0]; i++) {
				struct w_net net1 = {
					1,
					{{ pf("TIOB_%s%%i",		s[i]), 0,   y,   x },
					 { pf("IOI_TTERM_IOIUP_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIUP_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i",	s[i]), 0, y+2, x+1 },
					 { "" }}};
				struct w_net net2 = {
					1,
					{{ pf("TIOB_%s%%i",		s[i]), 2,   y,   x },
					 { pf("IOI_TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i_EXT",	s[i]), 0, y+2, x+1 },
					 { pf("TIOI_INNER_%s%%i",	s[i]), 0, y+3, x+1 },
					 { "" }}};

				if ((rc = add_conn_net(model, NOPREF_BI_F, &net1))) goto xout;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net2))) goto xout;
			}
		}

		y = model->y_height - BOT_OUTER_ROW;
		if (has_device(model, y, x, DEV_IOBM)) {
			for (i = 0; s[i][0]; i++) {
				struct w_net net1 = {
					1,
					{{ pf("BIOB_%s%%i",		s[i]), 0,   y,   x },
					 { pf("IOI_BTERM_IOIUP_%s%%i",	s[i]), 0, y-1,   x },
					 { pf("BTERM_IOIUP_%s%%i",	s[i]), 0, y-1, x+1 },
					 { pf("BIOI_OUTER_%s%%i_EXT",	s[i]), 0, y-2, x+1 },
					 { pf("BIOI_INNER_%s%%i",	s[i]), 0, y-3, x+1 },
					 { "" }}};

				if ((rc = add_conn_net(model, NOPREF_BI_F, &net1))) goto xout;
 				// The following is actually a net, but add_conn_net()/w_net
 				// does not support COUNT_DOWN ranges right now.
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-1, x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1,   x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 0);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1,   x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 0);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 0);
				if (rc) goto xout;
			}
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC)) {
			x = 0;
			for (i = 0; s[i][0]; i++) {
				struct w_net net = {
					1,
					{{ pf("LIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("LTERM_IOB_%s%%i",	s[i]), 0, y, x+1 },
					 { pf("LIOI_INT_%s%%i",		s[i]), 0, y, x+2 },
					 { pf("LIOI_IOB_%s%%i",		s[i]), 0, y, x+3 },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
		if (has_device(model, y, model->x_width - RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			x = model->x_width - RIGHT_OUTER_O;
			for (i = 0; s[i][0]; i++) {
				struct w_net net = {
					1,
					{{ pf("RIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("RTERM_IOB_%s%%i",	s[i]), 0, y, x-1 },
					 { pf("MCB_%s%%i",		s[i]), 0, y, x-2 },
					 { pf("RIOI_IOB_%s%%i",		s[i]), 0, y, x-3 },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int run_gclk(struct fpga_model* model)
{
	int x, i, rc, row, row_top_y, is_break, next_net_o;
	struct w_net gclk_net;

	for (row = model->cfg_rows-1; row >= 0; row--) {
		row_top_y = TOP_IO_TILES + (model->cfg_rows-1-row)*(8+1/* middle of row */+8);
		if (row < (model->cfg_rows/2)) row_top_y++; // center regs

		// net that connects the hclk of half the chip together horizontally
		gclk_net.last_inc = 15;
		next_net_o = 0;
		for (x = LEFT_IO_ROUTING;; x++) {
			if (next_net_o+2 > sizeof(gclk_net.pts) / sizeof(gclk_net.pts[0])) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
			gclk_net.pts[next_net_o].start_count = 0;
			gclk_net.pts[next_net_o].x = x;
			gclk_net.pts[next_net_o].y = row_top_y+8;

			if (is_atx(X_LEFT_IO_ROUTING_COL|X_FABRIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_INT";
			} else if (is_atx(X_LEFT_MCB, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_MCB";
			} else if (is_atx(X_LOGIC_COL|X_LEFT_IO_DEVS_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_CLB";
			} else if (is_atx(X_FABRIC_BRAM_MACC_ROUTING_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_BRAM_INTER";
			} else if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_BRAM";
			} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_DSP";
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				gclk_net.pts[next_net_o].y = row_top_y+7;
				gclk_net.pts[next_net_o++].name = "HCLK_CMT_GCLK%i_CLB";
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "CLKV_BUFH_LEFT_L%i";

				// connect left half
				gclk_net.pts[next_net_o].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;

				// start right half
				gclk_net.pts[0].start_count = 0;
				gclk_net.pts[0].x = x;
				gclk_net.pts[0].y = row_top_y+8;
				gclk_net.pts[0].name = "CLKV_BUFH_RIGHT_R%i";
				next_net_o = 1;

			} else if (is_atx(X_RIGHT_IO_ROUTING_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_INT";

				// connect right half
				gclk_net.pts[next_net_o].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;
				break;
			}
			if (x >= model->x_width) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
		}
	}
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (row = model->cfg_rows-1; row >= 0; row--) {
				row_top_y = 2 /* top IO */ + (model->cfg_rows-1-row)*(8+1/* middle of row */+8);
				if (row < (model->cfg_rows/2)) row_top_y++; // center regs

				is_break = 0;
 				if (is_atx(X_LEFT_IO_ROUTING_COL|X_RIGHT_IO_ROUTING_COL, model, x)) {
					if (row && row != model->cfg_rows/2)
						is_break = 1;
				} else {
					if (row)
						is_break = 1;
					else if (is_atx(X_ROUTING_TO_BRAM_COL|X_ROUTING_TO_MACC_COL, model, x))
						is_break = 1;
				}

				// vertical net inside row, pulling together 16 gclk
				// wires across top (8 tiles) and bottom (8 tiles) half
				// of the row.
				for (i = 0; i < 8; i++) {
					gclk_net.pts[i].name = "GCLK%i";
					gclk_net.pts[i].start_count = 0;
					gclk_net.pts[i].y = row_top_y+i;
					gclk_net.pts[i].x = x;
				}
				gclk_net.last_inc = 15;
				gclk_net.pts[8].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;
				for (i = 0; i < 8; i++)
					gclk_net.pts[i].y += 9;
				if (is_break)
					gclk_net.pts[7].name = "GCLK%i_BRK";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;

				// vertically connects gclk of each row tile to
				// hclk tile at the middle of the row
				for (i = 0; i < 8; i++) {
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						row_top_y+i, x, "GCLK%i",  0, 15,
						row_top_y+8, x, "HCLK_GCLK_UP%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						row_top_y+9+i, x, (i == 7 && is_break) ? "GCLK%i_BRK" : "GCLK%i", 0, 15,
						row_top_y+8, x, "HCLK_GCLK%i", 0))) goto xout;
				}
			}
		}
	}
	rc = run_gclk_horiz_regs(model);
	if (rc) goto xout;
	rc = run_gclk_vert_regs(model);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

static int run_gclk_horiz_regs(struct fpga_model* model)
{
	int x, i, rc, left_half;
	int gclk_sep_pos, start1, last1, start2;
	char* gclk_sep_str;

	//
	// Run a set of wire strings horizontally through the entire
	// chip from left to right over the center reg row.
	// The wires meet at the middle of each half of the chip on
	// the left and right side, as well as in the center.
	//

	//
	// 1. clk pll 0:1
	//

	{
		struct seed_data clkpll_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLKPLL%i" },
			{ X_INNER_LEFT, 		"REGL_LTERM_CLKPLL%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"INT_CLKPLL%i" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL%i" },
			{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL%i" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_IO_RT%i" },
			{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKPLL_IO_LT%i" },
			{ X_CENTER_REGS_COL, 		"CLKC_PLL_IO_RT%i" },
			{ X_INNER_RIGHT, 		"REGR_RTERM_CLKPLL%i" },
			{ X_OUTER_RIGHT, 		"REGR_CLKPLL%i" },
			{ 0 }};
	
		left_half = 1;
		seed_strx(model, clkpll_seeds);

		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;

			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				model->tmp_str[x] = left_half
				  ? "REGC_CLKPLL_IO_LT%i"
				  : "REGC_CLKPLL_IO_RT%i";
			if (!model->tmp_str[x])
				continue;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], 0, 1,
				model->center_y,
				left_half ? model->left_gclk_sep_x
					: model->right_gclk_sep_x,
				"INT_CLKPLL%i", 0))) goto xout;
			if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				left_half = 0;
				x--; // wire up cmtpll col on right side as well
			}
		}
	}

	//
	// 2. clk pll lock 0:1
	//

	{
		struct seed_data clkpll_lock_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_LOCKED%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_LOCKED%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"INT_CLKPLL_LOCK%i" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL_LOCK%i" },
			{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL_LOCK%i" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_LOCK_RT%i" },
			{ X_CENTER_LOGIC_COL,	 	"REGC_CLECLKPLL_LOCK_LT%i" },
			{ X_CENTER_REGS_COL, 		"CLKC_PLL_LOCK_RT%i" },
			{ X_INNER_RIGHT, 		"REGH_RTERM_LOCKED%i" },
			{ X_OUTER_RIGHT, 		"REGR_LOCKED%i" },
			{ 0 }};
	
		left_half = 1;
		seed_strx(model, clkpll_lock_seeds);

		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;

			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				model->tmp_str[x] = left_half
				  ? "CLK_PLL_LOCK_LT%i"
				  : "CLK_PLL_LOCK_RT%i";
			if (!model->tmp_str[x])
				continue;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], 0, 1,
				model->center_y,
				left_half ? model->left_gclk_sep_x
					: model->right_gclk_sep_x,
				"INT_CLKPLL_LOCK%i", 0))) goto xout;
			if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				left_half = 0;
				x--; // wire up cmtpll col on right side as well
			}
		}
	}

	//
	// 3. clk indirect 0:7
	// 4. clk feedback 0:7
	//

	{
		struct seed_data clk_indirect_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLK_INDIRECT%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_CLKINDIRECT%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"REGH_CLKINDIRECT_LR%i_INT" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"REGH_CLKINDIRECT_LR%i_CLB" },
			{ X_FABRIC_MACC_COL, 		"REGH_CLKINDIRECT_LR%i_DSP" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKINDIRECT_LR%i_INT" },
			{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKINDIRECT_LR%i_CLB" },
			{ X_CENTER_CMTPLL_COL, 		"REGC_CMT_CLKINDIRECT_LR%i" },
			{ X_CENTER_REGS_COL, 		"REGC_CLKINDIRECT_LR%i" },
			{ X_INNER_RIGHT, 		"REGH_RTERM_CLKINDIRECT%i" },
			{ X_OUTER_RIGHT, 		"REGR_CLK_INDIRECT%i" },
			{ 0 }};
		struct seed_data clk_feedback_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLK_FEEDBACK%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_CLKFEEDBACK%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"REGH_CLKFEEDBACK_LR%i_INT" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"REGH_CLKFEEDBACK_LR%i_CLB" },
			{ X_FABRIC_MACC_COL, 		"REGH_CLKFEEDBACK_LR%i_DSP" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKFEEDBACK_LR%i_INT" },
			{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKFEEDBACK_LR%i_CLB" },
			{ X_CENTER_CMTPLL_COL, 		"REGC_CMT_CLKFEEDBACK_LR%i" },
			{ X_CENTER_REGS_COL, 		"REGC_CLKFEEDBACK_LR%i" },
			{ X_INNER_RIGHT, 		"REGH_RTERM_CLKFEEDBACK%i" },
			{ X_OUTER_RIGHT, 		"REGR_CLK_FEEDBACK%i" },
			{ 0 }};
		struct seed_data* seeds[2] = {clk_indirect_seeds, clk_feedback_seeds};
		const char* gclk_sep_str[2] = {"REGH_CLKINDIRECT_LR%i_INT", "REGH_CLKFEEDBACK_LR%i_INT"};

		for (i = 0; i <= 1; i++) { // indirect and feedback
			left_half = 1;
			seed_strx(model, seeds[i]);
			for (x = 0; x < model->x_width; x++) {
				if (x == model->left_gclk_sep_x
				    || x == model->right_gclk_sep_x)
					continue;
				if (!model->tmp_str[x])
					continue;

				if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
					start1 = 8;
					last1 = 15;
					start2 = 0;
				} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					if (left_half) {
						start1 = 8;
						last1 = 15;
						start2 = 0;
					} else {
						start1 = 0;
						last1 = 3;
						start2 = 4;
					}
				} else if (is_atx(X_CENTER_REGS_COL|X_INNER_RIGHT|X_OUTER_RIGHT, model, x)) {
					start1 = 0;
					last1 = 3;
					start2 = 4;
				} else {
					start1 = 0;
					last1 = 7;
					start2 = 0;
				}
	
				if ((rc = add_conn_range(model, NOPREF_BI_F,
					model->center_y, x, model->tmp_str[x], start1, last1,
					model->center_y, left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x,
					gclk_sep_str[i], start2))) goto xout;
				if (last1 == 3) {
					if (start1 || start2 != 4) {
						fprintf(stderr, "Internal error in line %i.\n", __LINE__);
						goto xout;
					}
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						model->center_y, x, model->tmp_str[x], 4, 7,
						model->center_y, left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x,
						gclk_sep_str[i], 0))) goto xout;
				}

				if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					left_half = 0;
					x--; // wire up cmtpll col on right side as well
				}
			}
		}
	}

	//
	// 5. ckpin 0:7
	//

	{
		struct seed_data ckpin_seeds[] = {
			{ X_OUTER_LEFT,			"REGL_CKPIN%i" },
			{ X_INNER_LEFT,			"REGH_LTERM_CKPIN%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL,	"REGH_CKPIN%i_INT" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL,	"REGH_CKPIN%i_CLB" },
			{ X_FABRIC_MACC_COL,		"REGH_CKPIN%i_DSP" },
			{ X_CENTER_ROUTING_COL,		"REGC_INT_CKPIN%i_INT" },
			{ X_CENTER_LOGIC_COL,		"REGC_CLECKPIN%i_CLB" },
			{ X_CENTER_CMTPLL_COL,		"REGC_CMT_CKPIN%i" },
			{ X_CENTER_REGS_COL,		"CLKC_CKLR%i" },
			{ X_INNER_RIGHT,		"REGH_RTERM_CKPIN%i" },
			{ X_OUTER_RIGHT,		"REGR_CKPIN%i" },
			{ 0 }};
	
		left_half = 1;
		seed_strx(model, ckpin_seeds);
		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;
			if (!model->tmp_str[x])
				continue;
	
			if (is_atx(X_CENTER_ROUTING_COL|X_CENTER_LOGIC_COL|X_CENTER_CMTPLL_COL, model, x))
				start1 = 8;
			else if (is_atx(X_CENTER_REGS_COL, model, x))
				start1 = left_half ? 8 : 0;
			else
				start1 = 0;
	
			gclk_sep_pos = left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x;
			gclk_sep_str = ((x > gclk_sep_pos) ^ left_half) ? "REGH_DSP_IN_CKPIN%i" : "REGH_DSP_OUT_CKPIN%i";
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], start1, start1+7,
				model->center_y, gclk_sep_pos,
				gclk_sep_str, 0))) goto xout;
	
			// In this loop we tie around the CENTER_REGS_COL, not the
			// CENTER_CMTPLL_COL as before.
			if (left_half && is_atx(X_CENTER_REGS_COL, model, x)) {
				left_half = 0;
				x--; // wire up center regs col on right side as well
			}
		}
	}
	// some local nets around the center on the left side
	{ struct w_net net = {
		3,
		{{ "REGL_GCLK%i", 0, model->center_y, LEFT_OUTER_COL },
		 { "REGH_LTERM_GCLK%i", 0, model->center_y, LEFT_INNER_COL },
		 { "REGH_IOI_INT_GCLK%i", 0, model->center_y, LEFT_IO_ROUTING },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{
		const char* str[3] = {"REGL_GCLK%i", "REGH_LTERM_GCLK%i", "REGH_IOI_INT_GCLK%i"};
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->center_y-2, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 2, 3,
			model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i_EXT", 2))) goto xout;
		for (x = LEFT_OUTER_COL; x <= LEFT_IO_ROUTING; x++) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 0, 1,
				model->center_y, x, str[x], 0))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i_EXT", 2, 3,
				model->center_y, x, str[x], 2))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-2, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 2, 3,
				model->center_y, x, str[x], 2))) goto xout;
		}
	}
	// and right side
	{ struct w_net net = {
		3,
		{{ "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "MCB_REGH_GCLK%i", 0, model->center_y, model->x_width-RIGHT_MCB_O },
		 { "REGR_GCLK%i", 0, model->center_y, model->x_width-RIGHT_OUTER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{
		const char* str[5] = {"REGH_IOI_INT_GCLK%i", "REGH_RIOI_GCLK%i", "MCB_REGH_GCLK%i", "REGR_RTERM_GCLK%i", "REGR_GCLK%i"};
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->center_y-2, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
			model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2))) goto xout;
		for (x = model->x_width-RIGHT_IO_ROUTING_O; x <= model->x_width-RIGHT_OUTER_O; x++) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2, 3,
				model->center_y, x, str[x-(model->x_width-RIGHT_IO_ROUTING_O)], 2))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-2, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
				model->center_y, x, str[x-(model->x_width-RIGHT_IO_ROUTING_O)], 2))) goto xout;
		}
	}
	{ struct w_net net = {
		1,
		{{ "INT_BUFPLL_GCLK%i", 0, model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_INT_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGH_RTERM_GCLK%i", 0, model->center_y, model->x_width-RIGHT_INNER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	{ struct w_net net = {
		1,
		{{ "REGH_IOI_INT_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGR_RTERM_GCLK%i", 2, model->center_y, model->x_width-RIGHT_INNER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	// the naming is a little messed up here, and the networks are
	// actually simpler than represented here (with full 0:3 connections).
	// But until we have better representation of wire networks, this has
	// to suffice.
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGR_RTERM_GCLK%i", 2))) goto xout;
	// same from MCB_REGH_GCLK...
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGR_RTERM_GCLK%i", 2))) goto xout;
	return 0;
xout:
	return rc;
}

static int run_gclk_vert_regs(struct fpga_model* model)
{
	struct w_net net;
	int rc, i;

	// net tying together 15 gclk lines from row 10..27
	net.last_inc = 15;
	for (i = 0; i <= 17; i++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+10))
			net.pts[i].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (i == 9) // row 19
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+10;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 19..53
	net.last_inc = 15;
	for (i = 0; i <= 34; i++) { // row 19..53
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+19))
			net.pts[i].name = "REGV_GCLKH_MAIN%i";
		else if (is_aty(Y_CHIP_HORIZ_REGS, model, i+19))
			net.pts[i].name = "CLKC_GCLK_MAIN%i";
		else if (i == 16) // row 35
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BRK";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+19;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 45..62
	net.last_inc = 15;
	for (i = 0; i <= 17; i++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+45))
			net.pts[i].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (i == 8) // row 53
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+45;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// a few local gclk networks at the center top and bottom
	{ struct w_net net = {
		1,
		{{ "REGT_GCLK%i",	0, TOP_OUTER_ROW, model->center_x-1 },
		 { "REGT_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x-1 },
		 { "REGV_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x },
		 { "BUFPLL_TOP_GCLK%i", 0, TOP_INNER_ROW, model->center_x+1 },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{ struct w_net net = {
		1,
		{{ "REGB_GCLK%i",	0, model->y_height-1, model->center_x-1 },
		 { "REGB_BTERM_GCLK%i", 0, model->y_height-2, model->center_x-1 },
		 { "REGV_BTERM_GCLK%i", 0, model->y_height-2, model->center_x },
		 { "BUFPLL_BOT_GCLK%i", 0, model->y_height-2, model->center_x+1 },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	// wire up gclk from tterm down to top 8 rows at center_x+1
	for (i = TOP_IO_TILES; i <= TOP_IO_TILES+HALF_ROW; i++) {
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			TOP_INNER_ROW, model->center_x+1,
				"IOI_TTERM_GCLK%i",  0, 15,
			i, model->center_x+1,
				(i == TOP_IO_TILES+HALF_ROW) ?
				"HCLK_GCLK_UP%i" : "GCLK%i", 0))) goto xout;
	}
	// same at the bottom upwards
	for (i = model->y_height-2-1; i >= model->y_height-2-HALF_ROW-1; i--) {
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->y_height-2, model->center_x+1,
				"IOI_BTERM_GCLK%i",  0, 15,
			i, model->center_x+1,
				(i == model->y_height-2-HALF_ROW-1) ?
				"HCLK_GCLK%i" : "GCLK%i", 0))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int run_logic_inout(struct fpga_model* model)
{
	struct fpga_tile* tile;
	char buf[128];
	int x, y, i, rc;

	// LOGICOUT
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
			for (y = 0; y < model->y_height; y++) {
				tile = &model->tiles[y * model->x_width + x];
				if (tile[1].flags & TF_LOGIC_XM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXM_LOGICOUT%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXL_LOGICOUT%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) goto xout;
				}
			}
		}
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
				if (YX_TILE(model, y, x)[2].flags & TF_BRAM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT3", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT2", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT1", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT0", 0))) goto xout;
				}
				if (YX_TILE(model, y, x)[2].flags & TF_MACC_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT3", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT2", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT1", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT0", 0))) goto xout;
				}
			}
		}
		if (is_atx(X_CENTER_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x, "LOGICOUT%i", 0, 23, y+1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
					if (YX_TILE(model, y-1, x+2)->flags & TF_DCM_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB2_LOGICOUT%i", 0))) goto xout;
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB1_LOGICOUT%i", 0))) goto xout;
					} else if (YX_TILE(model, y-1, x+2)->flags & TF_PLL_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB2_LOGICOUT%i", 0))) goto xout;
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB1_LOGICOUT%i", 0))) goto xout;
					}
				}
				if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_REGC_LOGICOUT%i", 0))) goto xout;
				}
			}
		}
		if (is_atx(X_LEFT_IO_ROUTING_COL|X_RIGHT_IO_ROUTING_COL, model, x)) {
			int wired_side, local_size;
			if (is_atx(X_LEFT_IO_ROUTING_COL, model, x)) {
				local_size = 1;
				wired_side = Y_LEFT_WIRED;
			} else {
				local_size = 2;
				wired_side = Y_RIGHT_WIRED;
			}
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if (y < TOP_IO_TILES+local_size || y > model->y_height-BOT_IO_TILES-local_size-1) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOCAL_LOGICOUT%i", 0))) goto xout;
				} else if (is_aty(wired_side, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) goto xout;
				} else {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
				}
			}
		}
	}
	// LOGICIN
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs

		if (i%2) { // DCM
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 11))) goto xout;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 11))) goto xout;
		} else { // PLL
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 11))) goto xout;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 0, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB1_LOGICINB%i", 0))) goto xout;
		}
	}

	for (y = 0; y < model->y_height; y++) {
		for (x = 0; x < model->x_width; x++) {
			tile = &model->tiles[y * model->x_width + x];

			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				static const int north_p[4] = {21, 28, 52, 60};
				static const int south_p[4] = {20, 36, 44, 62};

				for (i = 0; i < sizeof(north_p)/sizeof(north_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN%i", north_p[i])))) goto xout;
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model, y-1, x, pf("LOGICIN_N%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN_N%i", north_p[i]), y+1, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
				}
				for (i = 0; i < sizeof(south_p)/sizeof(south_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN_S%i", south_p[i]), y-1, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model, y+1, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x))
							if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) goto xout;
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					}
				}

				if (tile[1].flags & TF_LOGIC_XM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "CLEXM_LOGICIN_B%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i",  0, 35, y, x+1, "CLEXL_LOGICIN_B%i",  0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 37, 43, y, x+1, "CLEXL_LOGICIN_B%i", 37))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 45, 52, y, x+1, "CLEXL_LOGICIN_B%i", 45))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 54, 60, y, x+1, "CLEXL_LOGICIN_B%i", 54))) goto xout;
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 3, y, x+1, "IOI_LOGICINB%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 5, 9, y, x+1, "IOI_LOGICINB%i", 5))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 11, 62, y, x+1, "IOI_LOGICINB%i", 11))) goto xout;
				}
				if (is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
					if (tile[2].flags & TF_BRAM_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "BRAM_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) goto xout;
						}
					}
				}
				if (is_atx(X_ROUTING_TO_MACC_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
					if (tile[2].flags & TF_MACC_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "MACC_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) goto xout;
						}
					}
				}
				if (is_atx(X_CENTER_REGS_COL, model, x+3)) {
					if (tile[2].flags & (TF_PLL_DEV|TF_DCM_DEV)) {
						const char* prefix = (tile[2].flags & TF_PLL_DEV) ? "PLL_CLB2" : "DCM_CLB2";

						for (i = 0;; i = 2) {
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN4"))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 5))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN10"))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 11))) goto xout;

							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y, x+2, pf("%s_LOGICINB%%i", prefix), 0))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y, x+2, pf("%s_LOGICINB4", prefix)))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y, x+2, pf("%s_LOGICINB%%i", prefix), 5))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y, x+2, pf("%s_LOGICINB10", prefix)))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y, x+2, pf("%s_LOGICINB%%i", prefix), 11))) goto xout;

							if (tile[2].flags & TF_PLL_DEV) {
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62, y+2, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62,   y, x+2, "PLL_CLB1_LOGICINB%i", 0))) goto xout;
								break;
							}
							if (i == 2) break;
							prefix = "DCM_CLB1";
						}
					}
					if (is_aty(Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_range(model, NOPREF_BI_F,   y, x, "LOGICIN_B%i",  0, 62,   y, x+1, "INT_INTERFACE_REGC_LOGICBIN%i", 0))) goto xout;
						int clk_pins[16] = { 24, 15, 7, 42, 5, 12, 62, 16, 47, 20, 38, 23, 48, 57, 44, 4 };
						for (i = 0; i <= 15; i++) {
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+1, pf("REGC_CLE_SEL%i", i)))) goto xout;
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+2, pf("REGC_CMT_SEL%i", i)))) goto xout;
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+3, pf("CLKC_SEL%i_PLL", i)))) goto xout;
						}
					}
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static const char* s_4wire = "BAMCE";

int wire_SS4E_N3(struct fpga_model* model, const struct w_net* net)
{
	int i, j, rc, e_y, e_x, extra_n3;

	for (i = 0; net->pts[i].name[0] != 0; i++);
	if (!i || net->pts[i-1].name[3] != 'E') return 0;

	// i-1 is 'E', i-2 is 'C' which if it's double
	// because of HCLK is also in i-3
	e_y = net->pts[i-1].y;
	e_x = net->pts[i-1].x;
	if (e_y == BOT_TERM(model)-1
	    && !is_atx(X_FABRIC_BRAM_ROUTING_COL, model, e_x))
		if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E_N3", e_y+1, e_x, "SS4E_N3"))) goto xout;
	if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E3", e_y-1, e_x, "SS4E_N3"))) goto xout;
	if (row_pos(e_y-1, model) == HCLK_POS
	    || IS_CENTER_Y(e_y-1, model)) {
		if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-1, e_x, "SS4E_N3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-1, e_x, "SS4C3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-2, e_x, "SS4C3", e_y-1, e_x, "SS4E_N3"))) goto xout;
		extra_n3 = 1;
		j = i-4;
	} else {
		extra_n3 = 0;
		j = i-3;
	}
	for (; j >= 0; j--) {
		if ((rc = add_conn_bi_pref(model, net->pts[j].y, e_x, pf("%.4s3", net->pts[j].name), e_y-1, e_x, "SS4E_N3"))) goto xout;
		if (extra_n3)
			if ((rc = add_conn_bi_pref(model, net->pts[j].y, e_x, pf("%.4s3", net->pts[j].name), e_y-2, e_x, "SS4E_N3"))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int run_direction_wires(struct fpga_model* model)
{
	int x, y, i, j, _row_num, _row_pos, rc;
	struct w_net net;

	// SS4
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		// some wiring at the top
		net.last_inc = 3;
		for (i = 1; i < 5; i++) { // go through "BAMCE"
			net.pts[0].start_count = 0;
			net.pts[0].y = TOP_TERM(model);
			net.pts[0].x = x;
			net.pts[0].name = pf("SS4%c%%i", s_4wire[i]);
			for (j = i; j < 5; j++) {
				net.pts[j-i+1].start_count = 0;
				net.pts[j-i+1].y = TOP_TERM(model)+(j-i+1);
				net.pts[j-i+1].x = x;
				net.pts[j-i+1].name = pf("SS4%c%%i", s_4wire[j]);
			}
			net.pts[j-i+1].name = "";
			if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			if ((rc = wire_SS4E_N3(model, &net))) goto xout;
		}
		// rest going down to bottom termination
		for (y = 0; y < model->y_height; y++) {
			is_in_row(model, y, &_row_num, &_row_pos);
			if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)
			    && y > BOT_TERM(model)-5)
				break;
			if (_row_pos < 0 || _row_pos == 8)
				continue;

			net.last_inc = 3;
			j = 0;
			for (i = 0; i < 5; i++) { // go through "BAMCE"
				net.pts[j].start_count = 0;
				net.pts[j].y = y+j;
				net.pts[j].x = x;
				if (y+j == BOT_TERM(model)) {
					ABORT(!i);
					net.pts[j].name = pf("SS4%c%%i", s_4wire[i-1]);
					j++;
					break;
				}
				if (IS_CENTER_Y(y+j, model)
				    || row_pos(y+j, model) == HCLK_POS) {
					ABORT(!i);
					net.pts[j].name = pf("SS4%c%%i", s_4wire[i-1]);
					j++;
					net.pts[j].start_count = 0;
					net.pts[j].y = y+j;
					net.pts[j].x = x;
				}
				net.pts[j].name = pf("SS4%c%%i", s_4wire[i]);
				j++;
			}
			net.pts[j].name = "";
			if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			if ((rc = wire_SS4E_N3(model, &net))) goto xout;
		}
	}

	// NN4
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = 0; y < model->y_height; y++) {
			is_in_row(model, y, &_row_num, &_row_pos);
			if (_row_pos >= 0 && _row_pos != 8) {
				net.last_inc = 3;
				j = 0;
				for (i = 0; i < 5; i++) { // go through "BAMCE"
					net.pts[j].start_count = 0;
					net.pts[j].y = y-j;
					net.pts[j].x = x;
					if (y-j == TOP_INNER_ROW) {
						ABORT(!i);
						net.pts[j].name = pf("NN4%c%%i", s_4wire[i-1]);
						j++;
						break;
					}
					net.pts[j].name = pf("NN4%c%%i", s_4wire[i]);
					if (IS_CENTER_Y(y-j, model)
					    || row_pos(y-j, model) == HCLK_POS) {
						ABORT(!i);
						i--;
					}
					j++;
				}
				net.pts[j].name = "";
				if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			}
		}
		if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			net.last_inc = 3;
			for (i = 1; i < 5; i++) { // go through "BAMCE"
				net.pts[0].start_count = 0;
				net.pts[0].y = BOT_TERM(model);
				net.pts[0].x = x;
				net.pts[0].name = pf("NN4%c%%i", s_4wire[i]);
				for (j = i; j < 5; j++) {
					net.pts[j-i+1].start_count = 0;
					net.pts[j-i+1].y = BOT_TERM(model)-(j-i+1);
					net.pts[j-i+1].x = x;
					net.pts[j-i+1].name = pf("NN4%c%%i", s_4wire[j]);
				}
				net.pts[j-i+1].name = "";
				if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			}
		}
	}

	for (y = 0; y < model->y_height; y++) {
		for (x = 0; x < model->x_width; x++) {
			// NR1
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "NR1E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						{ struct w_net net = {
							3,
							{{ "NR1E%i", 0,   y, x },
							 { "NR1E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					}
				}
			}

			// NN2
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					{ struct w_net net = {
						0,
						{{ "NN2E_S0", 0,   y, x },
						 { "NN2E_S0", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_INNER_TOP, model, y-2)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model, y-1, x, "NN2M0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-2)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E0",   y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2M0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-3, x, "NN2E0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
				} else {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi(model, y-2, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
					if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if ((rc = add_conn_bi(model, y, x, "NN2E_S0", y-1, x, "NN2E0"))) goto xout;
						if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
							{ struct w_net net = {
								3,
								{{ "NN2E%i", 0, y-1, x },
								 { "NN2M%i", 0,   y, x },
								 { "NN2M%i", 0, y+1, x },
								 { "" }}};
							if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
							if ((rc = add_conn_range(model, PREF_BI_F, y, x, "NN2E%i", 0, 3, y+1, x, "NN2E%i", 0))) goto xout;
							if ((rc = add_conn_bi(model, y, x, "NN2E0", y+1, x, "IOI_BTERM_NN2E_S0"))) goto xout;
							if ((rc = add_conn_bi(model, y, x, "NN2E_S0", y+1, x, "IOI_BTERM_NN2M0"))) goto xout;
						}
					}
				}
			}

			// SS2
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+2)) {
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+1, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2E_N3"))) goto xout;

					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;

					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2M3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2M3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2B3",   y+3, x, "SS2E_N3"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2B%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
				} else if (is_aty(Y_INNER_BOTTOM, model, y+2)) {
					if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						{ struct w_net net = {
							3,
							{{ "SS2B%i", 0,   y, x },
							 { "SS2M%i", 0, y+1, x },
							 { "SS2M%i", 0, y+2, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					}
				} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
					if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2B%i", 0, 3, y+1, x, "SS2B%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E_N3"))) goto xout;
					}
				} else {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						{ struct w_net net = {
							3,
							{{ "SS2M%i", 0, y-1, x },
							 { "SS2M%i", 0,   y, x },
							 { "SS2E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2E%i", 0, 3, y-1, x, "SS2E%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E3",   y-1, x, "SS2E_N3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y-1, x, "SS2M3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E3"))) goto xout;
					}
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2E%i", 0, y+2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+1, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2E3"))) goto xout;
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int init_tiles(struct fpga_model* model)
{
	int tile_rows, tile_columns, i, j, k, l, row_top_y, left_side;
	int start, end, no_io;
	char cur_cfgcol, last_col;
	struct fpga_tile* tile_i0;

	tile_rows = 1 /* middle */ + (8+1+8)*model->cfg_rows + 2+2 /* two extra tiles at top and bottom */;
	tile_columns = LEFT_SIDE_WIDTH + RIGHT_SIDE_WIDTH;
	for (i = 0; model->cfg_columns[i] != 0; i++) {
		if (model->cfg_columns[i] == 'L' || model->cfg_columns[i] == 'M')
			tile_columns += 2; // 2 for logic blocks L/M
		else if (model->cfg_columns[i] == 'B' || model->cfg_columns[i] == 'D')
			tile_columns += 3; // 3 for bram or macc
		else if (model->cfg_columns[i] == 'R')
			tile_columns += 2+2; // 2+2 for middle IO+logic+PLL/DCM
	}
	model->tmp_str = malloc((tile_columns > tile_rows ? tile_columns : tile_rows) * sizeof(*model->tmp_str));
	if (!model->tmp_str) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return -1;
	}
	model->x_width = tile_columns;
	model->y_height = tile_rows;
	model->center_x = -1;
	model->tiles = calloc(tile_columns * tile_rows, sizeof(struct fpga_tile));
	if (!model->tiles) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return -1;
	}
	for (i = 0; i < tile_rows * tile_columns; i++)
		model->tiles[i].type = NA;
	if (!(tile_rows % 2))
		fprintf(stderr, "Unexpected even number of tile rows (%i).\n", tile_rows);
	model->center_y = 2 /* top IO files */ + (model->cfg_rows/2)*(8+1/*middle of row clock*/+8);

	//
	// top, bottom, center:
	// go through columns from left to right, rows from top to bottom
	//

	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
	i = 5; // skip left IO columns
	for (j = 0; model->cfg_columns[j]; j++) {
		cur_cfgcol = model->cfg_columns[j];
		switch (cur_cfgcol) {
			case 'L':
			case 'l':
			case 'M':
			case 'm':
				no_io = (next_non_whitespace(&model->cfg_columns[j+1]) == 'n');
				last_col = last_major(model->cfg_columns, j);

				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				if (no_io) model->tiles[i].flags |= TF_ROUTING_NO_IO;
				model->tiles[i+1].flags |= TF_FABRIC_LOGIC_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles (center row)
					start = ((k == model->cfg_rows-1 && !no_io) ? 2 : 0);
					end = ((k == 0 && !no_io) ? 14 : 16);
					for (l = start; l < end; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15 || (!k && no_io))
							tile_i0->type = ROUTING;
						else
							tile_i0->type = ROUTING_BRK;
						if (cur_cfgcol == 'L') {
							tile_i0[1].flags |= TF_LOGIC_XL_DEV;
							tile_i0[1].type = LOGIC_XL;
						} else {
							tile_i0[1].flags |= TF_LOGIC_XM_DEV;
							tile_i0[1].type = LOGIC_XM;
						}
					}
					if (cur_cfgcol == 'L') {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					} else {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XM;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XM;
					}
				}

				if (last_col == 'R') {
					model->tiles[tile_columns + i].type = IO_BUFPLL_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i].type = IO_BUFPLL_TERM_B;
				} else {
					model->tiles[tile_columns + i].type = IO_TERM_T;
					if (!no_io)
						model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i].type = LOGIC_ROUTING_TERM_B;
				}
				if (!no_io) {
					model->tiles[i].type = IO_T;
					model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
					model->tiles[2*tile_columns + i].type = IO_ROUTING;
					model->tiles[3*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;
				}

				if (last_col == 'R') {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				} else {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_TERM_T;
					if (!no_io)
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = LOGIC_NOIO_TERM_B;
				}
				if (!no_io) {
					model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
					model->tiles[2*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
					model->tiles[3*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				}

				if (cur_cfgcol == 'L') {
					model->tiles[model->center_y*tile_columns + i].type = REGH_ROUTING_XL;
					model->tiles[model->center_y*tile_columns + i + 1].type = REGH_LOGIC_XL;
				} else {
					model->tiles[model->center_y*tile_columns + i].type = REGH_ROUTING_XM;
					model->tiles[model->center_y*tile_columns + i + 1].type = REGH_LOGIC_XM;
				}
				i += 2;
				break;
			case 'B':
				if (next_non_whitespace(&model->cfg_columns[j+1]) == 'g') {
					if (left_side)
						model->left_gclk_sep_x = i+2;
					else
						model->right_gclk_sep_x = i+2;
				}
				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i].flags |= TF_ROUTING_NO_IO; // no_io always on for BRAM
				model->tiles[i+1].flags |= TF_FABRIC_BRAM_MACC_ROUTING_COL;
				model->tiles[i+2].flags |= TF_FABRIC_BRAM_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15)
							tile_i0->type = BRAM_ROUTING;
						else
							tile_i0->type = BRAM_ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4)) {
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = BRAM;
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].flags |= TF_BRAM_DEV;
						}
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_BRAM_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_BRAM_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_BRAM;
				}

				model->tiles[tile_columns + i].type = BRAM_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = BRAM_ROUTING_TERM_B;
				model->tiles[tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_B;
				model->tiles[tile_columns + i + 2].type = left_side ? BRAM_TERM_LT : BRAM_TERM_RT;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? BRAM_TERM_LB : BRAM_TERM_RB;

				model->tiles[model->center_y*tile_columns + i].type = REGH_BRAM_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGH_BRAM_ROUTING_VIA;
				model->tiles[model->center_y*tile_columns + i + 2].type = left_side ? REGH_BRAM_L : REGH_BRAM_R;
				i += 3;
				break;
			case 'D':
				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i].flags |= TF_ROUTING_NO_IO; // no_io always on for MACC
				model->tiles[i+1].flags |= TF_FABRIC_BRAM_MACC_ROUTING_COL;
				model->tiles[i+2].flags |= TF_FABRIC_MACC_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15)
							tile_i0->type = ROUTING;
						else
							tile_i0->type = ROUTING_BRK;
						tile_i0[1].type = ROUTING_VIA;
						if (!(l%4)) {
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = MACC;
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].flags |= TF_MACC_DEV;
						}
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_MACC_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_MACC_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_MACC;
				}

				model->tiles[tile_columns + i].type = MACC_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = MACC_ROUTING_TERM_B;
				model->tiles[tile_columns + i + 1].type = MACC_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
				model->tiles[tile_columns + i + 2].type = left_side ? MACC_TERM_TL : MACC_TERM_TR;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? MACC_TERM_BL : MACC_TERM_BR;

				model->tiles[model->center_y*tile_columns + i].type = REGH_MACC_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGH_MACC_ROUTING_VIA;
				model->tiles[model->center_y*tile_columns + i + 2].type = REGH_MACC_L;
				i += 3;
				break;
			case 'R':
				if (next_non_whitespace(&model->cfg_columns[j+1]) != 'M') {
					// We expect a LOGIC_XM column to follow the center for
					// the top and bottom bufpll and reg routing.
					fprintf(stderr, "Expecting LOGIC_XM after center but found '%c'\n", model->cfg_columns[j+1]);
				}
				model->center_x = i+3;

				left_side = 0;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];

						if ((k < model->cfg_rows-1 || l >= 2) && (k || l<14)) {
							if (l < 15)
								tile_i0->type = ROUTING;
							else
								tile_i0->type = ROUTING_BRK;
							if (l == 7)
								tile_i0[1].type = ROUTING_VIA_IO;
							else if (l == 8)
								tile_i0[1].type = (k%2) ? ROUTING_VIA_CARRY : ROUTING_VIA_IO_DCM;
							else if (l == 15 && k==model->cfg_rows/2)
								tile_i0[1].type = ROUTING_VIA_REGC;
							else {
								tile_i0[1].type = LOGIC_XL;
								tile_i0[1].flags |= TF_LOGIC_XL_DEV;
							}
						}
						if (l == 7
						    || (l == 8 && !(k%2))) { // even row, together with DCM
							tile_i0->type = IO_ROUTING;
						}

						if (l == 7) {
							if (k%2) { // odd
								model->tiles[(row_top_y+l)*tile_columns + i + 2].flags |= TF_PLL_DEV;
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? PLL_B : PLL_T;
							} else { // even
								model->tiles[(row_top_y+l)*tile_columns + i + 2].flags |= TF_DCM_DEV;
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? DCM_B : DCM_T;
							}
						}
						// four midbuf tiles, in the middle of the top and bottom halves
						if (l == 15) {
							if (k == model->cfg_rows*3/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_MIDBUF_T;
							else if (k == model->cfg_rows/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_HCLKBUF_B;
							else
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_BRK;
						} else if (l == 0 && k == model->cfg_rows*3/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_HCLKBUF_T;
						} else if (l == 0 && k == model->cfg_rows/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_MIDBUF_B;
						} else if (l == 8) {
							model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = (k<model->cfg_rows/2) ? REGV_B : REGV_T;
						} else
							model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 3].type = REGV;
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 3].type = HCLK_REGV;
				}
				model->tiles[i].type = IO_T;
				model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
				model->tiles[tile_columns + i].type = IO_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
				model->tiles[2*tile_columns + i].type = IO_ROUTING;
				model->tiles[3*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;

				model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
				model->tiles[2*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
				model->tiles[3*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;

				model->tiles[i + 2].type = REG_T;
				model->tiles[tile_columns + i + 2].type = REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = REG_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = REG_B;
				model->tiles[tile_columns + i + 3].type = REGV_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 3].type = REGV_TERM_B;

				model->tiles[model->center_y*tile_columns + i].type = REGC_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGC_LOGIC;
				model->tiles[model->center_y*tile_columns + i + 2].type = REGC_CMT;
				model->tiles[model->center_y*tile_columns + i + 3].type = CENTER;

				i += 4;
				break;
			case ' ': // space used to make string more readable only
			case 'g': // global clock separator
			case 'n': // noio for logic blocks
				break;
			default:
				fprintf(stderr, "Ignoring unexpected column identifier '%c'\n", cur_cfgcol);
				break;
		}
	}

	//
	// left IO
	//

	for (k = model->cfg_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// +0
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns].flags |= TF_WIRED;
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns].type = IO_L;
			}
			//
			// +1
			//
			if ((k == model->cfg_rows-1 && !l) || (!k && l==15))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = CORNER_TERM_L;
			else if (k == model->cfg_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_TOP;
			else if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_BOT;
			else if (k == (model->cfg_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_TOP;
			else if (k == (model->cfg_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = IO_TERM_L;
			//
			// +2
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				if (l == 15 && k && k != model->cfg_rows/2)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_IO_L_BRK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING_IO_L;
			} else { // unwired
				if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_BRK;
				else if (k == model->cfg_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING;
			}
			//
			// +3
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_IO_VIA_L;
			} else { // unwired
				if (k == model->cfg_rows-1 && !l) {
					model->tiles[(row_top_y+l)*tile_columns + 3].type = CORNER_TL;
				} else if (!k && l == 15) {
					model->tiles[(row_top_y+l+1)*tile_columns + 3].type = CORNER_BL;
				} else {
					if (k && k != model->cfg_rows/2 && l == 15)
						model->tiles[(row_top_y+l+1)*tile_columns + 3].type = ROUTING_VIA_CARRY;
					else
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_VIA;
				}
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + 1].type = HCLK_TERM_L;
		model->tiles[(row_top_y+8)*tile_columns + 2].type = HCLK_ROUTING_IO_L;
		if (k >= model->cfg_rows/2) { // top half
			if (k > (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_UP_L;
			else if (k == (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_DN_L;
		} else { // bottom half
			if (k < model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_DN_L;
			else if (k == model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_UP_L;
		}
		model->tiles[(row_top_y+8)*tile_columns + 4].type = HCLK_MCB;
	}

	model->tiles[(model->center_y-3)*tile_columns].type = IO_PCI_L;
	model->tiles[(model->center_y-2)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[(model->center_y-1)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[model->center_y*tile_columns].type = REG_L;
	model->tiles[(model->center_y+1)*tile_columns].type = IO_RDY_L;

	model->tiles[model->center_y*tile_columns + 1].type = REGH_IO_TERM_L;

	model->tiles[tile_columns + 2].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + 2].type = CORNER_TERM_B;
	model->tiles[model->center_y*tile_columns + 2].type = REGH_ROUTING_IO_L;

	model->tiles[tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[(tile_rows-2)*tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[model->center_y*tile_columns + 3].type = REGH_IO_L;
	model->tiles[model->center_y*tile_columns + 4].type = REGH_MCB;

	//
	// right IO
	//

	for (k = model->cfg_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// -1
			//
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 1].flags |= TF_WIRED;

			if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_RDY_R;
			else if (k == model->cfg_rows/2 && l == 14)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == model->cfg_rows/2 && l == 15)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == model->cfg_rows/2-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 1].type = IO_PCI_R;
			else {
				if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 1].type = IO_R;
			}
			//
			// -2
			//
			if ((k == model->cfg_rows-1 && (!l || l == 1)) || (!k && (l==15 || l==14)))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = CORNER_TERM_R;
			else if (k == model->cfg_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_TOP;
			else if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_BOT;
			else if (k == (model->cfg_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_TOP;
			else if (k == (model->cfg_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = IO_TERM_R;
			//
			// -3
			//
			//
			// -4
			//
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_IO_VIA_R;
			else {
				if (k == model->cfg_rows-1 && l == 0)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_UPPER;
				else if (k == model->cfg_rows-1 && l == 1)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_LOWER;
				else if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = ROUTING_VIA_CARRY;
				else if (!k && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = CORNER_BR_UPPER;
				else if (!k && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = CORNER_BR_LOWER;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_VIA;
			}
			//
			// -5
			//
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = IO_ROUTING;
			else {
				if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_BRK;
				else if (k == model->cfg_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = ROUTING;
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 2].type = HCLK_TERM_R;
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 3].type = HCLK_MCB;

		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 5].type = HCLK_ROUTING_IO_R;
		if (k >= model->cfg_rows/2) { // top half
			if (k > (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_UP_R;
			else if (k == (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_DN_R;
		} else { // bottom half
			if (k < model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_DN_R;
			else if (k == model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_UP_R;
		}
	}
	model->tiles[tile_columns + tile_columns - 5].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 5].type = CORNER_TERM_B;
	model->tiles[tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 1].type = REG_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 2].type = REGH_IO_TERM_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 3].type = REGH_MCB;
	model->tiles[model->center_y*tile_columns + tile_columns - 4].type = REGH_IO_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 5].type = REGH_ROUTING_IO_R;
	return 0;
}

//
// helper funcs
//

#define NUM_PF_BUFS	16

static const char* pf(const char* fmt, ...)
{
	// safe to call it NUM_PF_BUFStimes in 1 expression,
	// such as function params or a net structure
	static char pf_buf[NUM_PF_BUFS][128];
	static int last_buf = 0;
	va_list list;
	last_buf = (last_buf+1)%NUM_PF_BUFS;
	pf_buf[last_buf][0] = 0;
	va_start(list, fmt);
	vsnprintf(pf_buf[last_buf], sizeof(pf_buf[0]), fmt, list);
	va_end(list);
	return pf_buf[last_buf];
}

static const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name)
{
	static char buf[8][128];
	static int last_buf = 0;
	char* prefix;

	if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
		prefix = is_atx(X_CENTER_REGS_COL, model, x+3)
			? "REGC_INT_" : "REGH_";
	} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
		prefix = "HCLK_";
	else if (is_aty(Y_INNER_TOP, model, y))
		prefix = "IOI_TTERM_";
	else if (is_aty(Y_INNER_BOTTOM, model, y))
		prefix = "IOI_BTERM_";
	else
		prefix = "";

	last_buf = (last_buf+1)%8;
	buf[last_buf][0] = 0;
	strcpy(buf[last_buf], prefix);
	strcat(buf[last_buf], wire_name);
	return buf[last_buf];
}

int has_connpt(struct fpga_model* model, int y, int x,
	const char* name)
{
	struct fpga_tile* tile;
	uint16_t name_i;
	int i;

	if (strarray_find(&model->str, name, &i))
		ABORT(1);
	if (i == STRIDX_NO_ENTRY)
		return 0;
	name_i = i;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == name_i)
			return 1;
	}
	return 0;
}

static int _add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o);

static int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name)
{
	return _add_connpt_name(model, y, x, connpt_name,
		1 /* warn_if_duplicate */,
		0 /* name_i */, 0 /* conn_point_o */);
}

#define CONN_NAMES_INCREMENT	128

static int _add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o)
{
	struct fpga_tile* tile;
	uint16_t _name_i;
	int rc, i;

	tile = &model->tiles[y * model->x_width + x];
	rc = strarray_add(&model->str, connpt_name, &i);
	if (rc) return rc;
	if (i > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	_name_i = i;
	if (name_i) *name_i = i;

	// Search for an existing connection point under name.
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == _name_i)
			break;
	}
	if (conn_point_o) *conn_point_o = i;
	if (i < tile->num_conn_point_names) {
		if (warn_if_duplicate)
			fprintf(stderr,
				"Duplicate connection point name y%02i x%02u %s\n",
				y, x, connpt_name);
		return 0;
	}
	// This is the first connection under name, add name.
	if (!(tile->num_conn_point_names % CONN_NAMES_INCREMENT)) {
		uint16_t* new_ptr = realloc(tile->conn_point_names,
			(tile->num_conn_point_names+CONN_NAMES_INCREMENT)*2*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_names = new_ptr;
	}
	tile->conn_point_names[tile->num_conn_point_names*2] = tile->num_conn_point_dests;
	tile->conn_point_names[tile->num_conn_point_names*2+1] = _name_i;
	tile->num_conn_point_names++;
	return 0;
}

static int has_device(struct fpga_model* model, int y, int x, int dev)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i;

	for (i = 0; i < tile->num_devices; i++) {
		if (tile->devices[i].type == dev)
			return 1;
	}
	return 0;
}

static int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2)
{
	char name_buf[64];
	int rc;

	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix1);
	rc = add_connpt_name(model, y, x, name_buf);
	if (rc) goto xout;
	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix2);
	rc = add_connpt_name(model, y, x, name_buf);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

#define CONNS_INCREMENT		128
#undef DBG_ADD_CONN_UNI

static int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	struct fpga_tile* tile;
	uint16_t name1_i, name2_i;
	uint16_t* new_ptr;
	int conn_start, num_conn_point_dests_for_this_wire, rc, j, conn_point_o;

	rc = _add_connpt_name(model, y1, x1, name1, 0 /* warn_if_duplicate */,
		&name1_i, &conn_point_o);
	if (rc) goto xout;

	rc = strarray_add(&model->str, name2, &j);
	if (rc) return rc;
	if (j > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	name2_i = j;
	tile = &model->tiles[y1 * model->x_width + x1];
	conn_start = tile->conn_point_names[conn_point_o*2];
	if (conn_point_o+1 >= tile->num_conn_point_names)
		num_conn_point_dests_for_this_wire = tile->num_conn_point_dests - conn_start;
	else
		num_conn_point_dests_for_this_wire = tile->conn_point_names[(conn_point_o+1)*2] - conn_start;

	// Is the connection made a second time?
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
		if (tile->conn_point_dests[j*3] == x2
		    && tile->conn_point_dests[j*3+1] == y2
		    && tile->conn_point_dests[j*3+2] == name2_i) {
			fprintf(stderr, "Duplicate conn (num_conn_point_dests %i): y%02i x%02i %s - y%02i x%02i %s.\n",
				num_conn_point_dests_for_this_wire, y1, x1, name1, y2, x2, name2);
			for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
				fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j,
					y1, x1, name1,
					tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
					strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
			}
			return 0;
		}
	}

	if (!(tile->num_conn_point_dests % CONNS_INCREMENT)) {
		new_ptr = realloc(tile->conn_point_dests, (tile->num_conn_point_dests+CONNS_INCREMENT)*3*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_dests = new_ptr;
	}
	if (tile->num_conn_point_dests > j)
		memmove(&tile->conn_point_dests[(j+1)*3], &tile->conn_point_dests[j*3], (tile->num_conn_point_dests-j)*3*sizeof(uint16_t));
	tile->conn_point_dests[j*3] = x2;
	tile->conn_point_dests[j*3+1] = y2;
	tile->conn_point_dests[j*3+2] = name2_i;
	tile->num_conn_point_dests++;
	while (conn_point_o+1 < tile->num_conn_point_names) {
		tile->conn_point_names[(conn_point_o+1)*2]++;
		conn_point_o++;
	}
#if DBG_ADD_CONN_UNI
	printf("conn_point_dests for y%02i x%02i %s now:\n", y1, x1, name1);
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire+1; j++) {
		fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j, y1, x1, name1,
			tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
			strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
	}
#endif
	return 0;
xout:
	return rc;
}

int add_conn_uni_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_uni(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

static int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	int rc = add_conn_uni(model, y1, x1, name1, y2, x2, name2);
	if (rc) return rc;
	return add_conn_uni(model, y2, x2, name2, y1, x1, name1);
}

static int add_conn_bi_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_bi(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

static int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func, int y1, int x1, const char* name1, int start1, int last1, int y2, int x2, const char* name2, int start2)
{
	char buf1[128], buf2[128];
	int rc, i;

	if (last1 <= start1)
		return (*add_conn_func)(model, y1, x1, name1, y2, x2, name2);
	for (i = start1; i <= last1; i++) {
		snprintf(buf1, sizeof(buf1), name1, i);
		if (start2 & COUNT_DOWN)
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)-(i-start1));
		else
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)+(i-start1));
		rc = (*add_conn_func)(model, y1, x1, buf1, y2, x2, buf2);
		if (rc) return rc;
	}
	return 0;
}

static int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net)
{
	int i, j, rc;

	for (i = 0; net->pts[i].name[0] && i < sizeof(net->pts)/sizeof(net->pts[0]); i++) {
		for (j = i+1; net->pts[j].name[0] && j < sizeof(net->pts)/sizeof(net->pts[0]); j++) {
			rc = add_conn_range(model, add_conn_func,
				net->pts[i].y, net->pts[i].x,
				net->pts[i].name,
				net->pts[i].start_count,
				net->pts[i].start_count + net->last_inc,
				net->pts[j].y, net->pts[j].x,
				net->pts[j].name,
				net->pts[j].start_count);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

#define SWITCH_ALLOC_INCREMENT 64

#define DBG_ALLOW_ADDPOINTS

static int add_switch(struct fpga_model* model, int y, int x, const char* from,
	const char* to, int is_bidirectional)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int rc, i, from_idx, to_idx, from_connpt_o, to_connpt_o;
	uint32_t new_switch;

// later this can be strarray_find() and not strarray_add(), but
// then we need all wires and ports to be present first...
#ifdef DBG_ALLOW_ADDPOINTS
	rc = strarray_add(&model->str, from, &from_idx);
	if (rc) goto xout;
	rc = strarray_add(&model->str, to, &to_idx);
	if (rc) goto xout;
#else
	rc = strarray_find(&model->str, from, &from_idx);
	if (rc) goto xout;
	rc = strarray_find(&model->str, to, &to_idx);
	if (rc) goto xout;
#endif
	if (from_idx == STRIDX_NO_ENTRY || to_idx == STRIDX_NO_ENTRY) {
		fprintf(stderr, "No string for switch from %s (%i) or %s (%i).\n",
			from, from_idx, to, to_idx);
		return -1;
	}

	from_connpt_o = -1;
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == from_idx) {
			from_connpt_o = i;
			break;
		}
	}
#ifdef DBG_ALLOW_ADDPOINTS
	if (from_connpt_o == -1) {
		rc = add_connpt_name(model, y, x, from);
		if (rc) goto xout;
		for (i = 0; i < tile->num_conn_point_names; i++) {
			if (tile->conn_point_names[i*2+1] == from_idx) {
				from_connpt_o = i;
				break;
			}
		}
	}
#endif
	to_connpt_o = -1;
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == to_idx) {
			to_connpt_o = i;
			break;
		}
	}
#ifdef DBG_ALLOW_ADDPOINTS
	if (to_connpt_o == -1) {
		rc = add_connpt_name(model, y, x, to);
		if (rc) goto xout;
		for (i = 0; i < tile->num_conn_point_names; i++) {
			if (tile->conn_point_names[i*2+1] == to_idx) {
				to_connpt_o = i;
				break;
			}
		}
	}
#endif
	if (from_connpt_o == -1 || to_connpt_o == -1) {
		fprintf(stderr, "No conn point for switch from %s (%i/%i) or %s (%i/%i).\n",
			from, from_idx, from_connpt_o, to, to_idx, to_connpt_o);
		return -1;
	}
	if (from_connpt_o > SWITCH_MAX_CONNPT_O
	    || to_connpt_o > SWITCH_MAX_CONNPT_O) {
		fprintf(stderr, "Internal error in %s:%i (from_o %i to_o %i)\n",
			__FILE__, __LINE__, from_connpt_o, to_connpt_o);
		return -1;
	}
	new_switch = (from_connpt_o << 15) | to_connpt_o;
	if (is_bidirectional)
		new_switch |= SWITCH_BIDIRECTIONAL;

	for (i = 0; i < tile->num_switches; i++) {
		if ((tile->switches[i] & 0x3FFFFFFF) == (new_switch & 0x3FFFFFFF)) {
			fprintf(stderr, "Internal error in %s:%i duplicate switch from %s to %s\n",
				__FILE__, __LINE__, from, to);
			return -1;
		}
	}
	if (!(tile->num_switches % SWITCH_ALLOC_INCREMENT)) {
		uint32_t* new_ptr = realloc(tile->switches,
			(tile->num_switches+SWITCH_ALLOC_INCREMENT)*sizeof(*tile->switches));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
		tile->switches = new_ptr;
	}
	tile->switches[tile->num_switches++] = new_switch;
	return 0;
xout:
	return rc;
}

static void seed_strx(struct fpga_model* model, struct seed_data* data)
{
	int x, i;
	for (x = 0; x < model->x_width; x++) {
		model->tmp_str[x] = 0;
		for (i = 0; data[i].x_flags; i++) {
			if (is_atx(data[i].x_flags, model, x))
				model->tmp_str[x] = data[i].str;
		}
	}
}

static char next_non_whitespace(const char* s)
{
	int i;
	for (i = 0; s[i] == ' '; i++);
	return s[i];
}

static char last_major(const char* str, int cur_o)
{
	for (; cur_o; cur_o--) {
		if (str[cur_o-1] >= 'A' && str[cur_o-1] <= 'Z')
			return str[cur_o-1];
	}
	return 0;
}

int is_aty(int check, struct fpga_model* model, int y)
{
	if (y < 0) return 0;
	if (check & Y_INNER_TOP && y == TOP_INNER_ROW) return 1;
	if (check & Y_INNER_BOTTOM && y == model->y_height-BOT_INNER_ROW) return 1;
	if (check & Y_CHIP_HORIZ_REGS && y == model->center_y) return 1;
	if (check & (Y_ROW_HORIZ_AXSYMM|Y_BOTTOM_OF_ROW)) {
		int row_pos;
		is_in_row(model, y, 0 /* row_num */, &row_pos);
		if (check & Y_ROW_HORIZ_AXSYMM && row_pos == 8) return 1;
		if (check & Y_BOTTOM_OF_ROW && row_pos == 16) return 1;
	}
	if (check & Y_LEFT_WIRED && model->tiles[y*model->x_width].flags & TF_WIRED) return 1;
	if (check & Y_RIGHT_WIRED && model->tiles[y*model->x_width + model->x_width-RIGHT_OUTER_O].flags & TF_WIRED) return 1;
	if (check & Y_TOPBOT_IO_RANGE
	    && ((y > TOP_INNER_ROW && y <= TOP_INNER_ROW + TOP_IO_TILES)
	        || (y >= model->y_height - BOT_INNER_ROW - BOT_IO_TILES && y < model->y_height - BOT_INNER_ROW))) return 1;
	return 0;
}

int is_atx(int check, struct fpga_model* model, int x)
{
	if (x < 0) return 0;
	if (check & X_OUTER_LEFT && !x) return 1;
	if (check & X_INNER_LEFT && x == 1) return 1;
	if (check & X_INNER_RIGHT && x == model->x_width-2) return 1;
	if (check & X_OUTER_RIGHT && x == model->x_width-1) return 1;
	if (check & X_ROUTING_COL
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->x_width-5
		|| x == model->center_x-3)) return 1;
	if (model->tiles[x].flags & TF_FABRIC_ROUTING_COL) {
		if (check & X_ROUTING_TO_BRAM_COL
		    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
		    && model->tiles[x+2].flags & TF_FABRIC_BRAM_COL) return 1;
		if (check & X_ROUTING_TO_MACC_COL
		    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
		    && model->tiles[x+2].flags & TF_FABRIC_MACC_COL) return 1;
	}
	if (check & X_ROUTING_NO_IO && model->tiles[x].flags & TF_ROUTING_NO_IO) return 1;
	if (check & X_LOGIC_COL
	    && (model->tiles[x].flags & TF_FABRIC_LOGIC_COL
	        || x == model->center_x-2)) return 1;
	if (check & X_FABRIC_ROUTING_COL && model->tiles[x].flags & TF_FABRIC_ROUTING_COL) return 1;
	// todo: the routing/no_io flags could be cleaned up
	if (check & X_FABRIC_LOGIC_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_LOGIC_COL) return 1;
	if (check & X_FABRIC_LOGIC_COL && model->tiles[x].flags & TF_FABRIC_LOGIC_COL) return 1;
	if (check & X_FABRIC_BRAM_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
	    && model->tiles[x+2].flags & TF_FABRIC_BRAM_COL) return 1;
	if (check & X_FABRIC_MACC_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
	    && model->tiles[x+2].flags & TF_FABRIC_MACC_COL) return 1;
	if (check & X_FABRIC_BRAM_MACC_ROUTING_COL && model->tiles[x].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL) return 1;
	if (check & X_FABRIC_BRAM_COL && model->tiles[x].flags & TF_FABRIC_BRAM_COL) return 1;
	if (check & X_FABRIC_MACC_COL && model->tiles[x].flags & TF_FABRIC_MACC_COL) return 1;
	if (check & X_CENTER_ROUTING_COL && x == model->center_x-3) return 1;
	if (check & X_CENTER_LOGIC_COL && x == model->center_x-2) return 1;
	if (check & X_CENTER_CMTPLL_COL && x == model->center_x-1) return 1;
	if (check & X_CENTER_REGS_COL && x == model->center_x) return 1;
	if (check & X_LEFT_IO_ROUTING_COL && x == LEFT_IO_ROUTING) return 1;
	if (check & X_LEFT_IO_DEVS_COL && x == LEFT_IO_DEVS) return 1;
	if (check & X_RIGHT_IO_ROUTING_COL
	    && x == model->x_width-RIGHT_IO_ROUTING_O) return 1;
	if (check & X_RIGHT_IO_DEVS_COL
	    && x == model->x_width-RIGHT_IO_DEVS_O) return 1;
	if (check & X_LEFT_SIDE && x < model->center_x) return 1;
	if (check & X_LEFT_MCB && x == LEFT_MCB_COL) return 1;
	if (check & X_RIGHT_MCB && x == model->x_width-RIGHT_MCB_O) return 1;
	return 0;
}

int is_atyx(int check, struct fpga_model* model, int y, int x)
{
	struct fpga_tile* tile;

	if (y < 0 || x < 0) return 0;
	if (check & YX_ROUTING_TILE
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->x_width-5
		|| x == model->center_x-3)) {
		int row_pos;
		is_in_row(model, y, 0 /* row_num */, &row_pos);
		if (row_pos >= 0 && row_pos != 8) return 1;
	}
	tile = YX_TILE(model, y, x);
	if (check & YX_IO_ROUTING
            && (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L)) return 1;
	return 0;
}

void is_in_row(const struct fpga_model* model, int y,
	int* row_num, int* row_pos)
{
	int dist_to_center;

	if (row_num) *row_num = -1;
	if (row_pos) *row_pos = -1;
	if (y < 2) return;
	// normalize y to beginning of rows
	y -= 2;

	// calculate distance to center and check
	// that y is not pointing to the center
	dist_to_center = (model->cfg_rows/2)*(8+1/*middle of row*/+8);
	if (y == dist_to_center) return;
	if (y > dist_to_center) y--;

	// check that y is not pointing past the last row
	if (y >= model->cfg_rows*(8+1+8)) return;

	if (row_num) *row_num = model->cfg_rows-(y/(8+1+8))-1;
	if (row_pos) *row_pos = y%(8+1+8);
}

int row_num(int y, struct fpga_model* model)
{
	int result;
	is_in_row(model, y, &result, 0 /* row_pos */);
	return result;
}

int row_pos(int y, struct fpga_model* model)
{
	int result;
	is_in_row(model, y, 0 /* row_num */, &result);
	return result;
}

static const char* fpga_ttstr[] = // tile type strings
{
	[NA] = "NA",
	[ROUTING] = "ROUTING",
	[ROUTING_BRK] = "ROUTING_BRK",
	[ROUTING_VIA] = "ROUTING_VIA",
	[HCLK_ROUTING_XM] = "HCLK_ROUTING_XM",
	[HCLK_ROUTING_XL] = "HCLK_ROUTING_XL",
	[HCLK_LOGIC_XM] = "HCLK_LOGIC_XM",
	[HCLK_LOGIC_XL] = "HCLK_LOGIC_XL",
	[LOGIC_XM] = "LOGIC_XM",
	[LOGIC_XL] = "LOGIC_XL",
	[REGH_ROUTING_XM] = "REGH_ROUTING_XM",
	[REGH_ROUTING_XL] = "REGH_ROUTING_XL",
	[REGH_LOGIC_XM] = "REGH_LOGIC_XM",
	[REGH_LOGIC_XL] = "REGH_LOGIC_XL",
	[BRAM_ROUTING] = "BRAM_ROUTING",
	[BRAM_ROUTING_BRK] = "BRAM_ROUTING_BRK",
	[BRAM] = "BRAM",
	[BRAM_ROUTING_TERM_T] = "BRAM_ROUTING_TERM_T",
	[BRAM_ROUTING_TERM_B] = "BRAM_ROUTING_TERM_B",
	[BRAM_ROUTING_VIA_TERM_T] = "BRAM_ROUTING_VIA_TERM_T",
	[BRAM_ROUTING_VIA_TERM_B] = "BRAM_ROUTING_VIA_TERM_B",
	[BRAM_TERM_LT] = "BRAM_TERM_LT",
	[BRAM_TERM_RT] = "BRAM_TERM_RT",
	[BRAM_TERM_LB] = "BRAM_TERM_LB",
	[BRAM_TERM_RB] = "BRAM_TERM_RB",
	[HCLK_BRAM_ROUTING] = "HCLK_BRAM_ROUTING",
	[HCLK_BRAM_ROUTING_VIA] = "HCLK_BRAM_ROUTING_VIA",
	[HCLK_BRAM] = "HCLK_BRAM",
	[REGH_BRAM_ROUTING] = "REGH_BRAM_ROUTING",
	[REGH_BRAM_ROUTING_VIA] = "REGH_BRAM_ROUTING_VIA",
	[REGH_BRAM_L] = "REGH_BRAM_L",
	[REGH_BRAM_R] = "REGH_BRAM_R",
	[MACC] = "MACC",
	[HCLK_MACC_ROUTING] = "HCLK_MACC_ROUTING",
	[HCLK_MACC_ROUTING_VIA] = "HCLK_MACC_ROUTING_VIA",
	[HCLK_MACC] = "HCLK_MACC",
	[REGH_MACC_ROUTING] = "REGH_MACC_ROUTING",
	[REGH_MACC_ROUTING_VIA] = "REGH_MACC_ROUTING_VIA",
	[REGH_MACC_L] = "REGH_MACC_L",
	[PLL_T] = "PLL_T",
	[DCM_T] = "DCM_T",
	[PLL_B] = "PLL_B",
	[DCM_B] = "DCM_B",
	[REG_T] = "REG_T",
	[REG_TERM_T] = "REG_TERM_T",
	[REG_TERM_B] = "REG_TERM_B",
	[REG_B] = "REG_B",
	[REGV_TERM_T] = "REGV_TERM_T",
	[REGV_TERM_B] = "REGV_TERM_B",
	[HCLK_REGV] = "HCLK_REGV",
	[REGV] = "REGV",
	[REGV_BRK] = "REGV_BRK",
	[REGV_T] = "REGV_T",
	[REGV_B] = "REGV_B",
	[REGV_MIDBUF_T] = "REGV_MIDBUF_T",
	[REGV_HCLKBUF_T] = "REGV_HCLKBUF_T",
	[REGV_HCLKBUF_B] = "REGV_HCLKBUF_B",
	[REGV_MIDBUF_B] = "REGV_MIDBUF_B",
	[REGC_ROUTING] = "REGC_ROUTING",
	[REGC_LOGIC] = "REGC_LOGIC",
	[REGC_CMT] = "REGC_CMT",
	[CENTER] = "CENTER",
	[IO_T] = "IO_T",
	[IO_B] = "IO_B",
	[IO_TERM_T] = "IO_TERM_T",
	[IO_TERM_B] = "IO_TERM_B",
	[IO_ROUTING] = "IO_ROUTING",
	[IO_LOGIC_TERM_T] = "IO_LOGIC_TERM_T",
	[IO_LOGIC_TERM_B] = "IO_LOGIC_TERM_B",
	[IO_OUTER_T] = "IO_OUTER_T",
	[IO_INNER_T] = "IO_INNER_T",
	[IO_OUTER_B] = "IO_OUTER_B",
	[IO_INNER_B] = "IO_INNER_B",
	[IO_BUFPLL_TERM_T] = "IO_BUFPLL_TERM_T",
	[IO_LOGIC_REG_TERM_T] = "IO_LOGIC_REG_TERM_T",
	[IO_BUFPLL_TERM_B] = "IO_BUFPLL_TERM_B",
	[IO_LOGIC_REG_TERM_B] = "IO_LOGIC_REG_TERM_B",
	[LOGIC_ROUTING_TERM_B] = "LOGIC_ROUTING_TERM_B",
	[LOGIC_NOIO_TERM_B] = "LOGIC_NOIO_TERM_B",
	[MACC_ROUTING_TERM_T] = "MACC_ROUTING_TERM_T",
	[MACC_ROUTING_TERM_B] = "MACC_ROUTING_TERM_B",
	[MACC_VIA_TERM_T] = "MACC_VIA_TERM_T",
	[MACC_TERM_TL] = "MACC_TERM_TL",
	[MACC_TERM_TR] = "MACC_TERM_TR",
	[MACC_TERM_BL] = "MACC_TERM_BL",
	[MACC_TERM_BR] = "MACC_TERM_BR",
	[ROUTING_VIA_REGC] = "ROUTING_VIA_REGC",
	[ROUTING_VIA_IO] = "ROUTING_VIA_IO",
	[ROUTING_VIA_IO_DCM] = "ROUTING_VIA_IO_DCM",
	[ROUTING_VIA_CARRY] = "ROUTING_VIA_CARRY",
	[CORNER_TERM_L] = "CORNER_TERM_L",
	[CORNER_TERM_R] = "CORNER_TERM_R",
	[IO_TERM_L_UPPER_TOP] = "IO_TERM_L_UPPER_TOP",
	[IO_TERM_L_UPPER_BOT] = "IO_TERM_L_UPPER_BOT",
	[IO_TERM_L_LOWER_TOP] = "IO_TERM_L_LOWER_TOP",
	[IO_TERM_L_LOWER_BOT] = "IO_TERM_L_LOWER_BOT",
	[IO_TERM_R_UPPER_TOP] = "IO_TERM_R_UPPER_TOP",
	[IO_TERM_R_UPPER_BOT] = "IO_TERM_R_UPPER_BOT",
	[IO_TERM_R_LOWER_TOP] = "IO_TERM_R_LOWER_TOP",
	[IO_TERM_R_LOWER_BOT] = "IO_TERM_R_LOWER_BOT",
	[IO_TERM_L] = "IO_TERM_L",
	[IO_TERM_R] = "IO_TERM_R",
	[HCLK_TERM_L] = "HCLK_TERM_L",
	[HCLK_TERM_R] = "HCLK_TERM_R",
	[REGH_IO_TERM_L] = "REGH_IO_TERM_L",
	[REGH_IO_TERM_R] = "REGH_IO_TERM_R",
	[REG_L] = "REG_L",
	[REG_R] = "REG_R",
	[IO_PCI_L] = "IO_PCI_L",
	[IO_PCI_R] = "IO_PCI_R",
	[IO_RDY_L] = "IO_RDY_L",
	[IO_RDY_R] = "IO_RDY_R",
	[IO_L] = "IO_L",
	[IO_R] = "IO_R",
	[IO_PCI_CONN_L] = "IO_PCI_CONN_L",
	[IO_PCI_CONN_R] = "IO_PCI_CONN_R",
	[CORNER_TERM_T] = "CORNER_TERM_T",
	[CORNER_TERM_B] = "CORNER_TERM_B",
	[ROUTING_IO_L] = "ROUTING_IO_L",
	[HCLK_ROUTING_IO_L] = "HCLK_ROUTING_IO_L",
	[HCLK_ROUTING_IO_R] = "HCLK_ROUTING_IO_R",
	[REGH_ROUTING_IO_L] = "REGH_ROUTING_IO_L",
	[REGH_ROUTING_IO_R] = "REGH_ROUTING_IO_R",
	[ROUTING_IO_L_BRK] = "ROUTING_IO_L_BRK",
	[ROUTING_GCLK] = "ROUTING_GCLK",
	[REGH_IO_L] = "REGH_IO_L",
	[REGH_IO_R] = "REGH_IO_R",
	[REGH_MCB] = "REGH_MCB",
	[HCLK_MCB] = "HCLK_MCB",
	[ROUTING_IO_VIA_L] = "ROUTING_IO_VIA_L",
	[ROUTING_IO_VIA_R] = "ROUTING_IO_VIA_R",
	[ROUTING_IO_PCI_CE_L] = "ROUTING_IO_PCI_CE_L",
	[ROUTING_IO_PCI_CE_R] = "ROUTING_IO_PCI_CE_R",
	[CORNER_TL] = "CORNER_TL",
	[CORNER_BL] = "CORNER_BL",
	[CORNER_TR_UPPER] = "CORNER_TR_UPPER",
	[CORNER_TR_LOWER] = "CORNER_TR_LOWER",
	[CORNER_BR_UPPER] = "CORNER_BR_UPPER",
	[CORNER_BR_LOWER] = "CORNER_BR_LOWER",
	[HCLK_IO_TOP_UP_L] = "HCLK_IO_TOP_UP_L",
	[HCLK_IO_TOP_UP_R] = "HCLK_IO_TOP_UP_R",
	[HCLK_IO_TOP_SPLIT_L] = "HCLK_IO_TOP_SPLIT_L",
	[HCLK_IO_TOP_SPLIT_R] = "HCLK_IO_TOP_SPLIT_R",
	[HCLK_IO_TOP_DN_L] = "HCLK_IO_TOP_DN_L",
	[HCLK_IO_TOP_DN_R] = "HCLK_IO_TOP_DN_R",
	[HCLK_IO_BOT_UP_L] = "HCLK_IO_BOT_UP_L",
	[HCLK_IO_BOT_UP_R] = "HCLK_IO_BOT_UP_R",
	[HCLK_IO_BOT_SPLIT_L] = "HCLK_IO_BOT_SPLIT_L",
	[HCLK_IO_BOT_SPLIT_R] = "HCLK_IO_BOT_SPLIT_R",
	[HCLK_IO_BOT_DN_L] = "HCLK_IO_BOT_DN_L",
	[HCLK_IO_BOT_DN_R] = "HCLK_IO_BOT_DN_R",
};

const char* fpga_tiletype_str(enum fpga_tile_type type)
{
	if (type >= sizeof(fpga_ttstr)/sizeof(fpga_ttstr[0])
	    || !fpga_ttstr[type]) return "UNK";
	return fpga_ttstr[type];
}
