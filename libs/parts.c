//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "control.h"
#include "parts.h"

const char* iob_xc6slx9_sitenames[IOB_WORDS*2/8] =
{
	[0x0000/8]
		"P70", "P69", "P67", "P66", "P65", "P64", "P62", "P61",
		"P60", "P59", "P58", "P57", 0, 0, 0, 0,
	[0x0080/8]
		0, 0, "P56", "P55", 0, 0, 0, 0,
		0, 0, "P51", "P50", 0, 0, 0, 0,
	[0x0100/8]
		0, 0, 0, 0, "UNB131", "UNB132", "P48", "P47",
		"P46", "P45", "P44", "P43", 0, 0, "P41", "P40",
	[0x0180/8]
		"P39", "P38", "P35", "P34", "P33", "P32", 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
	[0x0200/8]
		"P30", "P29", "P27", "P26", 0, 0, 0, 0,
		0, 0, "P24", "P23", "P22", "P21", 0, 0,
	[0x0280/8]
		0, 0, 0, 0, "P17", "P16", "P15", "P14",
		0, 0, 0, 0, 0, 0, 0, 0,
	[0x0300/8]
		"P12", "P11", "P10", "P9", "P8", "P7", "P6", "P5",
		0, 0, 0, 0, 0, 0, "P2", "P1",
	[0x0380/8]
		"P144", "P143", "P142", "P141", "P140", "P139", "P138", "P137",
		0, 0, 0, 0, 0, 0, 0, 0,
	[0x0400/8]
		0, 0, 0, 0, "P134", "P133", "P132", "P131",
		0, 0, 0, 0, 0, 0, "P127", "P126",
	[0x0480/8]
		"P124", "P123", 0, 0, 0, 0, 0, 0,
		"P121", "P120", "P119", "P118", "P117", "P116", "P115", "P114",
	[0x0500/8]
		"P112", "P111", "P105", "P104", 0, 0, 0, 0,
		0, 0, "P102", "P101", "P99", "P98", "P97", 0,
	[0x0580/8]
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, "P95", "P94", "P93", "P92", 0, 0,
	[0x0600/8]
		0, 0, 0, "P88", "P87", 0, "P85", "P84",
		0, 0, "P83", "P82", "P81", "P80", "P79", "P78",
	[0x0680/8]
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, "P75", "P74"
};

int get_num_iobs(int idcode)
{
	if ((idcode & IDCODE_MASK) != XC6SLX9)
		EXIT(1);
	return sizeof(iob_xc6slx9_sitenames)/sizeof(iob_xc6slx9_sitenames[0]);
}

const char* get_iob_sitename(int idcode, int idx)
{
	if ((idcode & IDCODE_MASK) != XC6SLX9)
		EXIT(1);
	if (idx < 0 || idx > sizeof(iob_xc6slx9_sitenames)/sizeof(iob_xc6slx9_sitenames[0]))
		EXIT(1);
	return iob_xc6slx9_sitenames[idx];
}

int find_iob_sitename(int idcode, const char* name)
{
	int i;

	if ((idcode & IDCODE_MASK) != XC6SLX9) {
		HERE();
		return -1;
	}
	for (i = 0; i < sizeof(iob_xc6slx9_sitenames)
			/sizeof(iob_xc6slx9_sitenames[0]); i++) {
		if (iob_xc6slx9_sitenames[i]
		    && !strcmp(iob_xc6slx9_sitenames[i], name))
			return i;
	}
	return -1;
}

int get_major_minors(int idcode, int major)
{
	static const int minors_per_major[] = // for slx9
	{
		/*  0 */	 4, // 505 bytes = middle 8-bit for each minor?
		/*  1 */ 	30, // left
		/*  2 */ 	31, // logic M
		/*  3 */ 	30, // logic L
		/*  4 */ 	25, // bram
		/*  5 */ 	31, // logic M
		/*  6 */ 	30, // logic L
		/*  7 */ 	24, // macc
		/*  8 */ 	31, // logic M
		/*  9 */ 	31, // center
		/* 10 */ 	31, // logic M
		/* 11 */ 	30, // logic L
		/* 12 */ 	31, // logic M
		/* 13 */ 	30, // logic L
		/* 14 */ 	25, // bram
		/* 15 */ 	31, // logic M
		/* 16 */ 	30, // logic L
		/* 17 */	30, // right
	};
	if ((idcode & IDCODE_MASK) != XC6SLX9)
		EXIT(1);
	if (major < 0 || major
		> sizeof(minors_per_major)/sizeof(minors_per_major[0]))
		EXIT(1);
	return minors_per_major[major];
}

