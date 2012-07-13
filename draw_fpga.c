//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

struct fpga_model
{
	int tile_x_range, tile_y_range;
	struct fpga_tile* tiles;
};

enum fpga_tile_type
{
	NA,
	ROUTING,
	ROUTING_BRK,
	ROUTING_VIA,
	HCLK_ROUTING_XM,
	HCLK_ROUTING_XL,
	HCLK_LOGIC_XM,
	HCLK_LOGIC_XL,
	LOGIC_XM,
	LOGIC_XL,
	REGH_ROUTING_XM,
	REGH_ROUTING_XL,
	REGH_LOGIC_XM,
	REGH_LOGIC_XL,
	BRAM_ROUTING,
	BRAM_ROUTING_BRK,
	BRAM,
	BRAM_ROUTING_T,
	BRAM_ROUTING_TERM_T,
	BRAM_ROUTING_TERM_B,
	BRAM_ROUTING_B,
	BRAM_ROUTING_VIA_T,
	BRAM_ROUTING_VIA_TERM_T,
	BRAM_ROUTING_VIA_TERM_B,
	BRAM_ROUTING_VIA_B,
	BRAM_T,
	BRAM_TERM_LT,
	BRAM_TERM_RT,
	BRAM_TERM_LB,
	BRAM_TERM_RB,
	BRAM_B,
	HCLK_BRAM_ROUTING,
	HCLK_BRAM_ROUTING_VIA,
	HCLK_BRAM,
	REGH_BRAM_ROUTING,
	REGH_BRAM_ROUTING_VIA,
	REGH_BRAM_L,
	REGH_BRAM_R,
	MACC,
	HCLK_MACC_ROUTING,
	HCLK_MACC_ROUTING_VIA,
	HCLK_MACC,
	REGH_MACC_ROUTING,
	REGH_MACC_ROUTING_VIA,
	REGH_MACC_L,
	PLL_T,
	DCM_T,
	PLL_B,
	DCM_B,
	REG_T,
	REG_TERM_T,
	REG_TERM_B,
	REG_B,
	REGV_TERM_T,
	REGV_TERM_B,
	HCLK_REGV,
	REGV,
	REGV_BRK,
	REGV_T,
	REGV_B,
	REGV_MIDBUF_T,
	REGV_HCLKBUF_T,
	REGV_HCLKBUF_B,
	REGV_MIDBUF_B,
	REGC_ROUTING,
	REGC_LOGIC,
	REGC_CMT,
	CENTER, // unique center tile in the middle of the chip
	IO_T,
	IO_B,
	IO_TERM_T,
	IO_TERM_B,
	IO_ROUTING,
	IO_LOGIC_TERM_T,
	IO_LOGIC_TERM_B,
	IO_OUTER_T,
	IO_INNER_T,
	IO_OUTER_B,
	IO_INNER_B,
	IO_BUFPLL_TERM_T,
	IO_LOGIC_REG_TERM_T,
	IO_BUFPLL_TERM_B,
	IO_LOGIC_REG_TERM_B,
	LOGIC_ROUTING_TERM_B,
	LOGIC_EMPTY_TERM_B,
	MACC_ROUTING_EMPTY_T,
	MACC_ROUTING_EMPTY_B,
	MACC_ROUTING_TERM_T,
	MACC_ROUTING_TERM_B,
	MACC_VIA_EMPTY,
	MACC_VIA_TERM_T,
	MACC_EMPTY_T,
	MACC_EMPTY_B,
	MACC_TERM_TL,
	MACC_TERM_TR,
	MACC_TERM_BL,
	MACC_TERM_BR,
	ROUTING_VIA_REGC,
	ROUTING_VIA_IO,
	ROUTING_VIA_IO_DCM,
	ROUTING_VIA_CARRY,
};

