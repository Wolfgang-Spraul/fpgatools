//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

void free_devices(struct fpga_model* model)
{
	int i, j;
	for (i = 0; i < model->x_width * model->y_height; i++) {
		if (!model->tiles[i].num_devs)
			continue;
		EXIT(!model->tiles[i].devs);
		for (j = 0; j < model->tiles[i].num_devs; j++) {
			if (model->tiles[i].devs[j].type != DEV_LOGIC)
				continue;
			free(model->tiles[i].devs[i].logic.A6_lut);
			model->tiles[i].devs[i].logic.A6_lut = 0;
			free(model->tiles[i].devs[i].logic.B6_lut);
			model->tiles[i].devs[i].logic.B6_lut = 0;
			free(model->tiles[i].devs[i].logic.C6_lut);
			model->tiles[i].devs[i].logic.C6_lut = 0;
			free(model->tiles[i].devs[i].logic.D6_lut);
			model->tiles[i].devs[i].logic.D6_lut = 0;
		}
		free(model->tiles[i].devs);
		model->tiles[i].devs = 0;
		model->tiles[i].num_devs = 0;
	}
}

#define DEV_INCREMENT 8

static int add_dev(struct fpga_model* model,
	int y, int x, int type, int subtype)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	if (!(tile->num_devs % DEV_INCREMENT)) {
		void* new_ptr = realloc(tile->devs,
			(tile->num_devs+DEV_INCREMENT)*sizeof(*tile->devs));
		EXIT(!new_ptr);
		memset(new_ptr + tile->num_devs * sizeof(*tile->devs), 0, DEV_INCREMENT*sizeof(*tile->devs));
		tile->devs = new_ptr;
	}
	tile->devs[tile->num_devs].type = type;
	if (type == DEV_IOB)
		tile->devs[tile->num_devs].iob.subtype = subtype;
	else if (type == DEV_LOGIC)
		tile->devs[tile->num_devs].logic.subtype = subtype;
	tile->num_devs++;
	return 0;
}