enum major_type get_major_type(int idcode, int major)
{
	static const int major_types[] = // for slx9
	{
		/*  0 */ MAJ_ZERO,
		/*  1 */ MAJ_LEFT,
		/*  2 */ MAJ_LOGIC_XM,
		/*  3 */ MAJ_LOGIC_XL,
		/*  4 */ MAJ_BRAM,
		/*  5 */ MAJ_LOGIC_XM,
		/*  6 */ MAJ_LOGIC_XL,
		/*  7 */ MAJ_MACC,
		/*  8 */ MAJ_LOGIC_XM,
		/*  9 */ MAJ_CENTER,
		/* 10 */ MAJ_LOGIC_XM,
		/* 11 */ MAJ_LOGIC_XL,
		/* 12 */ MAJ_LOGIC_XM,
		/* 13 */ MAJ_LOGIC_XL,
		/* 14 */ MAJ_BRAM,
		/* 15 */ MAJ_LOGIC_XM,
		/* 16 */ MAJ_LOGIC_XL,
		/* 17 */ MAJ_RIGHT
	};
	if ((idcode & IDCODE_MASK) != XC6SLX9)
		EXIT(1);
	if (major < 0 || major
		> sizeof(major_types)/sizeof(major_types[0]))
		EXIT(1);
	return major_types[major];
}

int get_rightside_major(int idcode)
{
	if ((idcode & IDCODE_MASK) != XC6SLX9) {
		HERE();
		return -1;
	}
	return XC6_SLX9_RIGHTMOST_MAJOR;
}

int get_major_framestart(int idcode, int major)
{
	int i, frame_count;

	frame_count = 0;
	for (i = 0; i < major; i++)
		frame_count += get_major_minors(idcode, i);
	return frame_count;
}

int get_frames_per_row(int idcode)
{
	return get_major_framestart(idcode, get_rightside_major(idcode)+1);
}

//
// routing switches
//

struct sw_mip_src
{
	int minor;

	int m0_sw_to;
	int m0_two_bits_o;
	int m0_two_bits_val;
	int m0_one_bit_start;

	int m1_sw_to;
	int m1_two_bits_o;
	int m1_two_bits_val;
	int m1_one_bit_start;

	int from_w[6];
};

// returns:
// 1 for the active side of a bidir switch, where the bits reside
// 0 for a unidirectional switch
// -1 for the passive side of a bidir switch, where no bits reside
static int bidir_check(int sw_to, int sw_from)
{
	// the first member of bidir switch pairs is where the bits reside
	static const int bidir[] = {
		LW + (LI_BX|LD1),	FAN_B,
		LW + (LI_AX|LD1),	GFAN0,
		LW + LI_AX,		GFAN0,
		LW + (LI_CE|LD1),	GFAN1,
		LW + (LI_CI|LD1),	GFAN1,
		LW + LI_BX,		LW + (LI_CI|LD1),
		LW + LI_BX,		LW + (LI_DI|LD1),
		LW + (LI_AX|LD1),	LW + (LI_CI|LD1),
		LW + (LI_BX|LD1),	LW + (LI_CE|LD1),
		LW + LI_AX,		LW + (LI_CE|LD1) };
	int i;

	// bidirectional switches are ignored on one side, and
	// marked as bidir on the other side
	for (i = 0; i < sizeof(bidir)/sizeof(*bidir)/2; i++) {
		if (sw_from == bidir[i*2] && sw_to == bidir[i*2+1])
			// nothing to do where no bits reside
			return -1;
		if (sw_from == bidir[i*2+1] && sw_to == bidir[i*2])
			return 1;
	}
	return 0;
}

static int wire_decrement(int wire)
{
	int _wire, flags;

	if (wire >= DW && wire <= DW_LAST) {
		_wire = wire - DW;
		flags = _wire & DIR_FLAGS;
		_wire &= ~DIR_FLAGS;

		if (_wire%4 == 0)
			return DW + ((_wire + 3) | flags);
		return DW + ((_wire - 1) | flags);
	}
	if (wire >= LW && wire <= LW_LAST) {
		_wire = wire - LW;
		flags = _wire & LD1;
		_wire &= ~LD1;

		if (_wire == LO_A)
			return LW + (LO_D|flags);
		if (_wire == LO_AMUX)
			return LW + (LO_DMUX|flags);
		if (_wire == LO_AQ)
			return LW + (LO_DQ|flags);
		if ((_wire >= LO_B && _wire <= LO_D)
		    || (_wire >= LO_BMUX && _wire <= LO_DMUX)
		    || (_wire >= LO_BQ && _wire <= LO_DQ))
			return LW + ((_wire-1)|flags);
	}
	if (wire == NO_WIRE) return wire;
	HERE();
	return wire;
}