const char* fpga_ttstr[] = // tile type strings
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
	[BRAM_ROUTING_T] = "BRAM_ROUTING_T",
	[BRAM_ROUTING_TERM_T] = "BRAM_ROUTING_TERM_T",
	[BRAM_ROUTING_TERM_B] = "BRAM_ROUTING_TERM_B",
	[BRAM_ROUTING_B] = "BRAM_ROUTING_B",
	[BRAM_ROUTING_VIA_T] = "BRAM_ROUTING_VIA_T",
	[BRAM_ROUTING_VIA_TERM_T] = "BRAM_ROUTING_VIA_TERM_T",
	[BRAM_ROUTING_VIA_TERM_B] = "BRAM_ROUTING_VIA_TERM_B",
	[BRAM_ROUTING_VIA_B] = "BRAM_ROUTING_VIA_B",
	[BRAM_T] = "BRAM_T",
	[BRAM_TERM_LT] = "BRAM_TERM_LT",
	[BRAM_TERM_RT] = "BRAM_TERM_RT",
	[BRAM_TERM_LB] = "BRAM_TERM_LB",
	[BRAM_TERM_RB] = "BRAM_TERM_RB",
	[BRAM_B] = "BRAM_B",
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
	[LOGIC_EMPTY_TERM_B] = "LOGIC_EMPTY_TERM_B",
	[MACC_ROUTING_EMPTY_T] = "MACC_ROUTING_EMPTY_T",
	[MACC_ROUTING_EMPTY_B] = "MACC_ROUTING_EMPTY_B",
	[MACC_ROUTING_TERM_T] = "MACC_ROUTING_TERM_T",
	[MACC_ROUTING_TERM_B] = "MACC_ROUTING_TERM_B",
	[MACC_VIA_EMPTY] = "MACC_VIA_EMPTY",
	[MACC_VIA_TERM_T] = "MACC_VIA_TERM_T",
	[MACC_EMPTY_T] = "MACC_EMPTY_T",
	[MACC_EMPTY_B] = "MACC_EMPTY_B",
	[MACC_TERM_TL] = "MACC_TERM_TL",
	[MACC_TERM_TR] = "MACC_TERM_TR",
	[MACC_TERM_BL] = "MACC_TERM_BL",
	[MACC_TERM_BR] = "MACC_TERM_BR",
	[ROUTING_VIA_REGC] = "ROUTING_VIA_REGC",
	[ROUTING_VIA_IO] = "ROUTING_VIA_IO",
	[ROUTING_VIA_IO_DCM] = "ROUTING_VIA_IO_DCM",
	[ROUTING_VIA_CARRY] = "ROUTING_VIA_CARRY",
};

struct fpga_tile
{
	enum fpga_tile_type type;
};

// columns
//  'L' = X+L logic block
//  'l' = X+L logic block without IO at top and bottom
//  'M' = X+M logic block
//  'm' = X+M logic block without IO at top and bottom
//  'B' = block ram
//  'D' = dsp (macc)
//  'R' = registers and center IO/logic column

#define XC6SLX9_ROWS	4
#define XC6SLX9_COLUMNS	"MLBMLDMRMlMLBML"

struct fpga_model* build_model(int fpga_rows, const char* columns);
void printf_model(struct fpga_model* model);

int main(int argc, char** argv)
{
	struct fpga_model* model = 0;

	//
	// build memory model
	//

	model = build_model(XC6SLX9_ROWS, XC6SLX9_COLUMNS);
	if (!model) goto fail;

	//
	// write svg
	//

	printf_model(model);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}