int init_devices(struct fpga_model* model)
{
	int x, y, i, j, rc;

	// DCM, PLL
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW-1 + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs
		x = model->center_x-CENTER_CMTPLL_O;
		if (i%2) {
			if ((rc = add_dev(model, y, x, DEV_DCM, 0))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_DCM, 0))) goto fail;
		} else
			if ((rc = add_dev(model, y, x, DEV_PLL, 0))) goto fail;
	}

	// BSCAN
	y = TOP_IO_TILES;
	x = model->x_width-RIGHT_IO_DEVS_O;
	if ((rc = add_dev(model, y, x, DEV_BSCAN, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BSCAN, 0))) goto fail;

	// BSCAN, OCT_CALIBRATE
	y = TOP_IO_TILES+1;
	x = model->x_width-RIGHT_IO_DEVS_O;
	if ((rc = add_dev(model, y, x, DEV_BSCAN, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BSCAN, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_OCT_CALIBRATE, 0))) goto fail;

	// ICAP, SPI_ACCESS, OCT_CALIBRATE
	y = model->y_height-BOT_IO_TILES-1;
	x = model->x_width-RIGHT_IO_DEVS_O;
	if ((rc = add_dev(model, y, x, DEV_ICAP, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_SPI_ACCESS, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_OCT_CALIBRATE, 0))) goto fail;

	// STARTUP, POST_CRC_INTERNAL, SLAVE_SPI, SUSPEND_SYNC
	y = model->y_height-BOT_IO_TILES-2;
	x = model->x_width-RIGHT_IO_DEVS_O;
	if ((rc = add_dev(model, y, x, DEV_STARTUP, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_POST_CRC_INTERNAL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_SLAVE_SPI, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_SUSPEND_SYNC, 0))) goto fail;

	// BUFGMUX
	y = model->center_y;
	x = model->center_x;
	for (i = 0; i < 16; i++)
		if ((rc = add_dev(model, y, x, DEV_BUFGMUX, 0))) goto fail;

	// BUFIO, BUFIO_FB, BUFPLL, BUFPLL_MCB
	y = TOP_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL_MCB, 0))) goto fail;
	for (j = 0; j < 8; j++) {
		if ((rc = add_dev(model, y, x, DEV_BUFIO, 0))) goto fail;
		if ((rc = add_dev(model, y, x, DEV_BUFIO_FB, 0))) goto fail;
	}
	y = model->center_y;
	x = LEFT_OUTER_COL;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL_MCB, 0))) goto fail;
	for (j = 0; j < 8; j++) {
		if ((rc = add_dev(model, y, x, DEV_BUFIO, 0))) goto fail;
		if ((rc = add_dev(model, y, x, DEV_BUFIO_FB, 0))) goto fail;
	}
	y = model->center_y;
	x = model->x_width - RIGHT_OUTER_O;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL_MCB, 0))) goto fail;
	for (j = 0; j < 8; j++) {
		if ((rc = add_dev(model, y, x, DEV_BUFIO, 0))) goto fail;
		if ((rc = add_dev(model, y, x, DEV_BUFIO_FB, 0))) goto fail;
	}
	y = model->y_height - BOT_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL, 0))) goto fail;
	if ((rc = add_dev(model, y, x, DEV_BUFPLL_MCB, 0))) goto fail;
	for (j = 0; j < 8; j++) {
		if ((rc = add_dev(model, y, x, DEV_BUFIO, 0))) goto fail;
		if ((rc = add_dev(model, y, x, DEV_BUFIO_FB, 0))) goto fail;
	}
	
	// BUFH
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs
		x = model->center_x;
		for (j = 0; j < 32; j++)
			if ((rc = add_dev(model, y, x, DEV_BUFH, 0))) goto fail;
	}

	// BRAM
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_FABRIC_BRAM_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
			if (!(YX_TILE(model, y, x)->flags & TF_BRAM_DEV))
				continue;
			if ((rc = add_dev(model, y, x, DEV_BRAM16, 0)))
				goto fail;
			if ((rc = add_dev(model, y, x, DEV_BRAM8, 0)))
				goto fail;
			if ((rc = add_dev(model, y, x, DEV_BRAM8, 0)))
				goto fail;
		}
	}

	// MACC
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_FABRIC_MACC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
			if (!(YX_TILE(model, y, x)->flags & TF_MACC_DEV))
				continue;
			if ((rc = add_dev(model, y, x, DEV_MACC, 0))) goto fail;
		}
	}

	// ILOGIC/OLOGIC/IODELAY
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (!is_atx(X_LOGIC_COL, model, x)
		    || is_atx(X_ROUTING_NO_IO, model, x-1))
			continue;
		for (i = 0; i <= 1; i++) {
			y = TOP_IO_TILES+i;
			for (j = 0; j <= 1; j++) {
				if ((rc = add_dev(model, y, x, DEV_ILOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_OLOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_IODELAY, 0)))
					goto fail;
			}
			y = model->y_height-BOT_IO_TILES-i-1;
			for (j = 0; j <= 1; j++) {
				if ((rc = add_dev(model, y, x, DEV_ILOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_OLOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_IODELAY, 0)))
					goto fail;
			}
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_LEFT_WIRED, model, y)) {
			x = LEFT_IO_DEVS;
			for (j = 0; j <= 1; j++) {
				if ((rc = add_dev(model, y, x, DEV_ILOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_OLOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_IODELAY, 0)))
					goto fail;
			}
		}
		if (is_aty(Y_RIGHT_WIRED, model, y)) {
			x = model->x_width-RIGHT_IO_DEVS_O;
			for (j = 0; j <= 1; j++) {
				if ((rc = add_dev(model, y, x, DEV_ILOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_OLOGIC, 0)))
					goto fail;
				if ((rc = add_dev(model, y, x, DEV_IODELAY, 0)))
					goto fail;
			}
		}
	}
	// IOB
	for (x = 0; x < model->x_width; x++) {
		// Note that the order of sub-types IOBM and IOBS must match
		// the order in the control.c sitename arrays.
		if (is_atx(X_OUTER_LEFT, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!is_aty(Y_LEFT_WIRED, model, y))
					continue;
				if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
			}
		}
		if (is_atx(X_OUTER_RIGHT, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!is_aty(Y_RIGHT_WIRED, model, y))
					continue;
				if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
			}
		}
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x)) {
			y = TOP_OUTER_ROW;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;

			y = model->y_height-BOT_OUTER_ROW;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
		}
	}

	// TIEOFF
	y = model->center_y;
	x = LEFT_OUTER_COL;
	if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
	y = model->center_y;
	x = model->x_width-RIGHT_OUTER_O;
	if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
	y = TOP_OUTER_ROW;
	x = model->center_x-1;
	if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
	y = model->y_height-BOT_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_LEFT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!is_aty(Y_LEFT_WIRED, model, y))
					continue;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
			}
		}
		if (is_atx(X_RIGHT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!is_aty(Y_RIGHT_WIRED, model, y))
					continue;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
			}
		}
		if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!(YX_TILE(model, y, x)->flags & TF_PLL_DEV))
					continue;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
				
			}
		}
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
			}
		}
		if (is_atx(X_LOGIC_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x-1)) {
			for (i = 0; i <= 1; i++) {
				y = TOP_IO_TILES+i;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
				y = model->y_height-BOT_IO_TILES-i-1;
				if ((rc = add_dev(model, y, x, DEV_TIEOFF, 0))) goto fail;
			}
		}
	}
	// LOGIC
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_LOGIC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (YX_TILE(model, y, x)->flags & TF_LOGIC_XM_DEV) {
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_X))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_M))) goto fail;
			}
			if (YX_TILE(model, y, x)->flags & TF_LOGIC_XL_DEV) {
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_X))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_L))) goto fail;
			}
		}
	}
	return 0;
fail:
	return rc;
}