static enum extra_wires clean_S0N3(enum extra_wires wire)
{
	int flags;

	if (wire < DW || wire > DW_LAST) return wire;
	wire -= DW;
	flags = wire & DIR_FLAGS;
	wire &= ~DIR_FLAGS;
	if (flags & DIR_S0 && wire%4 != 0)
		flags &= ~DIR_S0;
	if (flags & DIR_N3 && wire%4 != 3)
		flags &= ~DIR_N3;
	return DW + (wire | flags);
}

static int src_to_bitpos(struct xc6_routing_bitpos* bitpos, int* cur_el, int max_el,
	const struct sw_mip_src* src, int src_len)
{
	int i, j, bidir, rc;

	for (i = 0; i < src_len; i++) {
		for (j = 0; j < sizeof(src->from_w)/sizeof(src->from_w[0]); j++) {
			if (src[i].from_w[j] == NO_WIRE) continue;

			bidir = bidir_check(src[i].m0_sw_to, src[i].from_w[j]);
			if (bidir != -1) {
				if (*cur_el >= max_el) FAIL(EINVAL);
				bitpos[*cur_el].from = clean_S0N3(src[i].from_w[j]);
				bitpos[*cur_el].to = clean_S0N3(src[i].m0_sw_to);
				bitpos[*cur_el].bidir = bidir;
				bitpos[*cur_el].minor = src[i].minor;
				bitpos[*cur_el].two_bits_o = src[i].m0_two_bits_o;
				bitpos[*cur_el].two_bits_val = src[i].m0_two_bits_val;
				bitpos[*cur_el].one_bit_o = src[i].m0_one_bit_start + j*2;
				(*cur_el)++;
			}

			bidir = bidir_check(src[i].m1_sw_to, src[i].from_w[j]);
			if (bidir != -1) {
				if (*cur_el >= max_el) FAIL(EINVAL);
				bitpos[*cur_el].from = clean_S0N3(src[i].from_w[j]);
				bitpos[*cur_el].to = clean_S0N3(src[i].m1_sw_to);
				bitpos[*cur_el].bidir = bidir;
				bitpos[*cur_el].minor = src[i].minor;
				bitpos[*cur_el].two_bits_o = src[i].m1_two_bits_o;
				bitpos[*cur_el].two_bits_val = src[i].m1_two_bits_val;
				bitpos[*cur_el].one_bit_o = src[i].m1_one_bit_start + j*2;
				(*cur_el)++;
			}
		}
	}
	return 0;
fail:
	return rc;
}

#define LOGIN_ROW 2
#define LOGIN_MIP_ROWS 8
static const int logicin_matrix[] =
{
		  /*mip 12*/
	/* 000 */ LW + (LI_C6|LD1),	LW + LI_D6,
	/* 016 */ LW + (LI_B1|LD1),	LW + (LI_DI|LD1),
	/* 032 */ LW + (LI_C5|LD1),	LW + LI_D5,
	/* 048 */ LW + (LI_CI|LD1),	LW + LI_A2,
	/* 064 */ LW + (LI_C4|LD1),	LW + LI_D4,
	/* 080 */ LW + LI_A1,		LW + LI_CE,
	/* 096 */ LW + (LI_C3|LD1),	LW + LI_D3,
	/* 112 */ LW + (LI_B2|LD1),	LW + (LI_WE|LD1),

		  /*mip 14*/
	/* 000 */ LW + LI_C1,		LW + LI_DX,
	/* 016 */ LW + (LI_A3|LD1),	LW + LI_B3,
	/* 032 */ LW + (LI_CX|LD1),	LW + (LI_D2|LD1),
	/* 048 */ LW + (LI_A4|LD1),	LW + LI_B4,
	/* 064 */ LW + (LI_D1|LD1),	LW + LI_BX,
	/* 080 */ LW + (LI_A5|LD1),	LW + LI_B5,
	/* 096 */ LW + (LI_AX|LD1),	LW + LI_C2,
	/* 112 */ LW + (LI_A6|LD1),	LW + LI_B6,

		  /*mip 16*/
	/* 000 */ LW + (LI_B3|LD1),	LW + LI_A3,
	/* 016 */ LW + (LI_C2|LD1),	LW + (LI_DX|LD1),
	/* 032 */ LW + (LI_B4|LD1),	LW + LI_A4,
	/* 048 */ LW + LI_CX,		LW + LI_D1,
	/* 064 */ LW + (LI_B5|LD1),	LW + LI_A5,
	/* 080 */ LW + (LI_BX|LD1),	LW + LI_D2,
	/* 096 */ LW + (LI_B6|LD1),	LW + LI_A6,
	/* 112 */ LW + (LI_C1|LD1),	LW + LI_AX,

		  /*mip 18*/
	/* 000 */ LW + LI_B2,		FAN_B,
	/* 016 */ LW + (LI_D6|LD1),	LW + LI_C6,
	/* 032 */ LW + (LI_A1|LD1),	LW + (LI_CE|LD1),
	/* 048 */ LW + (LI_D5|LD1),	LW + LI_C5,
	/* 064 */ LW + (LI_A2|LD1),	LW + (LI_BI|LD1),
	/* 080 */ LW + (LI_D4|LD1),	LW + LI_C4,
	/* 096 */ LW + (LI_AI|LD1),	LW + LI_B1,
	/* 112 */ LW + (LI_D3|LD1),	LW + LI_C3
};

