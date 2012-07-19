//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"

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

struct fpga_model* fpga_build_model(int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring)
{
	int tile_rows, tile_columns, i, j, k, l, row_top_y, center_row, left_side;
	int start, end;
	struct fpga_model* model;

	tile_rows = 1 /* middle */ + (8+1+8)*fpga_rows + 2+2 /* two extra tiles at top and bottom */;
	tile_columns = 5 /* left */ + 5 /* right */;
	for (i = 0; columns[i] != 0; i++) {
		tile_columns += 2; // 2 for logic blocks L/M and minimum for others
		if (columns[i] == 'B' || columns[i] == 'D')
			tile_columns++; // 3 for bram or macc
		else if (columns[i] == 'R')
			tile_columns+=2; // 2+2 for middle IO+logic+PLL/DCM
	}
	model = calloc(1 /* nelem */, sizeof(struct fpga_model));
	if (!model) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return 0;
	}
	model->tile_x_range = tile_columns;
	model->tile_y_range = tile_rows;
	model->tiles = calloc(tile_columns * tile_rows, sizeof(struct fpga_tile));
	if (!model->tiles) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		free(model);
		return 0;
	}
	for (i = 0; i < tile_rows * tile_columns; i++)
		model->tiles[i].type = NA;
	if (!(tile_rows % 2))
		fprintf(stderr, "Unexpected even number of tile rows (%i).\n", tile_rows);
	center_row = 2 /* top IO files */ + (fpga_rows/2)*(8+1/*middle of row clock*/+8);

	//
	// top, bottom, center:
	// go through columns from left to right, rows from top to bottom
	//

	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
	i = 5; // skip left IO columns
	for (j = 0; columns[j]; j++) {
		switch (columns[j]) {
			case 'L':
			case 'l':
			case 'M':
			case 'm':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles (center row)
					start = ((k == fpga_rows-1 && (columns[j] == 'L' || columns[j] == 'M')) ? 2 : 0);
					end = ((k == 0 && (columns[j] == 'L' || columns[j] == 'M')) ? 14 : 16);
					for (l = start; l < end; l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type =
							(l < 15 || (!k && (columns[j] == 'l' || columns[j] == 'm'))) ? ROUTING : ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type
							= (columns[j] == 'L' || columns[j] == 'l') ? LOGIC_XL : LOGIC_XM;
					}
					if (columns[j] == 'L' || columns[j] == 'l') {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					} else {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XM;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XM;
					}
				}

				if (j && columns[j-1] == 'R') {
					model->tiles[tile_columns + i].type = IO_BUFPLL_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i].type = IO_BUFPLL_TERM_B;
				} else {
					model->tiles[tile_columns + i].type = IO_TERM_T;
					if (columns[j] == 'L' || columns[j] == 'M')
						model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i].type = LOGIC_ROUTING_TERM_B;
				}
				if (columns[j] == 'L' || columns[j] == 'M') {
					model->tiles[i].type = IO_T;
					model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
					model->tiles[2*tile_columns + i].type = IO_ROUTING;
					model->tiles[3*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;
				}

				if (j && columns[j-1] == 'R') {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				} else {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_TERM_T;
					if (columns[j] == 'L' || columns[j] == 'M')
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = LOGIC_NOIO_TERM_B;
				}
				if (columns[j] == 'L' || columns[j] == 'M') {
					model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
					model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
				}

				if (columns[j] == 'L' || columns[j] == 'l') {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XL;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XL;
				} else {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XM;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XM;
				}
				i += 2;
				break;
			case 'B':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type = (l < 15) ? BRAM_ROUTING : BRAM_ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = BRAM;
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

				model->tiles[center_row*tile_columns + i].type = REGH_BRAM_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_BRAM_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = left_side ? REGH_BRAM_L : REGH_BRAM_R;
				i += 3;
				break;
			case 'D':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type = (l < 15) ? ROUTING : ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = MACC;
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

				model->tiles[center_row*tile_columns + i].type = REGH_MACC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_MACC_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = REGH_MACC_L;
				i += 3;
				break;
			case 'R':
				if (columns[j+1] != 'M') {
					// We expect a LOGIC_XM column to follow the center for
					// the top and bottom bufpll and reg routing.
					fprintf(stderr, "Expecting LOGIC_XM after center but found '%c'\n", columns[j+1]);
				}
				left_side = 0;
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles

					for (l = 0; l < 16; l++) {

						if ((k < fpga_rows-1 || l >= 2) && (k || l<14)) {
							model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type =
								(l < 15) ? ROUTING : ROUTING_BRK;
							if (l == 7) {
								model->tiles[(row_top_y+l)*tile_columns + i + 1].type = ROUTING_VIA_IO;
							} else if (l == 8) {
								model->tiles[(row_top_y+l+1)*tile_columns + i + 1].type =
									(k%2) ? ROUTING_VIA_CARRY : ROUTING_VIA_IO_DCM;
							} else if (l == 15 && k==fpga_rows/2) {
								model->tiles[(row_top_y+l+1)*tile_columns + i + 1].type = ROUTING_VIA_REGC;
							} else
								model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = LOGIC_XL;
						}
						if (l == 7)
							model->tiles[(row_top_y+l)*tile_columns + i].type = IO_ROUTING;
						if (l == 8 && !(k%2)) // even row, together with DCM
							model->tiles[(row_top_y+l+1)*tile_columns + i].type = IO_ROUTING;

						if (l == 7) {
							if (k%2) // odd
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(fpga_rows/2)) ? PLL_B : PLL_T;
							else // even
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(fpga_rows/2)) ? DCM_B : DCM_T;
						}
						// four midbuf tiles, in the middle of the top and bottom halves
						if (l == 15) {
							if (k == fpga_rows*3/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_MIDBUF_T;
							else if (k == fpga_rows/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_HCLKBUF_B;
							else
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_BRK;
						} else if (l == 0 && k == fpga_rows*3/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_HCLKBUF_T;
						} else if (l == 0 && k == fpga_rows/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_MIDBUF_B;
						} else if (l == 8) {
							model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = (k<fpga_rows/2) ? REGV_B : REGV_T;
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
				model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;

				model->tiles[i + 2].type = REG_T;
				model->tiles[tile_columns + i + 2].type = REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = REG_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = REG_B;
				model->tiles[tile_columns + i + 3].type = REGV_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 3].type = REGV_TERM_B;

				model->tiles[center_row*tile_columns + i].type = REGC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGC_LOGIC;
				model->tiles[center_row*tile_columns + i + 2].type = REGC_CMT;
				model->tiles[center_row*tile_columns + i + 3].type = CENTER;
				i += 4;
				break;
			default:
				fprintf(stderr, "Unexpected column identifier '%c'\n", columns[j]);
					break;
		}
	}

	//
	// left IO
	//

	for (k = fpga_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(fpga_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// +0
			//
			if (left_wiring[(fpga_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns].type = IO_L;
			//
			// +1
			//
			if ((k == fpga_rows-1 && !l) || (!k && l==15))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = CORNER_TERM_L;
			else if (k == fpga_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_TOP;
			else if (k == fpga_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_BOT;
			else if (k == (fpga_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_TOP;
			else if (k == (fpga_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = IO_TERM_L;
			//
			// +2
			//
			if (left_wiring[(fpga_rows-1-k)*16+l] == 'W') {
				if (l == 15 && k && k != fpga_rows/2)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_IO_L_BRK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING_IO_L;
			} else { // unwired
				if (k && k != fpga_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_BRK;
				else if (k == fpga_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING;
			}
			//
			// +3
			//
			if (left_wiring[(fpga_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_IO_VIA_L;
			} else { // unwired
				if (k == fpga_rows-1 && !l) {
					model->tiles[(row_top_y+l)*tile_columns + 3].type = CORNER_TL;
				} else if (!k && l == 15) {
					model->tiles[(row_top_y+l+1)*tile_columns + 3].type = CORNER_BL;
				} else {
					if (k && k != fpga_rows/2 && l == 15)
						model->tiles[(row_top_y+l+1)*tile_columns + 3].type = ROUTING_VIA_CARRY;
					else
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_VIA;
				}
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + 1].type = HCLK_TERM_L;
		model->tiles[(row_top_y+8)*tile_columns + 2].type = HCLK_ROUTING_IO_L;
		if (k >= fpga_rows/2) { // top half
			if (k > (fpga_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_UP_L;
			else if (k == (fpga_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_DN_L;
		} else { // bottom half
			if (k < fpga_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_DN_L;
			else if (k == fpga_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_UP_L;
		}
		model->tiles[(row_top_y+8)*tile_columns + 4].type = HCLK_MCB;
	}

	model->tiles[(center_row-3)*tile_columns].type = IO_PCI_L;
	model->tiles[(center_row-2)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[(center_row-1)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[center_row*tile_columns].type = REG_L;
	model->tiles[(center_row+1)*tile_columns].type = IO_RDY_L;

	model->tiles[center_row*tile_columns + 1].type = REGH_IO_TERM_L;

	model->tiles[tile_columns + 2].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + 2].type = CORNER_TERM_B;
	model->tiles[center_row*tile_columns + 2].type = REGH_ROUTING_IO_L;

	model->tiles[tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[(tile_rows-2)*tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[center_row*tile_columns + 3].type = REGH_IO_L;
	model->tiles[center_row*tile_columns + 4].type = REGH_MCB;

	//
	// right IO
	//

	for (k = fpga_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(fpga_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// -1
			//
			if (k == fpga_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_RDY_R;
			else if (k == fpga_rows/2 && l == 14)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == fpga_rows/2 && l == 15)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == fpga_rows/2-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 1].type = IO_PCI_R;
			else {
				if (right_wiring[(fpga_rows-1-k)*16+l] == 'W')
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 1].type = IO_R;
			}
			//
			// -2
			//
			if ((k == fpga_rows-1 && (!l || l == 1)) || (!k && (l==15 || l==14)))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = CORNER_TERM_R;
			else if (k == fpga_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_TOP;
			else if (k == fpga_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_BOT;
			else if (k == (fpga_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_TOP;
			else if (k == (fpga_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = IO_TERM_R;
			//
			// -3
			//
			//
			// -4
			//
			if (right_wiring[(fpga_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_IO_VIA_R;
			else {
				if (k == fpga_rows-1 && l == 0)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_UPPER;
				else if (k == fpga_rows-1 && l == 1)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_LOWER;
				else if (k && k != fpga_rows/2 && l == 15)
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
			if (right_wiring[(fpga_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = IO_ROUTING;
			else {
				if (k && k != fpga_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_BRK;
				else if (k == fpga_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = ROUTING;
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 2].type = HCLK_TERM_R;
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 3].type = HCLK_MCB;

		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 5].type = HCLK_ROUTING_IO_R;
		if (k >= fpga_rows/2) { // top half
			if (k > (fpga_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_UP_R;
			else if (k == (fpga_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_DN_R;
		} else { // bottom half
			if (k < fpga_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_DN_R;
			else if (k == fpga_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_UP_R;
		}
	}
	model->tiles[tile_columns + tile_columns - 5].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 5].type = CORNER_TERM_B;
	model->tiles[tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[center_row*tile_columns + tile_columns - 1].type = REG_R;
	model->tiles[center_row*tile_columns + tile_columns - 2].type = REGH_IO_TERM_R;
	model->tiles[center_row*tile_columns + tile_columns - 3].type = REGH_MCB;
	model->tiles[center_row*tile_columns + tile_columns - 4].type = REGH_IO_R;
	model->tiles[center_row*tile_columns + tile_columns - 5].type = REGH_ROUTING_IO_R;

	return model;
}

const char* fpga_tiletype_str(enum fpga_tile_type type)
{
	if (type >= sizeof(fpga_ttstr)/sizeof(fpga_ttstr[0])
	    || !fpga_ttstr[type]) return "UNK";
	return fpga_ttstr[type];
}

// Dan Bernstein's hash function
unsigned long hash_djb2(const unsigned char* str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        return hash;
}

//
// The format of each entry in a bin is.
//   uint16_t idx
//   uint16_t entry len including 4-byte header
//   char[]   zero-terminated string
//
// Offsets point to the zero-terminated string, so the len
// is at off-2, the index at off-4. bin0 offset0 can thus be
// used as a special value to signal 'no entry'.
//

const char* strarray_lookup(struct hashed_strarray* array, uint16_t idx)
{
	int bin = array->index_to_bin[idx];
	int offset = array->bin_offsets[idx];

	// bin 0 offset 0 is a special value that signals 'no
	// entry'. Normal offsets cannot be less than 4.
	if (!bin && !offset) return 0;

	if (!array->bin_strings[bin] || offset >= array->bin_len[bin]
	    || offset < 4) {
		// This really should never happen and is an internal error.
		fprintf(stderr, "Internal error.\n");
		return 0;
	}

	return &array->bin_strings[bin][offset];
}

#define BIN_INCREMENT 32768

int strarray_find_or_add(struct hashed_strarray* array, const char* str,
	uint16_t* idx)
{
	int bin, search_off, str_len, i, num_indices, free_index;
	int new_alloclen, start_index;
	unsigned long hash;
	void* new_ptr;

	hash = hash_djb2((const unsigned char*) str);
	str_len = strlen(str);
	bin = hash % (sizeof(array->bin_strings)/sizeof(array->bin_strings[0]));
	// iterate over strings in bin to find match
	if (array->bin_strings[bin]) {
		search_off = 4;
		while (search_off < array->bin_len[bin]) {
			if (!strcmp(&array->bin_strings[bin][search_off],
				str)) {
				*idx = *(uint16_t*)&array->bin_strings
					[bin][search_off-4];
				return 1;
			}
			search_off += *(uint16_t*)&array->bin_strings
				[bin][search_off-2];
		}
	}
	// search free index
	num_indices = sizeof(array->bin_offsets)/sizeof(array->bin_offsets[0]);
	start_index = hash % num_indices;
	for (i = 0; i < num_indices; i++) {
		if (!array->bin_offsets[(start_index+i)%num_indices])
			break;
	}
	if (i >= num_indices) {
		fprintf(stderr, "All array indices full.\n");
		return 0;
	}
	free_index = (start_index+i)%num_indices;
	// check whether bin needs expansion
	if (!(array->bin_len[bin]%BIN_INCREMENT)
	    || array->bin_len[bin]%BIN_INCREMENT + 4+str_len+1 > BIN_INCREMENT)
	{
		new_alloclen = 
 			((array->bin_len[bin]
				+ 4+str_len+1)/BIN_INCREMENT + 1)
			  * BIN_INCREMENT;
		new_ptr = realloc(array->bin_strings[bin], new_alloclen);
		if (!new_ptr) {
			fprintf(stderr, "Out of memory.\n");
			return 0;
		}
		array->bin_strings[bin] = new_ptr;
	}
	// append new string at end of bin
	*(uint16_t*)&array->bin_strings[bin][array->bin_len[bin]] = free_index;
	*(uint16_t*)&array->bin_strings[bin][array->bin_len[bin]+2] = 4+str_len+1;
	strcpy(&array->bin_strings[bin][array->bin_len[bin]+4], str);
	array->index_to_bin[free_index] = bin;
	array->bin_offsets[free_index] = array->bin_len[bin]+4;
	array->bin_len[bin] += 4+str_len+1;
	*idx = free_index;
	return 1;
}

void strarray_init(struct hashed_strarray* array)
{
	memset(array, 0, sizeof(*array));
}

void strarray_free(struct hashed_strarray* array)
{
	int i;
	for (i = 0; i < sizeof(array->bin_strings)/
		sizeof(array->bin_strings[0]); i++) {
		free(array->bin_strings[i]);
		array->bin_strings[i] = 0;
	}
}
