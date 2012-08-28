//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "helper.h"
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