static int mip_to_bitpos(struct xc6_routing_bitpos* bitpos, int* cur_el,
	int max_el, int minor, int m0_two_bits_val, int m0_one_bit_start,
	int m1_two_bits_val, int m1_one_bit_start, int (*from_ws)[8][6])
{
	struct sw_mip_src src;
	int i, j, rc;

	src.minor = minor;
	src.m0_two_bits_o = 0;
	src.m0_two_bits_val = m0_two_bits_val;
	src.m0_one_bit_start = m0_one_bit_start;
	src.m1_two_bits_o = 14;
	src.m1_two_bits_val = m1_two_bits_val;
	src.m1_one_bit_start = m1_one_bit_start;
	for (i = 0; i < 8; i++) {
		int logicin_o = ((src.minor-12)/2)*LOGIN_MIP_ROWS*LOGIN_ROW;
		logicin_o += i*LOGIN_ROW;
		src.m0_sw_to = logicin_matrix[logicin_o+0];
		src.m1_sw_to = logicin_matrix[logicin_o+1];
		if (i) {
			src.m0_two_bits_o += 16;
			src.m0_one_bit_start += 16;
			src.m1_two_bits_o += 16;
			src.m1_one_bit_start += 16;
		}
		for (j = 0; j < sizeof(src.from_w)/sizeof(src.from_w[0]); j++)
			src.from_w[j] = (*from_ws)[i][j];

		rc = src_to_bitpos(bitpos, cur_el, max_el, &src, /*src_len*/ 1);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

int get_xc6_routing_bitpos(struct xc6_routing_bitpos** bitpos, int* num_bitpos)
{
	int i, j, k, rc;

	*num_bitpos = 0;
	*bitpos = malloc(MAX_SWITCHBOX_SIZE * sizeof(**bitpos));
	if (!(*bitpos)) FAIL(ENOMEM);

	// mip 0-10 (6*288=1728 switches)
	{ struct sw_mip_src src[] = {
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 2, 2,
			{DW + ((W_SW2*4+2)|DIR_N3), DW + ((W_SS2*4+2)|DIR_N3), DW + ((W_WW2*4+2)|DIR_N3),
			 DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_NW2*4+3}},
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 0, 2,
			{DW + ((W_SW4*4+2)|DIR_N3), DW + ((W_SS4*4+2)|DIR_N3), DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_NW4*4+3, DW + W_WW4*4+3}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SW2*4+3, DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_SS2*4+3, DW + W_SE2*4+3, DW + W_EE2*4+3}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SW4*4+3, DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0),
			 DW + ((W_NW4*4+0)|DIR_S0), DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_EE2*4+3, DW + W_SE2*4+3, DW + ((W_WW2*4+2)|DIR_N3),
			 DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_NW2*4+3}},
		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_NW4*4+3, DW + W_WW4*4+3}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SW2*4+3, DW + W_NN2*4+3, DW + W_NE2*4+3,
			 DW + W_SS2*4+3, DW + W_SE2*4+3, DW + W_EE2*4+3}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SW4*4+3, DW + W_SS4*4+3, DW + W_NN4*4+3,
			 DW + W_NE4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_NW4*4+3, DW + W_WW4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + ((W_WW2*4+2)|DIR_N3), DW + W_NW2*4+3}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + ((W_WR1*4+0)|DIR_S0),
			 DW + W_WL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0), DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0), DW + ((W_NW4*4+0)|DIR_S0)}},

		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_EL1*4+3,
			 DW + W_ER1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + W_ER1*4+3,
			 DW + W_EL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SE2*4+3, DW + W_EE2*4+3, DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 0, 2,
			{DW + W_NW4*4+3, DW + W_WW4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + ((W_WW2*4+2)|DIR_N3), DW + W_NW2*4+3}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 2, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + ((W_WR1*4+0)|DIR_S0),
			 DW + W_WL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 1, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 0, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 0, 19,
			{DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0), DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0), DW + ((W_NW4*4+0)|DIR_S0)}},

		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 2, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX,	     LW + LO_DQ,       LW + LO_D}},
		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 1, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_EL1*4+3,
			 DW + W_ER1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 0, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 2, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + W_ER1*4+3,
			 DW + W_EL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 1, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 2, 19,
			{LW + LO_D,       LW + LO_DQ,       LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 0, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 0, 19,
			{DW + W_SE2*4+3, DW + W_EE2*4+3, DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}}};

		for (i = 0;; i++) {
			rc = src_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
				src, sizeof(src)/sizeof(*src));
			if (rc) FAIL(rc);
			if (i >= 3) break;
			for (j = 0; j < sizeof(src)/sizeof(*src); j++) {
				src[j].m0_sw_to = wire_decrement(src[j].m0_sw_to);
				src[j].m0_two_bits_o += 32;
				src[j].m0_one_bit_start += 32;
				src[j].m1_sw_to = wire_decrement(src[j].m1_sw_to);
				src[j].m1_two_bits_o += 32;
				src[j].m1_one_bit_start += 32;
				for (k = 0; k < sizeof(src[0].from_w)/sizeof(src[0].from_w[0]); k++)
					src[j].from_w[k] = wire_decrement(src[j].from_w[k]);
			}
		}
	}

	// mip 12-18, decrementing directional wires (1024 switches)
	{ struct sw_mip_src src[] = {
		{12, NO_WIRE, 0, 2, 2,
		     NO_WIRE, 14, 2, 3,
			{DW + W_EL1*4+3, DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{12, NO_WIRE, 0, 0, 2,
		     NO_WIRE, 14, 0, 3,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_WW2*4+3, DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{12, NO_WIRE, 0, 1, 2,
		     NO_WIRE, 14, 1, 3,
			{NO_WIRE, NO_WIRE, DW + ((W_NL1*4+0)|DIR_S0),
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + W_SR1*4+3}},

		{14, NO_WIRE, 0, 1, 3,
		     NO_WIRE, 14, 1, 2,
			{DW + ((W_EL1*4+0)|DIR_S0), DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + ((W_WR1*4+0)|DIR_S0), DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{14, NO_WIRE, 0, 0, 3,
		     NO_WIRE, 14, 0, 2,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_WW2*4+3, DW + ((W_NE2*4+0)|DIR_S0), DW + ((W_NN2*4+0)|DIR_S0)}},
		{14, NO_WIRE, 0, 2, 3,
		     NO_WIRE, 14, 2, 2,
			{NO_WIRE, NO_WIRE, DW + ((W_NL1*4+0)|DIR_S0),
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + W_SR1*4+3}},

		{16, NO_WIRE, 0, 2, 2,
		     NO_WIRE, 14, 2, 3,
			{DW + W_EL1*4+3, DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{16, NO_WIRE, 0, 0, 2,
		     NO_WIRE, 14, 0, 3,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + W_NW2*4+3,
			 DW + ((W_WW2*4+2)|DIR_N3), DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{16, NO_WIRE, 0, 1, 2,
		     NO_WIRE, 14, 1, 3,
			{NO_WIRE, NO_WIRE, DW + W_NL1*4+3,
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + ((W_SR1*4+2)|DIR_N3)}},

		{18, NO_WIRE, 0, 1, 3,
		     NO_WIRE, 14, 1, 2,
			{DW + W_EL1*4+3, DW + ((W_ER1*4+2)|DIR_N3), DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{18, NO_WIRE, 0, 0, 3,
		     NO_WIRE, 14, 0, 2,
			{DW + ((W_SS2*4+2)|DIR_N3), DW + ((W_SW2*4+2)|DIR_N3), DW + W_NW2*4+3,
			 DW + ((W_WW2*4+2)|DIR_N3), DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{18, NO_WIRE, 0, 2, 3,
		     NO_WIRE, 14, 2, 2,
			{NO_WIRE, NO_WIRE, DW + W_NL1*4+3,
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + ((W_SR1*4+2)|DIR_N3)}}};

		for (i = 0; i < 8; i++) {
			for (j = 0; j < sizeof(src)/sizeof(*src); j++) {

				int logicin_o = ((src[j].minor-12)/2)*LOGIN_MIP_ROWS*LOGIN_ROW;
				logicin_o += i*LOGIN_ROW;

				src[j].m0_sw_to = logicin_matrix[logicin_o+0];
				src[j].m1_sw_to = logicin_matrix[logicin_o+1];

				if (i) {
					src[j].m0_two_bits_o += 16;
					src[j].m0_one_bit_start += 16;
					src[j].m1_two_bits_o += 16;
					src[j].m1_one_bit_start += 16;
					if (!(i%2)) // at 2, 4 and 6 we decrement the wires
						for (k = 0; k < sizeof(src[0].from_w)/sizeof(src[0].from_w[0]); k++)
							src[j].from_w[k] = wire_decrement(src[j].from_w[k]);
				}
			}
			rc = src_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
				src, sizeof(src)/sizeof(*src));
			if (rc) FAIL(rc);
		}
	}

	// VCC/GND/GFAN, logicin and logicout sources
	// mip12-14
	{ int logicin_src[8][6] = {
		{VCC_WIRE, LW + (LO_D|LD1), LW + LO_DQ,       LW + (LO_BMUX|LD1), LOGICIN_S62,      LOGICIN_S20},
		{GFAN1,    LW + (LO_D|LD1), LW + LO_DQ,       LW + (LO_BMUX|LD1), LOGICIN_S62,      LOGICIN_S20},
		{VCC_WIRE, LW + LO_C,       LW + (LO_CQ|LD1), LW + LO_AMUX,       LOGICIN_S62,      LW + (LI_AX|LD1)},
		{GFAN1,    LW + LO_C,       LW + (LO_CQ|LD1), LW + LO_AMUX,       LOGICIN_S62,      LW + (LI_AX|LD1)},
		{VCC_WIRE, LW + (LO_B|LD1), LW + LO_BQ,       LW + (LO_DMUX|LD1), LW + (LI_AX|LD1), LW + (LI_CI|LD1)},
		{GFAN0,    LW + (LO_B|LD1), LW + LO_BQ,       LW + (LO_DMUX|LD1), LW + (LI_AX|LD1), LW + (LI_CI|LD1)},
		{VCC_WIRE, LW + LO_A,       LW + (LO_AQ|LD1), LW + LO_CMUX,       LOGICIN20,        LW + (LI_CI|LD1)},
		{GFAN0,    LW + LO_A,       LW + (LO_AQ|LD1), LW + LO_CMUX,       LOGICIN20,        LW + (LI_CI|LD1)},
		};

		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			12, 3, 2, 3, 3, &logicin_src);
		if (rc) FAIL(rc);

		logicin_src[1][0] = logicin_src[3][0] = logicin_src[5][0] = logicin_src[7][0] = VCC_WIRE;
		logicin_src[0][0] = logicin_src[2][0] = GFAN1;
		logicin_src[4][0] = logicin_src[6][0] = GFAN0;
		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			14, 3, 3, 3, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	{ int logicin_src[8][6] = {
		{ LW + LI_BX,       LOGICIN52 },
		{ LW + LI_BX,       LOGICIN52 },
		{ LW + LI_BX,       LW + (LI_DI|LD1) },
		{ LW + LI_BX,       LW + (LI_DI|LD1) },
		{ LW + (LI_DI|LD1), LOGICIN_N28 },
		{ LW + (LI_DI|LD1), LOGICIN_N28 },
		{ LOGICIN_N52,      LOGICIN_N28 },
		{ LOGICIN_N52,      LOGICIN_N28 }};

		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			12, 1, 2, 1, 3, &logicin_src);
		if (rc) FAIL(rc);
		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			14, 2, 3, 2, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	// mip16-18
	{ int logicin_src[8][6] = {
		{VCC_WIRE, LW + LO_D,       LW + (LO_DQ|LD1), LW + LO_BMUX,       LOGICIN_S36, LOGICIN_S44},
		{GFAN1,    LW + LO_D,       LW + (LO_DQ|LD1), LW + LO_BMUX,       LOGICIN_S36, LOGICIN_S44},
		{VCC_WIRE, LW + (LO_C|LD1), LW + LO_CQ,       LW + (LO_AMUX|LD1), LOGICIN_S36, LW + LI_AX},
		{GFAN1,    LW + (LO_C|LD1), LW + LO_CQ,       LW + (LO_AMUX|LD1), LOGICIN_S36, LW + LI_AX},
		{VCC_WIRE, LW + LO_B,       LW + (LO_BQ|LD1), LW + LO_DMUX,       LW + LI_AX,  LW + (LI_CE|LD1)},
		{GFAN0,    LW + LO_B,       LW + (LO_BQ|LD1), LW + LO_DMUX,       LW + LI_AX,  LW + (LI_CE|LD1)},
		{VCC_WIRE, LW + (LO_A|LD1), LW + LO_AQ,       LW + (LO_CMUX|LD1), LOGICIN44,   LW + (LI_CE|LD1)},
		{GFAN0,    LW + (LO_A|LD1), LW + LO_AQ,       LW + (LO_CMUX|LD1), LOGICIN44,   LW + (LI_CE|LD1)},
		};

		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			16, 3, 2, 3, 3, &logicin_src);
		if (rc) FAIL(rc);

		logicin_src[1][0] = logicin_src[3][0] = logicin_src[5][0] = logicin_src[7][0] = VCC_WIRE;
		logicin_src[0][0] = logicin_src[2][0] = GFAN1;
		logicin_src[4][0] = logicin_src[6][0] = GFAN0;
		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			18, 3, 3, 3, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	{ int logicin_src[8][6] = {
		{ LW + (LI_BX|LD1),     LOGICIN21 },
		{ LW + (LI_BX|LD1),     LOGICIN21 },
		{ LW + (LI_BX|LD1),     FAN_B },
		{ LW + (LI_BX|LD1),     FAN_B },
		{ FAN_B,		LOGICIN_N60 },
		{ FAN_B,		LOGICIN_N60 },
		{ LOGICIN_N21,		LOGICIN_N60 },
		{ LOGICIN_N21,		LOGICIN_N60 }};

		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			16, 1, 2, 1, 3, &logicin_src);
		if (rc) FAIL(rc);
		rc = mip_to_bitpos(*bitpos, num_bitpos, MAX_SWITCHBOX_SIZE,
			18, 2, 3, 2, 2, &logicin_src);
		if (rc) FAIL(rc);
	}

	// minor 20 switches (SR, CLK, GFAN = 113 switches (4 bidir added on other side))
	{ const struct sw_mip_src src[] = {
		{20, SR1, 6, 3, 0, .from_w =
			{GCLK11, GCLK10, GCLK13, GCLK12, GCLK9, GCLK8}},
		{20, SR1, 6, 2, 0, .from_w =
			{DW+W_WR1*4+2, DW+W_NR1*4+2, VCC_WIRE, GND_WIRE, DW+W_ER1*4+2, DW+W_SR1*4+2}},
		{20, SR1, 6, 1, 0, .from_w =
			{FAN_B, LW+(LI_DI|LD1), LW+(LI_BX|LD1), LW+LI_BX, GCLK15, GCLK14}},

		{20, SR0, 8, 3, 10, .from_w =
			{GCLK8, GCLK9, GCLK10, GCLK13, GCLK12, GCLK11}},
		{20, SR0, 8, 2, 10, .from_w =
			{GCLK14, GCLK15, LW+(LI_DI|LD1), LW+(LI_BX|LD1), LW+LI_BX, FAN_B}},
		{20, SR0, 8, 1, 10, .from_w = {DW+W_SR1*4+2, DW+W_ER1*4+2, DW+W_NR1*4+2,
			VCC_WIRE, NO_WIRE, DW+W_WR1*4+2}},

		{20, CLK0, 16, 3, 18, .from_w =
			{GCLK0, GCLK1, GCLK2, GCLK5, GCLK4, GCLK3}},
		{20, CLK0, 16, 2, 18, .from_w =
			{GCLK6, GCLK7, GCLK8, GCLK11, GCLK10, GCLK9}},
		{20, CLK0, 16, 1, 18, .from_w =
			{GCLK12, GCLK13, GCLK14, LW+(LI_BX|LD1), LW+(LI_CI|LD1), GCLK15}},
		{20, CLK0, 16, 0, 18, .from_w =
			{DW+W_NR1*4+2, DW+W_WR1*4+2, DW+W_SR1*4+1, VCC_WIRE, NO_WIRE, DW+W_ER1*4+1}},

		{20, CLK1, 46, 3, 40, .from_w =
			{GCLK3, GCLK2, GCLK5, GCLK4, GCLK1, GCLK0}},
		{20, CLK1, 46, 2, 40, .from_w =
			{GCLK15, GCLK14, LW+(LI_BX|LD1), LW+(LI_CI|LD1), GCLK13, GCLK12}},
		{20, CLK1, 46, 1, 40, .from_w =
			{GCLK9, GCLK8, GCLK11, GCLK10, GCLK7, GCLK6}},
		{20, CLK1, 46, 0, 40, .from_w =
			{DW+W_ER1*4+1, DW+W_SR1*4+1, VCC_WIRE, NO_WIRE, DW+W_WR1*4+2, DW+W_NR1*4+2}},

		{20, GFAN0, 54, 3, 48, .from_w =
			{GCLK3, GCLK4, GCLK5, GCLK2, GCLK1, GCLK0}},
		{20, GFAN0, 54, 2, 48, .from_w =
			{DW+W_WR1*4+1, GND_WIRE, VCC_WIRE, DW+W_NR1*4+1, DW+W_ER1*4+1, DW+W_SR1*4+1}},
		{20, GFAN0, 54, 1, 48, .from_w =
			{LW+(LI_CE|LD1), NO_WIRE, NO_WIRE, LW+(LI_CI|LD1), GCLK7, GCLK6}},

		{20, GFAN1, 56, 3, 58, .from_w =
			{GCLK0, GCLK1, GCLK4, GCLK5, GCLK2, GCLK3}},
		{20, GFAN1, 56, 2, 58, .from_w =
			{GCLK6, GCLK7, LW+(LI_AX|LD1), LW+LI_AX, NO_WIRE, NO_WIRE}},
		{20, GFAN1, 56, 1, 58, .from_w =
			{DW+W_SR1*4+1, DW+W_ER1*4+1, GND_WIRE, VCC_WIRE, DW+W_NR1*4+1, DW+W_WR1*4+1}}};

	for (i = 0; i < sizeof(src)/sizeof(*src); i++) {
		for (j = 0; j < sizeof(src[0].from_w)/sizeof(src[0].from_w[0]); j++) {
			if (src[i].from_w[j] == NO_WIRE) continue;

			if (*num_bitpos >= MAX_SWITCHBOX_SIZE) FAIL(EINVAL);
			(*bitpos)[*num_bitpos].from = src[i].from_w[j];
			(*bitpos)[*num_bitpos].to = src[i].m0_sw_to;
			(*bitpos)[*num_bitpos].bidir = 0;
			(*bitpos)[*num_bitpos].minor = 20;
			(*bitpos)[*num_bitpos].two_bits_o = src[i].m0_two_bits_o;
			(*bitpos)[*num_bitpos].two_bits_val = src[i].m0_two_bits_val;
			(*bitpos)[*num_bitpos].one_bit_o = src[i].m0_one_bit_start + j;
			(*num_bitpos)++;
		}
	}}
	return 0;
fail:
	return rc;
}

void free_xc6_routing_bitpos(struct xc6_routing_bitpos* bitpos)
{
	free(bitpos);
}

void xc6_lut_bitmap(int lut_pos, int* map, int num_bits)
{
	static const int xc6_lut_wiring[4][16] = {
	// xm-m a; xm-m c;
	  { 17, 19, 16, 18, 23, 21, 22, 20, 31, 29, 30, 28, 25, 27, 24, 26 },
	// xm-m b; xm-m d;
	  { 47, 45, 46, 44, 41, 43, 40, 42, 33, 35, 32, 34, 39, 37, 38, 36 },
	// xm-x a; xm-x b; xl-l b; xl-l d; xl-x a; xl-x b;
	  { 31, 29, 30, 28, 27, 25, 26, 24, 19, 17, 18, 16, 23, 21, 22, 20 },
	// xm-x c; xm-x d; xl-l a; xl-l c; xl-x c; xl-x d;
	  { 33, 35, 32, 34, 37, 39, 36, 38, 45, 47, 44, 46, 41, 43, 40, 42 }};

	int map32[32];
	int i;

	// expand from 16 to 32 bit positions
	for (i = 0; i < 16; i++) {
		map32[i] = xc6_lut_wiring[lut_pos][i];
		if (map32[i] < 32) {
			if (map32[i] < 16) HERE();
			map32[16+i] = map32[i]-16;
		} else {
			if (map32[i] > 47) HERE();
			map32[16+i] = map32[i]+16;
		}
	}
	// expand from 32 to 64 for either lut6 only or lut5/lut6 pair.
	for (i = 0; i < 32; i++) {
		if (num_bits == 32) {
			map[i] = map32[i]%32;
			map[32+i] = 32+map[i];
		} else {
			if (num_bits != 64) HERE();
			map[i] = map32[i];
			map[32+i] = (map[i]+32)%64;
		}
	}
}
