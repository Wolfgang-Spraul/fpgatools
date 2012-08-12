//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

int init_devices(struct fpga_model* model)
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
			tile->devices[tile->num_devices++].type = DEV_PLL;
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
