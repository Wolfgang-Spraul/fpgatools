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
	// Note that the configuration space for 4*6 IOBs
	// that are marked with 0 is used for clocks etc,
	// just not IOBs.
	[0]
		"P70", "P69", "P67", "P66", "P65", "P64", "P62", "P61",
		"P60", "P59", "P58", "P57", "UNB113", "UNB114", "UNB115", "UNB116",
	[16]
		"UNB117", "UNB118", "P56", "P55", 0, 0, 0, 0,
		0, 0, "P51", "P50", "UNB123", "UNB124", "UNB125", "UNB126",
	[32]
		"UNB127", "UNB128", "UNB129", "UNB130", "UNB131", "UNB132", "P48", "P47",
		"P46", "P45", "P44", "P43", "UNB139", "UNB140", "P41", "P40",
	[48]
		"P39", "P38", "P35", "P34", "P33", "P32", "UNB149", "UNB150",
		"UNB151", "UNB152", "UNB153", "UNB154", "UNB155", "UNB156", "UNB157", "UNB158",
	[64]
		"P30", "P29", "P27", "P26", "UNB163", "UNB164", "UNB165", "UNB166",
		"UNB167", "UNB168", "P24", "P23", "P22", "P21", 0, 0,
	[80]
		0, 0, 0, 0, "P17", "P16", "P15", "P14",
		"UNB177", "UNB178", "UNB179", "UNB180", "UNB181", "UNB182", "UNB183", "UNB184",
	[96]
		"P12", "P11", "P10", "P9", "P8", "P7", "P6", "P5",
		"UNB193", "UNB194", "UNB195", "UNB196", "UNB197", "UNB198", "P2", "P1",
	[112]
		"P144", "P143", "P142", "P141", "P140", "P139", "P138", "P137",
		"UNB9", "UNB10", "UNB11", "UNB12", "UNB13", "UNB14", "UNB15", "UNB16",
	[128]
		"UNB17", "UNB18", "UNB19", "UNB20", "P134", "P133", "P132", "P131",
		0, 0, 0, 0, 0, 0, "P127", "P126",
	[144]
		"P124", "P123", "UNB29", "UNB30", "UNB31", "UNB32", "UNB33", "UNB34",
		"P121", "P120", "P119", "P118", "P117", "P116", "P115", "P114",
	[160]
		"P112", "P111", "P105", "P104", "UNB47", "UNB48", "UNB49", "UNB50",
		"UNB51", "UNB52", "P102", "P101", "P100", "P99", "P98", "P97",
	[176]
		"UNB59", "UNB60", "UNB61", "UNB62", "UNB63", "UNB64", "UNB65", "UNB66",
		"UNB67", "UNB68", "P95", "P94", "P93", "P92", 0, 0,
	[192]
		0, 0, 0, 0, "P88", "P87", "P85", "P84",
		"UNB77", "UNB78", "P83", "P82", "P81", "P80", "P79", "P78",
	[208]
		"UNB85", "UNB86", "UNB87", "UNB88", "UNB89", "UNB90", "UNB91", "UNB92",
		"UNB93", "UNB94", "UNB95", "UNB96", "UNB97", "UNB98", "P75", "P74"
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

int xc_num_rows(int idcode)
{
	if ((idcode & IDCODE_MASK) != XC6SLX9) {
		HERE();
		return -1;
	}
	return NUM_ROWS;
}

