//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "control.h"

const struct xc_die *xc_die_info(int idcode)
{
	static const struct xc_die xc6slx9_info = {
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
		.num_t2_ios = 224,
		.t2_io = {
			  [0] = { .pair =  1, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 38, .type_idx = 3 },
			  [1] = { .pair =  1, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 38, .type_idx = 2 },
			  [2] = { .pair =  2, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 38, .type_idx = 0 },
			  [3] = { .pair =  2, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 38, .type_idx = 1 },
			  [4] = { .pair =  3, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 36, .type_idx = 3 },
			  [5] = { .pair =  3, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 36, .type_idx = 2 },
			  [6] = { .pair = 12, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 36, .type_idx = 0 },
			  [7] = { .pair = 12, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 36, .type_idx = 1 },
			  [8] = { .pair = 13, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 31, .type_idx = 3 },
			  [9] = { .pair = 13, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 31, .type_idx = 2 },
			 [10] = { .pair = 14, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 31, .type_idx = 0 },
			 [11] = { .pair = 14, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 31, .type_idx = 1 },
			 [12] = { .pair = 23, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 29, .type_idx = 3 },
			 [13] = { .pair = 23, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 29, .type_idx = 2 },
			 [14] = { .pair = 16, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 29, .type_idx = 0 },
			 [15] = { .pair = 16, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 29, .type_idx = 1 },
			 [16] = { .pair = 29, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 25, .type_idx = 3 },
			 [17] = { .pair = 29, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 25, .type_idx = 2 },
			 [18] = { .pair = 30, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 25, .type_idx = 0 },
			 [19] = { .pair = 30, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 25, .type_idx = 1 },
			// 20-25 are for gclk switches and remain 0
			 [26] = { .pair = 31, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 21, .type_idx = 3 },
			 [27] = { .pair = 31, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 21, .type_idx = 2 },
			 [28] = { .pair = 32, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 21, .type_idx = 0 },
			 [29] = { .pair = 32, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 21, .type_idx = 1 },
			 [30] = { .pair = 45, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 19, .type_idx = 3 },
			 [31] = { .pair = 45, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 19, .type_idx = 2 },
			 [32] = { .pair = 41, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 19, .type_idx = 0 },
			 [33] = { .pair = 41, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 19, .type_idx = 1 },
			 [34] = { .pair = 43, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 14, .type_idx = 3 },
			 [35] = { .pair = 43, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 14, .type_idx = 2 },
			 [36] = { .pair = 46, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 14, .type_idx = 0 },
			 [37] = { .pair = 46, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 14, .type_idx = 1 },
			 [38] = { .pair = 48, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 12, .type_idx = 3 },
			 [39] = { .pair = 48, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 12, .type_idx = 2 },
			 [40] = { .pair = 49, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 12, .type_idx = 0 },
			 [41] = { .pair = 49, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x = 12, .type_idx = 1 },
			 [42] = { .pair = 62, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  7, .type_idx = 3 },
			 [43] = { .pair = 62, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  7, .type_idx = 2 },
			 [44] = { .pair = 63, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  7, .type_idx = 0 },
			 [45] = { .pair = 63, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  7, .type_idx = 1 },
			 [46] = { .pair = 64, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  5, .type_idx = 3 },
			 [47] = { .pair = 64, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  5, .type_idx = 2 },
			 [48] = { .pair = 65, .pos_side = 1, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  5, .type_idx = 0 },
			 [49] = { .pair = 65, .pos_side = 0, .bank = 2, .y = XC6_SLX9_TOTAL_TILE_ROWS - BOT_OUTER_ROW, .x =  5, .type_idx = 1 },

			 [50] = { .pair =  1, .pos_side = 1, .bank = 3, .y = 68, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [51] = { .pair =  1, .pos_side = 0, .bank = 3, .y = 68, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [52] = { .pair =  2, .pos_side = 1, .bank = 3, .y = 67, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [53] = { .pair =  2, .pos_side = 0, .bank = 3, .y = 67, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [54] = { .pair = 31, .pos_side = 1, .bank = 3, .y = 66, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [55] = { .pair = 31, .pos_side = 0, .bank = 3, .y = 66, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [56] = { .pair = 32, .pos_side = 1, .bank = 3, .y = 65, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [57] = { .pair = 32, .pos_side = 0, .bank = 3, .y = 65, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [58] = { .pair = 33, .pos_side = 1, .bank = 3, .y = 61, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [59] = { .pair = 33, .pos_side = 0, .bank = 3, .y = 61, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [60] = { .pair = 34, .pos_side = 1, .bank = 3, .y = 58, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [61] = { .pair = 34, .pos_side = 0, .bank = 3, .y = 58, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [62] = { .pair = 35, .pos_side = 1, .bank = 3, .y = 55, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [63] = { .pair = 35, .pos_side = 0, .bank = 3, .y = 55, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [64] = { .pair = 36, .pos_side = 1, .bank = 3, .y = 52, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [65] = { .pair = 36, .pos_side = 0, .bank = 3, .y = 52, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [66] = { .pair = 37, .pos_side = 1, .bank = 3, .y = 49, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [67] = { .pair = 37, .pos_side = 0, .bank = 3, .y = 49, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [68] = { .pair = 38, .pos_side = 1, .bank = 3, .y = 46, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [69] = { .pair = 38, .pos_side = 0, .bank = 3, .y = 46, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [70] = { .pair = 39, .pos_side = 1, .bank = 3, .y = 42, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [71] = { .pair = 39, .pos_side = 0, .bank = 3, .y = 42, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [72] = { .pair = 40, .pos_side = 1, .bank = 3, .y = 39, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [73] = { .pair = 40, .pos_side = 0, .bank = 3, .y = 39, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [74] = { .pair = 41, .pos_side = 1, .bank = 3, .y = 38, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [75] = { .pair = 41, .pos_side = 0, .bank = 3, .y = 38, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [76] = { .pair = 42, .pos_side = 1, .bank = 3, .y = 37, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [77] = { .pair = 42, .pos_side = 0, .bank = 3, .y = 37, .x = LEFT_OUTER_COL, .type_idx = 0 },
			// 78-83 are for gclk switches and remain 0
			 [84] = { .pair = 43, .pos_side = 1, .bank = 3, .y = 33, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [85] = { .pair = 43, .pos_side = 0, .bank = 3, .y = 33, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [86] = { .pair = 44, .pos_side = 1, .bank = 3, .y = 32, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [87] = { .pair = 44, .pos_side = 0, .bank = 3, .y = 32, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [88] = { .pair = 45, .pos_side = 1, .bank = 3, .y = 31, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [89] = { .pair = 45, .pos_side = 0, .bank = 3, .y = 31, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [90] = { .pair = 46, .pos_side = 1, .bank = 3, .y = 30, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [91] = { .pair = 46, .pos_side = 0, .bank = 3, .y = 30, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [92] = { .pair = 47, .pos_side = 1, .bank = 3, .y = 29, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [93] = { .pair = 47, .pos_side = 0, .bank = 3, .y = 29, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [94] = { .pair = 48, .pos_side = 1, .bank = 3, .y = 28, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [95] = { .pair = 48, .pos_side = 0, .bank = 3, .y = 28, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [96] = { .pair = 49, .pos_side = 1, .bank = 3, .y = 14, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [97] = { .pair = 49, .pos_side = 0, .bank = 3, .y = 14, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [98] = { .pair = 50, .pos_side = 1, .bank = 3, .y = 13, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [99] = { .pair = 50, .pos_side = 0, .bank = 3, .y = 13, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [100] = { .pair = 51, .pos_side = 1, .bank = 3, .y = 12, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [101] = { .pair = 51, .pos_side = 0, .bank = 3, .y = 12, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [102] = { .pair = 52, .pos_side = 1, .bank = 3, .y = 11, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [103] = { .pair = 52, .pos_side = 0, .bank = 3, .y = 11, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [104] = { .pair = 53, .pos_side = 1, .bank = 3, .y =  9, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [105] = { .pair = 53, .pos_side = 0, .bank = 3, .y =  9, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [106] = { .pair = 54, .pos_side = 1, .bank = 3, .y =  7, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [107] = { .pair = 54, .pos_side = 0, .bank = 3, .y =  7, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [108] = { .pair = 55, .pos_side = 1, .bank = 3, .y =  5, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [109] = { .pair = 55, .pos_side = 0, .bank = 3, .y =  5, .x = LEFT_OUTER_COL, .type_idx = 0 },
			 [110] = { .pair = 83, .pos_side = 1, .bank = 3, .y =  3, .x = LEFT_OUTER_COL, .type_idx = 1 },
			 [111] = { .pair = 83, .pos_side = 0, .bank = 3, .y =  3, .x = LEFT_OUTER_COL, .type_idx = 0 },

			 [112] = { .pair =  1, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x =  5, .type_idx = 0 },
			 [113] = { .pair =  1, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x =  5, .type_idx = 1 },
			 [114] = { .pair =  2, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x =  5, .type_idx = 2 },
			 [115] = { .pair =  2, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x =  5, .type_idx = 3 },
			 [116] = { .pair =  3, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x =  7, .type_idx = 0 },
			 [117] = { .pair =  3, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x =  7, .type_idx = 1 },
			 [118] = { .pair =  4, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x =  7, .type_idx = 2 },
			 [119] = { .pair =  4, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x =  7, .type_idx = 3 },
			 [120] = { .pair =  5, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 12, .type_idx = 0 },
			 [121] = { .pair =  5, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 12, .type_idx = 1 },
			 [122] = { .pair =  6, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 12, .type_idx = 2 },
			 [123] = { .pair =  6, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 12, .type_idx = 3 },
			 [124] = { .pair = 10, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 14, .type_idx = 0 },
			 [125] = { .pair = 10, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 14, .type_idx = 1 },
			 [126] = { .pair =  8, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 14, .type_idx = 2 },
			 [127] = { .pair =  8, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 14, .type_idx = 3 },
			 [128] = { .pair = 11, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 19, .type_idx = 0 },
			 [129] = { .pair = 11, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 19, .type_idx = 1 },
			 [130] = { .pair = 33, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 19, .type_idx = 2 },
			 [131] = { .pair = 33, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 19, .type_idx = 3 },
			 [132] = { .pair = 34, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 21, .type_idx = 0 },
			 [133] = { .pair = 34, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 21, .type_idx = 1 },
			 [134] = { .pair = 35, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 21, .type_idx = 2 },
			 [135] = { .pair = 35, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 21, .type_idx = 3 },
			// 136-141 are for gclk switches and remain 0
			 [142] = { .pair = 36, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 25, .type_idx = 0 },
			 [143] = { .pair = 36, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 25, .type_idx = 1 },
			 [144] = { .pair = 37, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 25, .type_idx = 2 },
			 [145] = { .pair = 37, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 25, .type_idx = 3 },
			 [146] = { .pair = 38, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 29, .type_idx = 0 },
			 [147] = { .pair = 38, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 29, .type_idx = 1 },
			 [148] = { .pair = 39, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 29, .type_idx = 2 },
			 [149] = { .pair = 39, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 29, .type_idx = 3 },
			 [150] = { .pair = 41, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 31, .type_idx = 0 },
			 [151] = { .pair = 41, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 31, .type_idx = 1 },
			 [152] = { .pair = 62, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 31, .type_idx = 2 },
			 [153] = { .pair = 62, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 31, .type_idx = 3 },
			 [154] = { .pair = 63, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 36, .type_idx = 0 },
			 [155] = { .pair = 63, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 36, .type_idx = 1 },
			 [156] = { .pair = 64, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 36, .type_idx = 2 },
			 [157] = { .pair = 64, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 36, .type_idx = 3 },
			 [158] = { .pair = 65, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 38, .type_idx = 0 },
			 [159] = { .pair = 65, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 38, .type_idx = 1 },
			 [160] = { .pair = 66, .pos_side = 1, .bank = 0, .y = TOP_OUTER_ROW, .x = 38, .type_idx = 2 },
			 [161] = { .pair = 66, .pos_side = 0, .bank = 0, .y = TOP_OUTER_ROW, .x = 38, .type_idx = 3 },

			 [162] = { .pair =  1, .pos_side = 1, .bank = 1, .y =  4, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [163] = { .pair =  1, .pos_side = 0, .bank = 1, .y =  4, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [164] = { .pair = 29, .pos_side = 1, .bank = 1, .y =  5, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [165] = { .pair = 29, .pos_side = 0, .bank = 1, .y =  5, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [166] = { .pair = 30, .pos_side = 1, .bank = 1, .y =  7, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [167] = { .pair = 30, .pos_side = 0, .bank = 1, .y =  7, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [168] = { .pair = 31, .pos_side = 1, .bank = 1, .y =  9, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [169] = { .pair = 31, .pos_side = 0, .bank = 1, .y =  9, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [170] = { .pair = 32, .pos_side = 1, .bank = 1, .y = 11, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [171] = { .pair = 32, .pos_side = 0, .bank = 1, .y = 11, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [172] = { .pair = 33, .pos_side = 1, .bank = 1, .y = 12, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [173] = { .pair = 33, .pos_side = 0, .bank = 1, .y = 12, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [174] = { .pair = 34, .pos_side = 1, .bank = 1, .y = 13, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [175] = { .pair = 34, .pos_side = 0, .bank = 1, .y = 13, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [176] = { .pair = 35, .pos_side = 1, .bank = 1, .y = 14, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [177] = { .pair = 35, .pos_side = 0, .bank = 1, .y = 14, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [178] = { .pair = 36, .pos_side = 1, .bank = 1, .y = 28, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [179] = { .pair = 36, .pos_side = 0, .bank = 1, .y = 28, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [180] = { .pair = 37, .pos_side = 1, .bank = 1, .y = 29, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [181] = { .pair = 37, .pos_side = 0, .bank = 1, .y = 29, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [182] = { .pair = 38, .pos_side = 1, .bank = 1, .y = 30, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [183] = { .pair = 38, .pos_side = 0, .bank = 1, .y = 30, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [184] = { .pair = 39, .pos_side = 1, .bank = 1, .y = 31, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [185] = { .pair = 39, .pos_side = 0, .bank = 1, .y = 31, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [186] = { .pair = 40, .pos_side = 1, .bank = 1, .y = 32, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [187] = { .pair = 40, .pos_side = 0, .bank = 1, .y = 32, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [188] = { .pair = 41, .pos_side = 1, .bank = 1, .y = 33, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [189] = { .pair = 41, .pos_side = 0, .bank = 1, .y = 33, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			// 190-195 are for gclk switches and remain 0
			 [196] = { .pair = 42, .pos_side = 1, .bank = 1, .y = 37, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [197] = { .pair = 42, .pos_side = 0, .bank = 1, .y = 37, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [198] = { .pair = 43, .pos_side = 1, .bank = 1, .y = 38, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [199] = { .pair = 43, .pos_side = 0, .bank = 1, .y = 38, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [200] = { .pair = 44, .pos_side = 1, .bank = 1, .y = 39, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [201] = { .pair = 44, .pos_side = 0, .bank = 1, .y = 39, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [202] = { .pair = 45, .pos_side = 1, .bank = 1, .y = 42, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [203] = { .pair = 45, .pos_side = 0, .bank = 1, .y = 42, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [204] = { .pair = 46, .pos_side = 1, .bank = 1, .y = 46, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [205] = { .pair = 46, .pos_side = 0, .bank = 1, .y = 46, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [206] = { .pair = 47, .pos_side = 1, .bank = 1, .y = 49, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [207] = { .pair = 47, .pos_side = 0, .bank = 1, .y = 49, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [208] = { .pair = 48, .pos_side = 1, .bank = 1, .y = 52, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [209] = { .pair = 48, .pos_side = 0, .bank = 1, .y = 52, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [210] = { .pair = 49, .pos_side = 1, .bank = 1, .y = 55, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [211] = { .pair = 49, .pos_side = 0, .bank = 1, .y = 55, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [212] = { .pair = 50, .pos_side = 1, .bank = 1, .y = 58, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [213] = { .pair = 50, .pos_side = 0, .bank = 1, .y = 58, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [214] = { .pair = 51, .pos_side = 1, .bank = 1, .y = 61, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [215] = { .pair = 51, .pos_side = 0, .bank = 1, .y = 61, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [216] = { .pair = 52, .pos_side = 1, .bank = 1, .y = 65, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [217] = { .pair = 52, .pos_side = 0, .bank = 1, .y = 65, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [218] = { .pair = 53, .pos_side = 1, .bank = 1, .y = 66, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [219] = { .pair = 53, .pos_side = 0, .bank = 1, .y = 66, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [220] = { .pair = 61, .pos_side = 1, .bank = 1, .y = 67, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [221] = { .pair = 61, .pos_side = 0, .bank = 1, .y = 67, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 },
			 [222] = { .pair = 74, .pos_side = 1, .bank = 1, .y = 68, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 0 },
			 [223] = { .pair = 74, .pos_side = 0, .bank = 1, .y = 68, .x = XC6_SLX9_TOTAL_TILE_COLS - RIGHT_OUTER_O, .type_idx = 1 }},
		// gclk: ug382 table 1-6 page 25
		.num_gclk_pins = XC6_NUM_GCLK_PINS,
		.gclk_t2_io_idx = { // indices into t2_io
			/*  0 */  19,  18,  17,  16,
			/*  4 */ 199, 198, 197, 196,
			/*  8 */ 189, 188, 187, 186,
			/* 12 */ 145, 144, 143, 142,
			/* 16 */ 135, 134, 133, 132,
			/* 20 */  87,  86,  85,  84,
			/* 24 */  77,  76,  75,  74,
			/* 28 */  29,  28,  27,  26 },
		// todo: gclk 2/3/28 and 29 positions not yet verified
		.gclk_t2_switches = { // 16-bit words into type2 data
			/*  0 */  20*4+ 6,  20*4+ 9,  20*4+ 0,  20*4+ 3,
			/*  4 */ 190*4+18, 190*4+21, 190*4+12, 190*4+15,
			/*  8 */ 190*4+ 6, 190*4+ 9, 190*4+ 0, 190*4+ 3,
			/* 12 */ 136*4+18, 136*4+21, 136*4+12, 136*4+15,
			/* 16 */ 136*4+ 6, 136*4+ 9, 136*4+ 0, 136*4+ 3,
			/* 20 */  78*4+21,  78*4+18,  78*4+15,  78*4+12,
			/* 24 */  78*4+ 9,  78*4+ 6,  78*4+ 3,  78*4+ 0,
			/* 28 */  20*4+18,  20*4+21,  20*4+12,  20*4+15 },
		.mcb_ypos = 20,
		.num_mui = 8,
		.mui_pos = { 40, 43, 47, 50, 53, 56, 59, 63 },
		.sel_logicin = {
			24, 15,  7, 42,  5, 12, 62, 16,
			47, 20, 38, 23, 48, 57, 44,  4 }};
	switch (idcode & IDCODE_MASK) {
		case XC6SLX9: return &xc6slx9_info;
	}
	HERE();
	return 0;
}

int xc_die_center_major(const struct xc_die *die)
{
	int i;
	for (i = 0; i < die->num_majors; i++) {
		if (die->majors[i].flags & XC_MAJ_CENTER)
			return i;
	}
	HERE();
	return -1;
}

const struct xc6_pkg_info *xc6_pkg_info(enum xc6_pkg pkg)
{
	// see ug385
	static const struct xc6_pkg_info pkg_tqg144 = {
		.pkg = TQG144,
		.num_pins = /*physical pinouts*/ 144 + /*on die but unbonded*/ 98,
		.pin = {
			// name bank bufio2 description         pair pos_side
			{ "P144",  0, "TL", "IO_L1P_HSWAPEN_0",  1, 1 },
			{ "P143",  0, "TL", "IO_L1N_VREF_0",  1, 0 },
			{ "P142",  0, "TL", "IO_L2P_0", 2, 1 },
			{ "P141",  0, "TL", "IO_L2N_0", 2, 0 },
			{ "P140",  0, "TL", "IO_L3P_0", 3, 1 },
			{ "P139",  0, "TL", "IO_L3N_0", 3, 0 },
			{ "P138",  0, "TL", "IO_L4P_0", 4, 1 },
			{ "P137",  0, "TL", "IO_L4N_0", 4, 0 },
			{ "P134",  0, "TL", "IO_L34P_GCLK19_0", 34, 1 },
			{ "P133",  0, "TL", "IO_L34N_GCLK18_0", 34, 0 },
			{ "P132",  0, "TL", "IO_L35P_GCLK17_0", 35, 1 },
			{ "P131",  0, "TL", "IO_L35N_GCLK16_0", 35, 0 },
			{ "P127",  0, "TR", "IO_L36P_GCLK15_0", 36, 1 },
			{ "P126",  0, "TR", "IO_L36N_GCLK14_0", 36, 0 },
			{ "P124",  0, "TR", "IO_L37P_GCLK13_0", 37, 1 },
			{ "P123",  0, "TR", "IO_L37N_GCLK12_0", 37, 0 },
			{ "P121",  0, "TR", "IO_L62P_0", 62, 1 },
			{ "P120",  0, "TR", "IO_L62N_VREF_0", 62, 0 },
			{ "P119",  0, "TR", "IO_L63P_SCP7_0", 63, 1 },
			{ "P118",  0, "TR", "IO_L63N_SCP6_0", 63, 0 },
			{ "P117",  0, "TR", "IO_L64P_SCP5_0", 64, 1 },
			{ "P116",  0, "TR", "IO_L64N_SCP4_0", 64, 0 },
			{ "P115",  0, "TR", "IO_L65P_SCP3_0", 65, 1 },
			{ "P114",  0, "TR", "IO_L65N_SCP2_0", 65, 0 },
			{ "P112",  0, "TR", "IO_L66P_SCP1_0", 66, 1 },
			{ "P111",  0, "TR", "IO_L66N_SCP0_0", 66, 0 },
			{ "P109", -1, "NA", "TCK", 0, 0 },
			{ "P110", -1, "NA", "TDI", 0, 0 },
			{ "P107", -1, "NA", "TMS", 0, 0 },
			{ "P106", -1, "NA", "TDO", 0, 0 },
			{ "P105",  1, "RT", "IO_L1P_1", 1, 1 },
			{ "P104",  1, "RT", "IO_L1N_VREF_1", 1, 0 },
			{ "P102",  1, "RT", "IO_L32P_1", 32, 1 },
			{ "P101",  1, "RT", "IO_L32N_1", 32, 0 },
			{ "P100",  1, "RT", "IO_L33P_1", 33, 1 },
			{ "P99",   1, "RT", "IO_L33N_1", 33, 0 },
			{ "P98",   1, "RT", "IO_L34P_1", 34, 1 },
			{ "P97",   1, "RT", "IO_L34N_1", 34, 0 },
			{ "P95",   1, "RT", "IO_L40P_GCLK11_1", 40, 1 },
			{ "P94",   1, "RT", "IO_L40N_GCLK10_1", 40, 0 },
			{ "P93",   1, "RT", "IO_L41P_GCLK9_IRDY1_1", 41, 1 },
			{ "P92",   1, "RT", "IO_L41N_GCLK8_1", 41, 0 },
			{ "P88",   1, "RB", "IO_L42P_GCLK7_1", 42, 1 },
			{ "P87",   1, "RB", "IO_L42N_GCLK6_TRDY1_1", 42, 0 },
			{ "P85",   1, "RB", "IO_L43P_GCLK5_1", 43, 1 },
			{ "P84",   1, "RB", "IO_L43N_GCLK4_1", 43, 0 },
			{ "P83",   1, "RB", "IO_L45P_1", 45, 1 },
			{ "P82",   1, "RB", "IO_L45N_1", 45, 0 },
			{ "P81",   1, "RB", "IO_L46P_1", 46, 1 },
			{ "P80",   1, "RB", "IO_L46N_1", 46, 0 },
			{ "P79",   1, "RB", "IO_L47P_1", 47, 1 },
			{ "P78",   1, "RB", "IO_L47N_1", 47, 0 },
			{ "P75",   1, "RB", "IO_L74P_AWAKE_1", 74, 1 },
			{ "P74",   1, "RB", "IO_L74N_DOUT_BUSY_1", 74, 0 },
			{ "P73",  -1, "NA", "SUSPEND", 0, 0 },
			{ "P72",   2, "NA", "CMPCS_B_2", 0, 0 },
			{ "P71",   2, "NA", "DONE_2", 0, 0 },
			{ "P70",   2, "BR", "IO_L1P_CCLK_2", 1, 1 },
			{ "P69",   2, "BR", "IO_L1N_M0_CMPMISO_2", 1, 0 },
			{ "P67",   2, "BR", "IO_L2P_CMPCLK_2", 2, 1 },
			{ "P66",   2, "BR", "IO_L2N_CMPMOSI_2", 2, 0 },
			{ "P65",   2, "BR", "IO_L3P_D0_DIN_MISO_MISO1_2", 3, 1 },
			{ "P64",   2, "BR", "IO_L3N_MOSI_CSI_B_MISO0_2", 3, 0 },
			{ "P62",   2, "BR", "IO_L12P_D1_MISO2_2", 12, 1 },
			{ "P61",   2, "BR", "IO_L12N_D2_MISO3_2", 12, 0 },
			{ "P60",   2, "BR", "IO_L13P_M1_2", 13, 1 },
			{ "P59",   2, "BR", "IO_L13N_D10_2", 13, 0 },
			{ "P58",   2, "BR", "IO_L14P_D11_2", 14, 1 },
			{ "P57",   2, "BR", "IO_L14N_D12_2", 14, 0 },
			{ "P56",   2, "BR", "IO_L30P_GCLK1_D13_2", 30, 1 },
			{ "P55",   2, "BR", "IO_L30N_GCLK0_USERCCLK_2", 30, 0 },
			{ "P51",   2, "BL", "IO_L31P_GCLK31_D14_2", 31, 1 },
			{ "P50",   2, "BL", "IO_L31N_GCLK30_D15_2", 31, 0 },
			{ "P48",   2, "BL", "IO_L48P_D7_2", 48, 1 },
			{ "P47",   2, "BL", "IO_L48N_RDWR_B_VREF_2", 48, 0 },
			{ "P46",   2, "BL", "IO_L49P_D3_2", 49, 1 },
			{ "P45",   2, "BL", "IO_L49N_D4_2", 49, 0 },
			{ "P44",   2, "BL", "IO_L62P_D5_2", 62, 1 },
			{ "P43",   2, "BL", "IO_L62N_D6_2", 62, 0 },
			{ "P41",   2, "BL", "IO_L64P_D8_2", 64, 1 },
			{ "P40",   2, "BL", "IO_L64N_D9_2", 64, 0 },
			{ "P39",   2, "BL", "IO_L65P_INIT_B_2", 65, 1 },
			{ "P38",   2, "BL", "IO_L65N_CSO_B_2", 65, 0 },
			{ "P37",   2, "NA", "PROGRAM_B_2", 0, 0 },
			{ "P35",   3, "LB", "IO_L1P_3", 1, 1 },
			{ "P34",   3, "LB", "IO_L1N_VREF_3", 1, 0 },
			{ "P33",   3, "LB", "IO_L2P_3", 2, 1 },
			{ "P32",   3, "LB", "IO_L2N_3", 2, 0 },
			{ "P30",   3, "LB", "IO_L36P_3", 36, 1 },
			{ "P29",   3, "LB", "IO_L36N_3", 36, 0 },
			{ "P27",   3, "LB", "IO_L37P_3", 37, 1 },
			{ "P26",   3, "LB", "IO_L37N_3", 37, 0 },
			{ "P24",   3, "LB", "IO_L41P_GCLK27_3", 41, 1 },
			{ "P23",   3, "LB", "IO_L41N_GCLK26_3", 41, 0 },
			{ "P22",   3, "LB", "IO_L42P_GCLK25_TRDY2_3", 42, 1 },
			{ "P21",   3, "LB", "IO_L42N_GCLK24_3", 42, 0 },
			{ "P17",   3, "LT", "IO_L43P_GCLK23_3", 43, 1 },
			{ "P16",   3, "LT", "IO_L43N_GCLK22_IRDY2_3", 43, 0 },
			{ "P15",   3, "LT", "IO_L44P_GCLK21_3", 44, 1 },
			{ "P14",   3, "LT", "IO_L44N_GCLK20_3", 44, 0 },
			{ "P12",   3, "LT", "IO_L49P_3", 49, 1 },
			{ "P11",   3, "LT", "IO_L49N_3", 49, 0 },
			{ "P10",   3, "LT", "IO_L50P_3", 50, 1 },
			{ "P9",	   3, "LT", "IO_L50N_3", 50, 0 },
			{ "P8",	   3, "LT", "IO_L51P_3", 51, 1 },
			{ "P7",	   3, "LT", "IO_L51N_3", 51, 0 },
			{ "P6",    3, "LT", "IO_L52P_3", 52, 1 },
			{ "P5",	   3, "LT", "IO_L52N_3", 52, 0 },
			{ "P2",	   3, "LT", "IO_L83P_3", 83, 1 },
			{ "P1",	   3, "LT", "IO_L83N_VREF_3", 83, 0 },
			{ "P108", -1, "NA", "GND", 0, 0 },
			{ "P113", -1, "NA", "GND", 0, 0 },
			{ "P13",  -1, "NA", "GND", 0, 0 },
			{ "P130", -1, "NA", "GND", 0, 0 },
			{ "P136", -1, "NA", "GND", 0, 0 },
			{ "P25",  -1, "NA", "GND", 0, 0 },
			{ "P3",	  -1, "NA", "GND", 0, 0 },
			{ "P49",  -1, "NA", "GND", 0, 0 },
			{ "P54",  -1, "NA", "GND", 0, 0 },
			{ "P68",  -1, "NA", "GND", 0, 0 },
			{ "P77",  -1, "NA", "GND", 0, 0 },
			{ "P91",  -1, "NA", "GND", 0, 0 },
			{ "P96",  -1, "NA", "GND", 0, 0 },
			{ "P129", -1, "NA", "VCCAUX", 0, 0 },
			{ "P20",  -1, "NA", "VCCAUX", 0, 0 },
			{ "P36",  -1, "NA", "VCCAUX", 0, 0 },
			{ "P53",  -1, "NA", "VCCAUX", 0, 0 },
			{ "P90",  -1, "NA", "VCCAUX", 0, 0 },
			{ "P128", -1, "NA", "VCCINT", 0, 0 },
			{ "P19",  -1, "NA", "VCCINT", 0, 0 },
			{ "P28",  -1, "NA", "VCCINT", 0, 0 },
			{ "P52",  -1, "NA", "VCCINT", 0, 0 },
			{ "P89",  -1, "NA", "VCCINT", 0, 0 },
			{ "P122",  0, "NA", "VCCO_0", 0, 0 },
			{ "P125",  0, "NA", "VCCO_0", 0, 0 },
			{ "P135",  0, "NA", "VCCO_0", 0, 0 },
			{ "P103",  1, "NA", "VCCO_1", 0, 0 },
			{ "P76",   1, "NA", "VCCO_1", 0, 0 },
			{ "P86",   1, "NA", "VCCO_1", 0, 0 },
			{ "P42",   2, "NA", "VCCO_2", 0, 0 },
			{ "P63",   2, "NA", "VCCO_2", 0, 0 },
			{ "P18",   3, "NA", "VCCO_3", 0, 0 },
			{ "P31",   3, "NA", "VCCO_3", 0, 0 },
			{ "P4",    3, "NA", "VCCO_3", 0, 0 },

			// rest is unbonded (.description = 0)
			{ "UNB113", 2, "BR", 0, 23, 1 },
			{ "UNB114", 2, "BR", 0, 23, 0 },
			{ "UNB115", 2, "BR", 0, 16, 1 },
			{ "UNB116", 2, "BR", 0, 16, 0 },
			{ "UNB117", 2, "BR", 0, 29, 1 },
			{ "UNB118", 2, "BR", 0, 29, 0 },
			{ "UNB123", 2, "BL", 0, 32, 1 },
			{ "UNB124", 2, "BL", 0, 32, 0 },
			{ "UNB125", 2, "BL", 0, 45, 1 },
			{ "UNB126", 2, "BL", 0, 45, 0 },
			{ "UNB127", 2, "BL", 0, 41, 1 },
			{ "UNB128", 2, "BL", 0, 41, 0 },
			{ "UNB129", 2, "BL", 0, 43, 1 },
			{ "UNB130", 2, "BL", 0, 43, 0 },
			{ "UNB131", 2, "BL", 0, 46, 1 },
			{ "UNB132", 2, "BL", 0, 46, 0 },
			{ "UNB139", 2, "BL", 0, 63, 1 },
			{ "UNB140", 2, "BL", 0, 63, 0 },
			{ "UNB149", 3, "LB", 0, 31, 1 },
			{ "UNB150", 3, "LB", 0, 31, 0 },
			{ "UNB151", 3, "LB", 0, 32, 1 },
			{ "UNB152", 3, "LB", 0, 32, 0 },
			{ "UNB153", 3, "LB", 0, 33, 1 },
			{ "UNB154", 3, "LB", 0, 33, 0 },
			{ "UNB155", 3, "LB", 0, 34, 1 },
			{ "UNB156", 3, "LB", 0, 34, 0 },
			{ "UNB157", 3, "LB", 0, 35, 1 },
			{ "UNB158", 3, "LB", 0, 35, 0 },
			{ "UNB163", 3, "LB", 0, 38, 1 },
			{ "UNB164", 3, "LB", 0, 38, 0 },
			{ "UNB165", 3, "LB", 0, 39, 1 },
			{ "UNB166", 3, "LB", 0, 39, 0 },
			{ "UNB167", 3, "LB", 0, 40, 1 },
			{ "UNB168", 3, "LB", 0, 40, 0 },
			{ "UNB177", 3, "LT", 0, 45, 1 },
			{ "UNB178", 3, "LT", 0, 45, 0 },
			{ "UNB179", 3, "LT", 0, 46, 1 },
			{ "UNB180", 3, "LT", 0, 46, 0 },
			{ "UNB181", 3, "LT", 0, 47, 1 },
			{ "UNB182", 3, "LT", 0, 47, 0 },
			{ "UNB183", 3, "LT", 0, 48, 1 },
			{ "UNB184", 3, "LT", 0, 48, 0 },
			{ "UNB193", 3, "LT", 0, 53, 1 },
			{ "UNB194", 3, "LT", 0, 53, 0 },
			{ "UNB195", 3, "LT", 0, 54, 1 },
			{ "UNB196", 3, "LT", 0, 54, 0 },
			{ "UNB197", 3, "LT", 0, 55, 1 },
			{ "UNB198", 3, "LT", 0, 55, 0 },
			{ "UNB9",  0, "TL", 0,  5, 1 },
			{ "UNB10", 0, "TL", 0,  5, 0 },
			{ "UNB11", 0, "TL", 0,  6, 1 },
			{ "UNB12", 0, "TL", 0,  6, 0 },
			{ "UNB13", 0, "TL", 0, 10, 1 },
			{ "UNB14", 0, "TL", 0, 10, 0 },
			{ "UNB15", 0, "TL", 0,  8, 1 },
			{ "UNB16", 0, "TL", 0,  8, 0 },
			{ "UNB17", 0, "TL", 0, 11, 1 },
			{ "UNB18", 0, "TL", 0, 11, 0 },
			{ "UNB19", 0, "TL", 0, 33, 1 },
			{ "UNB20", 0, "TL", 0, 33, 0 },
			{ "UNB29", 0, "TR", 0, 38, 1 },
			{ "UNB30", 0, "TR", 0, 38, 0 },
			{ "UNB31", 0, "TR", 0, 39, 1 },
			{ "UNB32", 0, "TR", 0, 39, 0 },
			{ "UNB33", 0, "TR", 0, 41, 1 },
			{ "UNB34", 0, "TR", 0, 41, 0 },
			{ "UNB47", 1, "RT", 0, 29, 1 },
			{ "UNB48", 1, "RT", 0, 29, 0 },
			{ "UNB49", 1, "RT", 0, 30, 1 },
			{ "UNB50", 1, "RT", 0, 30, 0 },
			{ "UNB51", 1, "RT", 0, 31, 1 },
			{ "UNB52", 1, "RT", 0, 31, 0 },
			{ "UNB59", 1, "RT", 0, 35, 1 },
			{ "UNB60", 1, "RT", 0, 35, 0 },
			{ "UNB61", 1, "RT", 0, 36, 1 },
			{ "UNB62", 1, "RT", 0, 36, 0 },
			{ "UNB63", 1, "RT", 0, 37, 1 },
			{ "UNB64", 1, "RT", 0, 37, 0 },
			{ "UNB65", 1, "RT", 0, 38, 1 },
			{ "UNB66", 1, "RT", 0, 38, 0 },
			{ "UNB67", 1, "RT", 0, 39, 1 },
			{ "UNB68", 1, "RT", 0, 39, 0 },
			{ "UNB77", 1, "RB", 0, 44, 1 },
			{ "UNB78", 1, "RB", 0, 44, 0 },
			{ "UNB85", 1, "RB", 0, 48, 1 },
			{ "UNB86", 1, "RB", 0, 48, 0 },
			{ "UNB87", 1, "RB", 0, 49, 1 },
			{ "UNB88", 1, "RB", 0, 49, 0 },
			{ "UNB89", 1, "RB", 0, 50, 1 },
			{ "UNB90", 1, "RB", 0, 50, 0 },
			{ "UNB91", 1, "RB", 0, 51, 1 },
			{ "UNB92", 1, "RB", 0, 51, 0 },
			{ "UNB93", 1, "RB", 0, 52, 1 },
			{ "UNB94", 1, "RB", 0, 52, 0 },
			{ "UNB95", 1, "RB", 0, 53, 1 },
			{ "UNB96", 1, "RB", 0, 53, 0 },
			{ "UNB97", 1, "RB", 0, 61, 1 },
			{ "UNB98", 1, "RB", 0, 61, 0 }}};
	static const struct xc6_pkg_info pkg_ftg256 = {
		.pkg = FTG256,
		// todo: any unbonded pins?
		.num_pins = /*physical pinouts*/ 256 + /*on die but unbonded*/ 0,
			// name bank bufio2 description         pair pos_side
		.pin = {
			{ "C4",   0, "TL", "IO_L1P_HSWAPEN_0", 1, 1 },
			{ "A4",	  0, "TL", "IO_L1N_VREF_0", 1, 0 },
			{ "B5",	  0, "TL", "IO_L2P_0", 2, 1 },
			{ "A5",	  0, "TL", "IO_L2N_0", 2, 0 },
			{ "D5",	  0, "TL", "IO_L3P_0", 3, 1 },
			{ "C5",	  0, "TL", "IO_L3N_0", 3, 0 },
			{ "B6",	  0, "TL", "IO_L4P_0", 4, 1 },
			{ "A6",	  0, "TL", "IO_L4N_0", 4, 0 },
			{ "F7",	  0, "TL", "IO_L5P_0", 5, 1 },
			{ "E6",   0, "TL", "IO_L5N_0", 5, 0 },
			{ "C7",   0, "TL", "IO_L6P_0", 6, 1 },
			{ "A7",   0, "TL", "IO_L6N_0", 6, 0 },
			{ "D6",   0, "TL", "IO_L7P_0", 7, 1 },
			{ "C6",   0, "TL", "IO_L7N_0", 7, 0 },
			{ "B8",   0, "TL", "IO_L33P_0", 33, 1 },
			{ "A8",   0, "TL", "IO_L33N_0", 33, 0 },
			{ "C9",   0, "TL", "IO_L34P_GCLK19_0", 34, 1 },
			{ "A9",   0, "TL", "IO_L34N_GCLK18_0", 34, 0 },
			{ "B10",  0, "TL", "IO_L35P_GCLK17_0", 35, 1 },
			{ "A10",  0, "TL", "IO_L35N_GCLK16_0", 35, 0 },
			{ "E7",   0, "TR", "IO_L36P_GCLK15_0", 36, 1 },
			{ "E8",   0, "TR", "IO_L36N_GCLK14_0", 36, 0 },
			{ "E10",  0, "TR", "IO_L37P_GCLK13_0", 37, 1 },
			{ "C10",  0, "TR", "IO_L37N_GCLK12_0", 37, 0 },
			{ "D8",   0, "TR", "IO_L38P_0", 38, 1 },
			{ "C8",   0, "TR", "IO_L38N_VREF_0", 38, 0 },
			{ "C11",  0, "TR", "IO_L39P_0", 39, 1 },
			{ "A11",  0, "TR", "IO_L39N_0", 39, 0 },
			{ "F9",	  0, "TR", "IO_L40P_0", 40, 1 },
			{ "D9",	  0, "TR", "IO_L40N_0", 40, 0 },
			{ "B12",  0, "TR", "IO_L62P_0", 62, 1 },
			{ "A12",  0, "TR", "IO_L62N_VREF_0", 62, 0 },
			{ "C13",  0, "TR", "IO_L63P_SCP7_0", 63, 1 },
			{ "A13",  0, "TR", "IO_L63N_SCP6_0", 63, 0 },
			{ "F10",  0, "TR", "IO_L64P_SCP5_0", 64, 1 },
			{ "E11",  0, "TR", "IO_L64N_SCP4_0", 64, 0 },
			{ "B14",  0, "TR", "IO_L65P_SCP3_0", 65, 1 },
			{ "A14",  0, "TR", "IO_L65N_SCP2_0", 65, 0 },
			{ "D11",  0, "TR", "IO_L66P_SCP1_0", 66, 1 },
			{ "D12",  0, "TR", "IO_L66N_SCP0_0", 66, 0 },
			{ "C14", -1, "NA", "TCK" },
			{ "C12", -1, "NA", "TDI" },
			{ "A15", -1, "NA", "TMS" },
			{ "E14", -1, "NA", "TDO" },
			{ "E13",  1, "RT", "IO_L1P_A25_1", 1, 1 },
			{ "E12",  1, "RT", "IO_L1N_A24_VREF_1", 1, 0 },
			{ "B15",  1, "RT", "IO_L29P_A23_M1A13_1", 29, 1 },
			{ "B16",  1, "RT", "IO_L29N_A22_M1A14_1", 29, 0 },
			{ "F12",  1, "RT", "IO_L30P_A21_M1RESET_1", 30, 1 },
			{ "G11",  1, "RT", "IO_L30N_A20_M1A11_1", 30, 0 },
			{ "D14",  1, "RT", "IO_L31P_A19_M1CKE_1", 31, 1 },
			{ "D16",  1, "RT", "IO_L31N_A18_M1A12_1", 31, 0 },
			{ "F13",  1, "RT", "IO_L32P_A17_M1A8_1", 32, 1 },
			{ "F14",  1, "RT", "IO_L32N_A16_M1A9_1", 32, 0 },
			{ "C15",  1, "RT", "IO_L33P_A15_M1A10_1", 33, 1 },
			{ "C16",  1, "RT", "IO_L33N_A14_M1A4_1", 33, 0 },
			{ "E15",  1, "RT", "IO_L34P_A13_M1WE_1", 34, 1 },
			{ "E16",  1, "RT", "IO_L34N_A12_M1BA2_1", 34, 0 },
			{ "F15",  1, "RT", "IO_L35P_A11_M1A7_1", 35, 1 },
			{ "F16",  1, "RT", "IO_L35N_A10_M1A2_1", 35, 0 },
			{ "G14",  1, "RT", "IO_L36P_A9_M1BA0_1", 36, 1 },
			{ "G16",  1, "RT", "IO_L36N_A8_M1BA1_1", 36, 0 },
			{ "H15",  1, "RT", "IO_L37P_A7_M1A0_1", 37, 1 },
			{ "H16",  1, "RT", "IO_L37N_A6_M1A1_1", 37, 0 },
			{ "G12",  1, "RT", "IO_L38P_A5_M1CLK_1", 38, 1 },
			{ "H11",  1, "RT", "IO_L38N_A4_M1CLKN_1", 38, 0 },
			{ "H13",  1, "RT", "IO_L39P_M1A3_1", 39, 1 },
			{ "H14",  1, "RT", "IO_L39N_M1ODT_1", 39, 0 },
			{ "J11",  1, "RT", "IO_L40P_GCLK11_M1A5_1", 40, 1 },
			{ "J12",  1, "RT", "IO_L40N_GCLK10_M1A6_1", 40, 0 },
			{ "J13",  1, "RT", "IO_L41P_GCLK9_IRDY1_M1RASN_1", 41, 1 },
			{ "K14",  1, "RT", "IO_L41N_GCLK8_M1CASN_1", 41, 0 },
			{ "K12",  1, "RB", "IO_L42P_GCLK7_M1UDM_1", 42, 1 },
			{ "K11",  1, "RB", "IO_L42N_GCLK6_TRDY1_M1LDM_1", 42, 0 },
			{ "J14",  1, "RB", "IO_L43P_GCLK5_M1DQ4_1", 43, 1 },
			{ "J16",  1, "RB", "IO_L43N_GCLK4_M1DQ5_1", 43, 0 },
			{ "K15",  1, "RB", "IO_L44P_A3_M1DQ6_1", 44, 1 },
			{ "K16",  1, "RB", "IO_L44N_A2_M1DQ7_1", 44, 0 },
			{ "N14",  1, "RB", "IO_L45P_A1_M1LDQS_1", 45, 1 },
			{ "N16",  1, "RB", "IO_L45N_A0_M1LDQSN_1", 45, 0 },
			{ "M15",  1, "RB", "IO_L46P_FCS_B_M1DQ2_1", 46, 1 },
			{ "M16",  1, "RB", "IO_L46N_FOE_B_M1DQ3_1", 46, 0 },
			{ "L14",  1, "RB", "IO_L47P_FWE_B_M1DQ0_1", 47, 1 },
			{ "L16",  1, "RB", "IO_L47N_LDC_M1DQ1_1", 47, 0 },
			{ "P15",  1, "RB", "IO_L48P_HDC_M1DQ8_1", 48, 1 },
			{ "P16",  1, "RB", "IO_L48N_M1DQ9_1", 48, 0 },
			{ "R15",  1, "RB", "IO_L49P_M1DQ10_1", 49, 1 },
			{ "R16",  1, "RB", "IO_L49N_M1DQ11_1", 49, 0 },
			{ "R14",  1, "RB", "IO_L50P_M1UDQS_1", 50, 1 },
			{ "T15",  1, "RB", "IO_L50N_M1UDQSN_1", 50, 0 },
			{ "T14",  1, "RB", "IO_L51P_M1DQ12_1", 51, 1 },
			{ "T13",  1, "RB", "IO_L51N_M1DQ13_1", 51, 0 },
			{ "R12",  1, "RB", "IO_L52P_M1DQ14_1", 52, 1 },
			{ "T12",  1, "RB", "IO_L52N_M1DQ15_1", 52, 0 },
			{ "L12",  1, "RB", "IO_L53P_1", 53, 1 },
			{ "L13",  1, "RB", "IO_L53N_VREF_1", 53, 0 },
			{ "M13",  1, "RB", "IO_L74P_AWAKE_1", 74, 1 },
			{ "M14",  1, "RB", "IO_L74N_DOUT_BUSY_1", 74, 0 },
			{ "P14", -1, "NA", "SUSPEND" },
			{ "L11",  2, "NA", "CMPCS_B_2" },
			{ "P13",  2, "NA", "DONE_2" },
			{ "R11",  2, "BR", "IO_L1P_CCLK_2", 1, 1 },
			{ "T11",  2, "BR", "IO_L1N_M0_CMPMISO_2", 1, 0 },
			{ "M12",  2, "BR", "IO_L2P_CMPCLK_2", 2, 1 },
			{ "M11",  2, "BR", "IO_L2N_CMPMOSI_2", 2, 0 },
			{ "P10",  2, "BR", "IO_L3P_D0_DIN_MISO_MISO1_2", 3, 1 },
			{ "T10",  2, "BR", "IO_L3N_MOSI_CSI_B_MISO0_2", 3, 0 },
			{ "N12",  2, "BR", "IO_L12P_D1_MISO2_2", 12, 1 },
			{ "P12",  2, "BR", "IO_L12N_D2_MISO3_2", 12, 0 },
			{ "N11",  2, "BR", "IO_L13P_M1_2", 13, 1 },
			{ "P11",  2, "BR", "IO_L13N_D10_2", 13, 0 },
			{ "N9",	  2, "BR", "IO_L14P_D11_2", 14, 1 },
			{ "P9",	  2, "BR", "IO_L14N_D12_2", 14, 0 },
			{ "R9",   2, "BR", "IO_L23P_2", 23, 1 },
			{ "T9",	  2, "BR", "IO_L23N_2", 23, 0 },
			{ "L10",  2, "BR", "IO_L16P_2", 16, 1 },
			{ "M10",  2, "BR", "IO_L16N_VREF_2", 16, 0 },
			{ "M9",   2, "BR", "IO_L29P_GCLK3_2", 29, 1 },
			{ "N8",   2, "BR", "IO_L29N_GCLK2_2", 29, 0 },
			{ "P8",   2, "BR", "IO_L30P_GCLK1_D13_2", 30, 1 },
			{ "T8",   2, "BR", "IO_L30N_GCLK0_USERCCLK_2", 30, 0 },
			{ "P7",   2, "BL", "IO_L31P_GCLK31_D14_2", 31, 1 },
			{ "M7",   2, "BL", "IO_L31N_GCLK30_D15_2", 31, 0 },
			{ "R7",   2, "BL", "IO_L32P_GCLK29_2", 32, 1 },
			{ "T7",   2, "BL", "IO_L32N_GCLK28_2", 32, 0 },
			{ "P6",   2, "BL", "IO_L47P_2", 47, 1 },
			{ "T6",   2, "BL", "IO_L47N_2", 47, 0 },
			{ "R5",   2, "BL", "IO_L48P_D7_2", 48, 1 },
			{ "T5",   2, "BL", "IO_L48N_RDWR_B_VREF_2", 48, 0 },
			{ "N5",   2, "BL", "IO_L49P_D3_2", 49, 1 },
			{ "P5",   2, "BL", "IO_L49N_D4_2", 49, 0 },
			{ "L8",   2, "BL", "IO_L62P_D5_2", 62, 1 },
			{ "L7",   2, "BL", "IO_L62N_D6_2", 62, 0 },
			{ "P4",   2, "BL", "IO_L63P_2", 63, 1 },
			{ "T4",   2, "BL", "IO_L63N_2", 63, 0 },
			{ "M6",   2, "BL", "IO_L64P_D8_2", 64, 1 },
			{ "N6",   2, "BL", "IO_L64N_D9_2", 64, 0 },
			{ "R3",   2, "BL", "IO_L65P_INIT_B_2", 65, 1 },
			{ "T3",   2, "BL", "IO_L65N_CSO_B_2", 65, 0 },
			{ "T2",   2, "NA", "PROGRAM_B_2" },
			{ "M4",   3, "LB", "IO_L1P_3", 1, 1 },
			{ "M3",   3, "LB", "IO_L1N_VREF_3", 1, 0 },
			{ "M5",   3, "LB", "IO_L2P_3", 2, 1 },
			{ "N4",   3, "LB", "IO_L2N_3", 2, 0 },
			{ "R2",   3, "LB", "IO_L32P_M3DQ14_3", 32, 1 },
			{ "R1",   3, "LB", "IO_L32N_M3DQ15_3", 32, 0 },
			{ "P2",   3, "LB", "IO_L33P_M3DQ12_3", 33, 1 },
			{ "P1",   3, "LB", "IO_L33N_M3DQ13_3", 33, 0 },
			{ "N3",   3, "LB", "IO_L34P_M3UDQS_3", 34, 1 },
			{ "N1",   3, "LB", "IO_L34N_M3UDQSN_3", 34, 0 },
			{ "M2",   3, "LB", "IO_L35P_M3DQ10_3", 35, 1 },
			{ "M1",   3, "LB", "IO_L35N_M3DQ11_3", 35, 0 },
			{ "L3",   3, "LB", "IO_L36P_M3DQ8_3", 36, 1 },
			{ "L1",   3, "LB", "IO_L36N_M3DQ9_3", 36, 0 },
			{ "K2",   3, "LB", "IO_L37P_M3DQ0_3", 37, 1 },
			{ "K1",   3, "LB", "IO_L37N_M3DQ1_3", 37, 0 },
			{ "J3",   3, "LB", "IO_L38P_M3DQ2_3", 38, 1 },
			{ "J1",   3, "LB", "IO_L38N_M3DQ3_3", 38, 0 },
			{ "H2",   3, "LB", "IO_L39P_M3LDQS_3", 39, 1 },
			{ "H1",   3, "LB", "IO_L39N_M3LDQSN_3", 39, 0 },
			{ "G3",   3, "LB", "IO_L40P_M3DQ6_3", 40, 1 },
			{ "G1",   3, "LB", "IO_L40N_M3DQ7_3", 40, 0 },
			{ "F2",   3, "LB", "IO_L41P_GCLK27_M3DQ4_3", 41, 1 },
			{ "F1",   3, "LB", "IO_L41N_GCLK26_M3DQ5_3", 41, 0 },
			{ "K3",   3, "LB", "IO_L42P_GCLK25_TRDY2_M3UDM_3", 42, 1 },
			{ "J4",   3, "LB", "IO_L42N_GCLK24_M3LDM_3", 42, 0 },
			{ "J6",   3, "LT", "IO_L43P_GCLK23_M3RASN_3", 43, 1 },
			{ "H5",   3, "LT", "IO_L43N_GCLK22_IRDY2_M3CASN_3", 43, 0 },
			{ "H4",   3, "LT", "IO_L44P_GCLK21_M3A5_3", 44, 1 },
			{ "H3",   3, "LT", "IO_L44N_GCLK20_M3A6_3", 44, 0 },
			{ "L4",   3, "LT", "IO_L45P_M3A3_3", 45, 1 },
			{ "L5",   3, "LT", "IO_L45N_M3ODT_3", 45, 0 },
			{ "E2",   3, "LT", "IO_L46P_M3CLK_3", 46, 1 },
			{ "E1",   3, "LT", "IO_L46N_M3CLKN_3", 46, 0 },
			{ "K5",   3, "LT", "IO_L47P_M3A0_3", 47, 1 },
			{ "K6",   3, "LT", "IO_L47N_M3A1_3", 47, 0 },
			{ "C3",   3, "LT", "IO_L48P_M3BA0_3", 48, 1 },
			{ "C2",   3, "LT", "IO_L48N_M3BA1_3", 48, 0 },
			{ "D3",   3, "LT", "IO_L49P_M3A7_3", 49, 1 },
			{ "D1",   3, "LT", "IO_L49N_M3A2_3", 49, 0 },
			{ "C1",   3, "LT", "IO_L50P_M3WE_3", 50, 1 },
			{ "B1",   3, "LT", "IO_L50N_M3BA2_3", 50, 0 },
			{ "G6",   3, "LT", "IO_L51P_M3A10_3", 51, 1 },
			{ "G5",   3, "LT", "IO_L51N_M3A4_3", 51, 0 },
			{ "B2",   3, "LT", "IO_L52P_M3A8_3", 52, 1 },
			{ "A2",   3, "LT", "IO_L52N_M3A9_3", 52, 0 },
			{ "F4",   3, "LT", "IO_L53P_M3CKE_3", 53, 1 },
			{ "F3",   3, "LT", "IO_L53N_M3A12_3", 53, 0 },
			{ "E4",   3, "LT", "IO_L54P_M3RESET_3", 54, 1 },
			{ "E3",   3, "LT", "IO_L54N_M3A11_3", 54, 0 },
			{ "F6",   3, "LT", "IO_L55P_M3A13_3", 55, 1 },
			{ "F5",   3, "LT", "IO_L55N_M3A14_3", 55, 0 },
			{ "B3",   3, "LT", "IO_L83P_3", 83, 1 },
			{ "A3",   3, "LT", "IO_L83N_VREF_3", 83, 0 },
			{ "A1",  -1, "NA", "GND" },
			{ "A16", -1, "NA", "GND" },
			{ "B11", -1, "NA", "GND" },
			{ "B7",	 -1, "NA", "GND" },
			{ "D13", -1, "NA", "GND" },
			{ "D4",	 -1, "NA", "GND" },
			{ "E9",	 -1, "NA", "GND" },
			{ "G15", -1, "NA", "GND" },
			{ "G2",	 -1, "NA", "GND" },
			{ "G8",	 -1, "NA", "GND" },
			{ "H12", -1, "NA", "GND" },
			{ "H7",	 -1, "NA", "GND" },
			{ "H9",	 -1, "NA", "GND" },
			{ "J5",	 -1, "NA", "GND" },
			{ "J8",	 -1, "NA", "GND" },
			{ "K7",	 -1, "NA", "GND" },
			{ "K9",	 -1, "NA", "GND" },
			{ "L15", -1, "NA", "GND" },
			{ "L2",	 -1, "NA", "GND" },
			{ "M8",	 -1, "NA", "GND" },
			{ "N13", -1, "NA", "GND" },
			{ "P3",  -1, "NA", "GND" },
			{ "R10", -1, "NA", "GND" },
			{ "R6",	 -1, "NA", "GND" },
			{ "T1",	 -1, "NA", "GND" },
			{ "T16", -1, "NA", "GND" },
			{ "E5",	 -1, "NA", "VCCAUX" },
			{ "F11", -1, "NA", "VCCAUX" },
			{ "F8",	 -1, "NA", "VCCAUX" },
			{ "G10", -1, "NA", "VCCAUX" },
			{ "H6",	 -1, "NA", "VCCAUX" },
			{ "J10", -1, "NA", "VCCAUX" },
			{ "L6",	 -1, "NA", "VCCAUX" },
			{ "L9",	 -1, "NA", "VCCAUX" },
			{ "G7",	 -1, "NA", "VCCINT" },
			{ "G9",	 -1, "NA", "VCCINT" },
			{ "H10", -1, "NA", "VCCINT" },
			{ "H8",	 -1, "NA", "VCCINT" },
			{ "J7",	 -1, "NA", "VCCINT" },
			{ "J9",	 -1, "NA", "VCCINT" },
			{ "K10", -1, "NA", "VCCINT" },
			{ "K8",	 -1, "NA", "VCCINT" },
			{ "B13",  0, "NA", "VCCO_0" },
			{ "B4",	  0, "NA", "VCCO_0" },
			{ "B9",	  0, "NA", "VCCO_0" },
			{ "D10",  0, "NA", "VCCO_0" },
			{ "D7",	  0, "NA", "VCCO_0" },
			{ "D15",  1, "NA", "VCCO_1" },
			{ "G13",  1, "NA", "VCCO_1" },
			{ "J15",  1, "NA", "VCCO_1" },
			{ "K13",  1, "NA", "VCCO_1" },
			{ "N15",  1, "NA", "VCCO_1" },
			{ "R13",  1, "NA", "VCCO_1" },
			{ "N10",  2, "NA", "VCCO_2" },
			{ "N7",	  2, "NA", "VCCO_2" },
			{ "R4",	  2, "NA", "VCCO_2" },
			{ "R8",	  2, "NA", "VCCO_2" },
			{ "D2",	  3, "NA", "VCCO_3" },
			{ "G4",	  3, "NA", "VCCO_3" },
			{ "J2",	  3, "NA", "VCCO_3" },
			{ "K4",	  3, "NA", "VCCO_3" },
			{ "N2",	  3, "NA", "VCCO_3" }}};
	switch (pkg) {
		case TQG144: return &pkg_tqg144;
		case FTG256: return &pkg_ftg256;
		default: ;
	}
	HERE();
	return 0;
}

const char *xc6_find_pkg_pin(const struct xc6_pkg_info *pkg_info, const char *description)
{
	int i;

	for (i = 0; i < pkg_info->num_pins; i++) {
		if (!strcmp(pkg_info->pin[i].description, description))
			return pkg_info->pin[i].name;
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