struct fpga_model* build_model(int fpga_rows, const char* columns)
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
	i = 5; // left IO columns
	center_row = 2 /* top IO files */ + (fpga_rows/2)*(8+1/*middle of row clock*/+8);
	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
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
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = LOGIC_EMPTY_TERM_B;
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

				model->tiles[i].type = BRAM_ROUTING_T;
				model->tiles[tile_columns + i].type = BRAM_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = BRAM_ROUTING_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i].type = BRAM_ROUTING_B;
				model->tiles[i + 1].type = BRAM_ROUTING_VIA_T;
				model->tiles[tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 1].type = BRAM_ROUTING_VIA_B;
				model->tiles[i + 2].type = BRAM_T;
				model->tiles[tile_columns + i + 2].type = left_side ? BRAM_TERM_LT : BRAM_TERM_RT;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? BRAM_TERM_LB : BRAM_TERM_RB;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = BRAM_B;

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

				model->tiles[i].type = MACC_ROUTING_EMPTY_T;
				model->tiles[tile_columns + i].type = MACC_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = MACC_ROUTING_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i].type = MACC_ROUTING_EMPTY_B;
				model->tiles[i + 1].type = MACC_VIA_EMPTY;
				model->tiles[tile_columns + i + 1].type = MACC_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 1].type = MACC_VIA_EMPTY;
				model->tiles[i + 2].type = MACC_EMPTY_T;
				model->tiles[tile_columns + i + 2].type = left_side ? MACC_TERM_TL : MACC_TERM_TR;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? MACC_TERM_BL : MACC_TERM_BR;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = MACC_EMPTY_B;

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
	return model;
}

void printf_model(struct fpga_model* model)
{
	static const xmlChar* empty_svg = (const xmlChar*)
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<svg version=\"2.0\"\n"
		"   xmlns=\"http://www.w3.org/2000/svg\"\n"
		"   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
		"   xmlns:fpga=\"http://qi-hw.com/fpga\"\n"
		"   id=\"root\">\n"
		"<style type=\"text/css\"><![CDATA[text{font-size:6pt;text-anchor:end;}]]></style>\n"
		"</svg>\n";

	xmlDocPtr doc = 0;
	xmlXPathContextPtr xpathCtx = 0;
	xmlXPathObjectPtr xpathObj = 0;
	xmlNodePtr new_node;
	char str[128];
	int i, j;

// can't get indent formatting to work - use 'xmllint --pretty 1 -'
// on the output for now

	xmlInitParser();
	doc = xmlParseDoc(empty_svg);
	if (!doc) {
		fprintf(stderr, "Internal error %i.\n", __LINE__);
		goto fail;
	}
	xpathCtx = xmlXPathNewContext(doc);
	if (!xpathCtx) {
		fprintf(stderr, "Cannot create XPath context.\n");
		goto fail;
	}
	xmlXPathRegisterNs(xpathCtx, BAD_CAST "svg", BAD_CAST "http://www.w3.org/2000/svg");
	xpathObj = xmlXPathEvalExpression(BAD_CAST "//svg:*[@id='root']", xpathCtx);
	if (!xpathObj) {
		fprintf(stderr, "Cannot evaluate root expression.\n");
		goto fail;
	}
	if (!xpathObj->nodesetval) {
		fprintf(stderr, "Cannot find root node.\n");
		goto fail;
	}
	if (xpathObj->nodesetval->nodeNr != 1) {
		fprintf(stderr, "Found %i root nodes.\n", xpathObj->nodesetval->nodeNr);
		goto fail;
	}

	for (i = 0; i < model->tile_y_range; i++) {
		for (j = 0; j < model->tile_x_range; j++) {
			strcpy(str, fpga_ttstr[model->tiles[i*model->tile_x_range+j].type]);
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0], 0 /* xmlNsPtr */, BAD_CAST "text", BAD_CAST str);
			xmlSetProp(new_node, BAD_CAST "x", xmlXPathCastNumberToString(130 + j*130));
			xmlSetProp(new_node, BAD_CAST "y", xmlXPathCastNumberToString(40 + i*20));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_y", BAD_CAST xmlXPathCastNumberToString(i));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_x", BAD_CAST xmlXPathCastNumberToString(j));
		}
	}
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "width", BAD_CAST xmlXPathCastNumberToString(model->tile_x_range * 130 + 65));
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "height", BAD_CAST xmlXPathCastNumberToString(model->tile_y_range * 20 + 60));

	xmlDocFormatDump(stdout, doc, 1 /* format */);
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 
 	xmlCleanupParser();
	return;

fail:
	if (xpathObj) xmlXPathFreeObject(xpathObj);
	if (xpathCtx) xmlXPathFreeContext(xpathCtx); 
	if (doc) xmlFreeDoc(doc); 
 	xmlCleanupParser();
}