const struct xc_info* xc_info(int idcode)
{
	static const struct xc_info xc6slx9_info = {
		.idcode = XC6SLX9,
		.num_rows = 4,
		.left_wiring =
			/* row 3 */ "UWUWUWUW" "WWWWUUUU" \
			/* row 2 */ "UUUUUUUU" "WWWWWWUU" \
			/* row 1 */ "WWWUUWUU" "WUUWUUWU" \
			/* row 0 */ "UWUUWUUW" "UUWWWWUU",
		.right_wiring =
			/* row 3 */ "UUWWUWUW" "WWWWUUUU" \
			/* row 2 */ "UUUUUUUU" "WWWWWWUU" \
			/* row 1 */ "WWWUUWUU" "WUUWUUWU" \
			/* row 0 */ "UWUUWUUW" "UUWWWWUU",
		.major_str = "M L Bg M L D M R M Ln M L Bg M L",
		.num_majors = 18,
		.majors = {
			 // maj_zero: 505 bytes = extra 8-bit for each minor?
			 [0] = { XC_MAJ_ZERO, 4 },
			 [1] = { XC_MAJ_LEFT, 30 },
			 [2] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			 [3] = { XC_MAJ_XL | XC_MAJ_TOP_BOT_IO, 30 },
			 [4] = { XC_MAJ_BRAM | XC_MAJ_GCLK_SEP, 25 },
			 [5] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			 [6] = { XC_MAJ_XL | XC_MAJ_TOP_BOT_IO, 30 },
			 [7] = { XC_MAJ_MACC, 24 },
			 [8] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			 [9] = { XC_MAJ_CENTER | XC_MAJ_TOP_BOT_IO, 31 },
			[10] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			[11] = { XC_MAJ_XL, 30 },
			[12] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			[13] = { XC_MAJ_XL | XC_MAJ_TOP_BOT_IO, 30 },
			[14] = { XC_MAJ_BRAM | XC_MAJ_GCLK_SEP, 25 },
			[15] = { XC_MAJ_XM | XC_MAJ_TOP_BOT_IO, 31 },
			[16] = { XC_MAJ_XL | XC_MAJ_TOP_BOT_IO, 30 },
			[17] = { XC_MAJ_RIGHT, 30 }},
		.num_type2 = 224,
		.type2 = {
			  [0] = { XC_T2_IOB_PAD, 70 }, 
			  [1] = { XC_T2_IOB_PAD, 69 },
			  [2] = { XC_T2_IOB_PAD, 67 },
			  [3] = { XC_T2_IOB_PAD, 66 },
			  [4] = { XC_T2_IOB_PAD, 65 },
			  [5] = { XC_T2_IOB_PAD, 64 },
			  [6] = { XC_T2_IOB_PAD, 62 },
			  [7] = { XC_T2_IOB_PAD, 61 },
			  [8] = { XC_T2_IOB_PAD, 60 },
			  [9] = { XC_T2_IOB_PAD, 59 },
			 [10] = { XC_T2_IOB_PAD, 58 },
			 [11] = { XC_T2_IOB_PAD, 57 },
			 [12] = { XC_T2_IOB_UNBONDED, 113 },
			 [13] = { XC_T2_IOB_UNBONDED, 114 },
			 [14] = { XC_T2_IOB_UNBONDED, 115 },
			 [15] = { XC_T2_IOB_UNBONDED, 116 },
			 [16] = { XC_T2_IOB_UNBONDED, 117 },
			 [17] = { XC_T2_IOB_UNBONDED, 118 },
			 [18] = { XC_T2_IOB_PAD, 56 },
			 [19] = { XC_T2_IOB_PAD, 55 },
			 [20] = { XC_T2_CENTER },
			 [21] = { XC_T2_CENTER },
			 [22] = { XC_T2_CENTER },
			 [23] = { XC_T2_CENTER },
			 [24] = { XC_T2_CENTER },
			 [25] = { XC_T2_CENTER },
			 [26] = { XC_T2_IOB_PAD, 51 },
			 [27] = { XC_T2_IOB_PAD, 50 },
			 [28] = { XC_T2_IOB_UNBONDED, 123 },
			 [29] = { XC_T2_IOB_UNBONDED, 124 },
			 [30] = { XC_T2_IOB_UNBONDED, 125 },
			 [31] = { XC_T2_IOB_UNBONDED, 126 },
			 [32] = { XC_T2_IOB_UNBONDED, 127 },
			 [33] = { XC_T2_IOB_UNBONDED, 128 },
			 [34] = { XC_T2_IOB_UNBONDED, 129 },
			 [35] = { XC_T2_IOB_UNBONDED, 130 },
			 [36] = { XC_T2_IOB_UNBONDED, 131 },
			 [37] = { XC_T2_IOB_UNBONDED, 132 },
			 [38] = { XC_T2_IOB_PAD, 48 },
			 [39] = { XC_T2_IOB_PAD, 47 },
			 [40] = { XC_T2_IOB_PAD, 46 },
			 [41] = { XC_T2_IOB_PAD, 45 },
			 [42] = { XC_T2_IOB_PAD, 44 },
			 [43] = { XC_T2_IOB_PAD, 43 },
			 [44] = { XC_T2_IOB_UNBONDED, 139 },
			 [45] = { XC_T2_IOB_UNBONDED, 140 },
			 [46] = { XC_T2_IOB_PAD, 41 },
			 [47] = { XC_T2_IOB_PAD, 40 },
			 [48] = { XC_T2_IOB_PAD, 39 },
			 [49] = { XC_T2_IOB_PAD, 38 },
			 [50] = { XC_T2_IOB_PAD, 35 },
			 [51] = { XC_T2_IOB_PAD, 34 },
			 [52] = { XC_T2_IOB_PAD, 33 },
			 [53] = { XC_T2_IOB_PAD, 32 },
			 [54] = { XC_T2_IOB_UNBONDED, 149 },
			 [55] = { XC_T2_IOB_UNBONDED, 150 },
			 [56] = { XC_T2_IOB_UNBONDED, 151 },
			 [57] = { XC_T2_IOB_UNBONDED, 152 },
			 [58] = { XC_T2_IOB_UNBONDED, 153 },
			 [59] = { XC_T2_IOB_UNBONDED, 154 },
			 [60] = { XC_T2_IOB_UNBONDED, 155 },
			 [61] = { XC_T2_IOB_UNBONDED, 156 },
			 [62] = { XC_T2_IOB_UNBONDED, 157 },
			 [63] = { XC_T2_IOB_UNBONDED, 158 },
			 [64] = { XC_T2_IOB_PAD, 30 },
			 [65] = { XC_T2_IOB_PAD, 29 },
			 [66] = { XC_T2_IOB_PAD, 27 },
			 [67] = { XC_T2_IOB_PAD, 26 },
			 [68] = { XC_T2_IOB_UNBONDED, 163 },
			 [69] = { XC_T2_IOB_UNBONDED, 164 },
			 [70] = { XC_T2_IOB_UNBONDED, 165 },
			 [71] = { XC_T2_IOB_UNBONDED, 166 },
			 [72] = { XC_T2_IOB_UNBONDED, 167 },
			 [73] = { XC_T2_IOB_UNBONDED, 168 },
			 [74] = { XC_T2_IOB_PAD, 24 },
			 [75] = { XC_T2_IOB_PAD, 23 },
			 [76] = { XC_T2_IOB_PAD, 22 },
			 [77] = { XC_T2_IOB_PAD, 21 },
			 [78] = { XC_T2_CENTER },
			 [79] = { XC_T2_CENTER },
			 [80] = { XC_T2_CENTER },
			 [81] = { XC_T2_CENTER },
			 [82] = { XC_T2_CENTER },
			 [83] = { XC_T2_CENTER },
			 [84] = { XC_T2_IOB_PAD, 17 },
			 [85] = { XC_T2_IOB_PAD, 16 },
			 [86] = { XC_T2_IOB_PAD, 15 },
			 [87] = { XC_T2_IOB_PAD, 14 },
			 [88] = { XC_T2_IOB_UNBONDED, 177 },
			 [89] = { XC_T2_IOB_UNBONDED, 178 },
			 [90] = { XC_T2_IOB_UNBONDED, 179 },
			 [91] = { XC_T2_IOB_UNBONDED, 180 },
			 [92] = { XC_T2_IOB_UNBONDED, 181 },
			 [93] = { XC_T2_IOB_UNBONDED, 182 },
			 [94] = { XC_T2_IOB_UNBONDED, 183 },
			 [95] = { XC_T2_IOB_UNBONDED, 184 },
			 [96] = { XC_T2_IOB_PAD, 12 },
			 [97] = { XC_T2_IOB_PAD, 11 },
			 [98] = { XC_T2_IOB_PAD, 10 },
			 [99] = { XC_T2_IOB_PAD, 9 },
			 [100] = { XC_T2_IOB_PAD, 8 },
			 [101] = { XC_T2_IOB_PAD, 7 },
			 [102] = { XC_T2_IOB_PAD, 6 },
			 [103] = { XC_T2_IOB_PAD, 5 },
			 [104] = { XC_T2_IOB_UNBONDED, 193 },
			 [105] = { XC_T2_IOB_UNBONDED, 194 },
			 [106] = { XC_T2_IOB_UNBONDED, 195 },
			 [107] = { XC_T2_IOB_UNBONDED, 196 },
			 [108] = { XC_T2_IOB_UNBONDED, 197 },
			 [109] = { XC_T2_IOB_UNBONDED, 198 },
			 [110] = { XC_T2_IOB_PAD, 2 },
			 [111] = { XC_T2_IOB_PAD, 1 },
			 [112] = { XC_T2_IOB_PAD, 144 },
			 [113] = { XC_T2_IOB_PAD, 143 },
			 [114] = { XC_T2_IOB_PAD, 142 },
			 [115] = { XC_T2_IOB_PAD, 141 },
			 [116] = { XC_T2_IOB_PAD, 140 },
			 [117] = { XC_T2_IOB_PAD, 139 },
			 [118] = { XC_T2_IOB_PAD, 138 },
			 [119] = { XC_T2_IOB_PAD, 137 },
			 [120] = { XC_T2_IOB_UNBONDED, 9 },
			 [121] = { XC_T2_IOB_UNBONDED, 10 },
			 [122] = { XC_T2_IOB_UNBONDED, 11 },
			 [123] = { XC_T2_IOB_UNBONDED, 12 },
			 [124] = { XC_T2_IOB_UNBONDED, 13 },
			 [125] = { XC_T2_IOB_UNBONDED, 14 },
			 [126] = { XC_T2_IOB_UNBONDED, 15 },
			 [127] = { XC_T2_IOB_UNBONDED, 16 },
			 [128] = { XC_T2_IOB_UNBONDED, 17 },
			 [129] = { XC_T2_IOB_UNBONDED, 18 },
			 [130] = { XC_T2_IOB_UNBONDED, 19 },
			 [131] = { XC_T2_IOB_UNBONDED, 20 },
			 [132] = { XC_T2_IOB_PAD, 134 },
			 [133] = { XC_T2_IOB_PAD, 133 },
			 [134] = { XC_T2_IOB_PAD, 132 },
			 [135] = { XC_T2_IOB_PAD, 131 },
			 [136] = { XC_T2_CENTER },
			 [137] = { XC_T2_CENTER },
			 [138] = { XC_T2_CENTER },
			 [139] = { XC_T2_CENTER },
			 [140] = { XC_T2_CENTER },
			 [141] = { XC_T2_CENTER },
			 [142] = { XC_T2_IOB_PAD, 127 },
			 [143] = { XC_T2_IOB_PAD, 126 },
			 [144] = { XC_T2_IOB_PAD, 124 },
			 [145] = { XC_T2_IOB_PAD, 123 },
			 [146] = { XC_T2_IOB_UNBONDED, 29 },
			 [147] = { XC_T2_IOB_UNBONDED, 30 },
			 [148] = { XC_T2_IOB_UNBONDED, 31 },
			 [149] = { XC_T2_IOB_UNBONDED, 32 },
			 [150] = { XC_T2_IOB_UNBONDED, 33 },
			 [151] = { XC_T2_IOB_UNBONDED, 34 },
			 [152] = { XC_T2_IOB_PAD, 121 },
			 [153] = { XC_T2_IOB_PAD, 120 },
			 [154] = { XC_T2_IOB_PAD, 119 },
			 [155] = { XC_T2_IOB_PAD, 118 },
			 [156] = { XC_T2_IOB_PAD, 117 },
			 [157] = { XC_T2_IOB_PAD, 116 },
			 [158] = { XC_T2_IOB_PAD, 115 },
			 [159] = { XC_T2_IOB_PAD, 114 },
			 [160] = { XC_T2_IOB_PAD, 112 },
			 [161] = { XC_T2_IOB_PAD, 111 },
			 [162] = { XC_T2_IOB_PAD, 105 },
			 [163] = { XC_T2_IOB_PAD, 104 },
			 [164] = { XC_T2_IOB_UNBONDED, 47 },
			 [165] = { XC_T2_IOB_UNBONDED, 48 },
			 [166] = { XC_T2_IOB_UNBONDED, 49 },
			 [167] = { XC_T2_IOB_UNBONDED, 50 },
			 [168] = { XC_T2_IOB_UNBONDED, 51 },
			 [169] = { XC_T2_IOB_UNBONDED, 52 },
			 [170] = { XC_T2_IOB_PAD, 102 },
			 [171] = { XC_T2_IOB_PAD, 101 },
			 [172] = { XC_T2_IOB_PAD, 100 },
			 [173] = { XC_T2_IOB_PAD, 99 },
			 [174] = { XC_T2_IOB_PAD, 98 },
			 [175] = { XC_T2_IOB_PAD, 97 },
			 [176] = { XC_T2_IOB_UNBONDED, 59 },
			 [177] = { XC_T2_IOB_UNBONDED, 60 },
			 [178] = { XC_T2_IOB_UNBONDED, 61 },
			 [179] = { XC_T2_IOB_UNBONDED, 62 },
			 [180] = { XC_T2_IOB_UNBONDED, 63 },
			 [181] = { XC_T2_IOB_UNBONDED, 64 },
			 [182] = { XC_T2_IOB_UNBONDED, 65 },
			 [183] = { XC_T2_IOB_UNBONDED, 66 },
			 [184] = { XC_T2_IOB_UNBONDED, 67 },
			 [185] = { XC_T2_IOB_UNBONDED, 68 },
			 [186] = { XC_T2_IOB_PAD, 95 },
			 [187] = { XC_T2_IOB_PAD, 94 },
			 [188] = { XC_T2_IOB_PAD, 93 },
			 [189] = { XC_T2_IOB_PAD, 92 },
			 [190] = { XC_T2_CENTER },
			 [191] = { XC_T2_CENTER },
			 [192] = { XC_T2_CENTER },
			 [193] = { XC_T2_CENTER },
			 [194] = { XC_T2_CENTER },
			 [195] = { XC_T2_CENTER },
			 [196] = { XC_T2_IOB_PAD, 88 },
			 [197] = { XC_T2_IOB_PAD, 87 },
			 [198] = { XC_T2_IOB_PAD, 85 },
			 [199] = { XC_T2_IOB_PAD, 84 },
			 [200] = { XC_T2_IOB_UNBONDED, 77 },
			 [201] = { XC_T2_IOB_UNBONDED, 78 },
			 [202] = { XC_T2_IOB_PAD, 83 },
			 [203] = { XC_T2_IOB_PAD, 82 },
			 [204] = { XC_T2_IOB_PAD, 81 },
			 [205] = { XC_T2_IOB_PAD, 80 },
			 [206] = { XC_T2_IOB_PAD, 79 },
			 [207] = { XC_T2_IOB_PAD, 78 },
			 [208] = { XC_T2_IOB_UNBONDED, 85 },
			 [209] = { XC_T2_IOB_UNBONDED, 86 },
			 [210] = { XC_T2_IOB_UNBONDED, 87 },
			 [211] = { XC_T2_IOB_UNBONDED, 88 },
			 [212] = { XC_T2_IOB_UNBONDED, 89 },
			 [213] = { XC_T2_IOB_UNBONDED, 90 },
			 [214] = { XC_T2_IOB_UNBONDED, 91 },
			 [215] = { XC_T2_IOB_UNBONDED, 92 },
			 [216] = { XC_T2_IOB_UNBONDED, 93 },
			 [217] = { XC_T2_IOB_UNBONDED, 94 },
			 [218] = { XC_T2_IOB_UNBONDED, 95 },
			 [219] = { XC_T2_IOB_UNBONDED, 96 },
			 [220] = { XC_T2_IOB_UNBONDED, 97 },
			 [221] = { XC_T2_IOB_UNBONDED, 98 },
			 [222] = { XC_T2_IOB_PAD, 75 },
			 [223] = { XC_T2_IOB_PAD, 74 }},
		.mcb_ypos = 20,
		.num_mui = 8,
		.mui_pos = { 40, 43, 47, 50, 53, 56, 59, 63 }};
	switch (idcode & IDCODE_MASK) {
		case XC6SLX9: return &xc6slx9_info;
	}
	HERE();
	return 0;
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

void xc6_lut_bitmap(int lut_pos, int (*map)[64], int num_bits)
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
			if (lut_pos == XC6_LMAP_XM_M_A
			    || lut_pos == XC6_LMAP_XM_M_C
			    || lut_pos == XC6_LMAP_XM_X_A
			    || lut_pos == XC6_LMAP_XM_X_B
			    || lut_pos == XC6_LMAP_XL_L_B
			    || lut_pos == XC6_LMAP_XL_L_D
			    || lut_pos == XC6_LMAP_XL_X_A
			    || lut_pos == XC6_LMAP_XL_X_B) {
				(*map)[i] = map32[i]%32;
				(*map)[32+i] = 32+(map32[i]%32);
			} else {
				(*map)[i] = 32+(map32[i]%32);
				(*map)[32+i] = map32[i]%32;
			}
		} else {
			if (num_bits != 64) HERE();
			(*map)[i] = map32[i];
			(*map)[32+i] = ((*map)[i]+32)%64;
		}
	}
}
