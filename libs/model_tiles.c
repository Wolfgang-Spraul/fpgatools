//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

int init_tiles(struct fpga_model* model)
{
	int tile_rows, tile_columns, i, j, k, l, row_top_y, left_side;
	int start, end, no_io, cur_major;
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
	for (i = 0; i < LEFT_SIDE_WIDTH; i++)
		model->x_major[i] = LEFT_SIDE_MAJOR;
	cur_major = LEFT_SIDE_MAJOR+1;
	// i is now LEFT_SIDE_WIDTH (5)
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
				model->tiles[i+1].flags |= 
					(cur_cfgcol == 'L' || cur_cfgcol == 'l')
						? TF_FABRIC_LOGIC_XL_COL
						: TF_FABRIC_LOGIC_XM_COL;
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

				for (k = 0; k < 2; k++)
					model->x_major[i++] = cur_major;
				cur_major++;
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
				model->tiles[i+1].flags |= TF_FABRIC_BRAM_VIA_COL;
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

				for (k = 0; k < 3; k++)
					model->x_major[i++] = cur_major;
				cur_major++;
				break;

			case 'D':
				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i].flags |= TF_ROUTING_NO_IO; // no_io always on for MACC
				model->tiles[i+1].flags |= TF_FABRIC_MACC_VIA_COL;
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

				for (k = 0; k < 3; k++)
					model->x_major[i++] = cur_major;
				cur_major++;
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
							else if (k == model->cfg_rows/4) {
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].flags |= TF_CENTER_MIDBUF;
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_HCLKBUF_B;
							} else
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_BRK;
						} else if (l == 0 && k == model->cfg_rows*3/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].flags |= TF_CENTER_MIDBUF;
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_HCLKBUF_T;
						} else if (l == 0 && k == model->cfg_rows/4-1)
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_MIDBUF_B;
						else if (l == 8)
							model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = (k<model->cfg_rows/2) ? REGV_B : REGV_T;
						else
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

				for (k = 0; k < 4; k++)
					model->x_major[i++] = cur_major;
				cur_major++;
				break;
			case ' ': // space used to make string more readable only
			case 'g': // global clock separator
			case 'n': // noio for logic blocks
				break;
			default:
				fprintf(stderr, "Ignoring unexpected column "
					"identifier '%c'\n", cur_cfgcol);
				break;
		}
	}
	for (k = 0; k < RIGHT_SIDE_WIDTH; k++)
		model->x_major[i++] = cur_major;

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
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].flags |= TF_IOLOGIC_DELAY_DEV;
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
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_IO_VIA_R;
			} else {
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
