//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"
#include "control.h"

static int add_dev(struct fpga_model* model,
	int y, int x, int type, int subtype);
static int init_iob(struct fpga_model* model, int y, int x, int idx);
static int init_logic(struct fpga_model* model, int y, int x, int idx);

int init_devices(struct fpga_model* model)
{
	int x, y, i, j, rc;

	RC_CHECK(model);
	// DCM, PLL
	for (i = 0; i < model->die->num_rows; i++) {
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

	// MCB
	if ((rc = add_dev(model, model->die->mcb_ypos, LEFT_MCB_COL, DEV_MCB, 0))) goto fail;
	if ((rc = add_dev(model, model->die->mcb_ypos, model->x_width-RIGHT_MCB_O, DEV_MCB, 0))) goto fail;

	// OCT_CALIBRATE
	x = LEFT_IO_DEVS;
	if ((rc = add_dev(model, TOP_IO_TILES, x, DEV_OCT_CALIBRATE, 0)))
		FAIL(rc);
	if ((rc = add_dev(model, TOP_IO_TILES, x, DEV_OCT_CALIBRATE, 0)))
		FAIL(rc);
	if ((rc = add_dev(model, model->y_height-BOT_IO_TILES-1, x,
		DEV_OCT_CALIBRATE, 0))) FAIL(rc);
	if ((rc = add_dev(model, model->y_height-BOT_IO_TILES-1, x,
		DEV_OCT_CALIBRATE, 0))) FAIL(rc);

	// DNA, PMV
	x = LEFT_IO_DEVS;
	y = TOP_IO_TILES;
	if ((rc = add_dev(model, y, x, DEV_DNA, 0))) FAIL(rc);
	if ((rc = add_dev(model, y, x, DEV_PMV, 0))) FAIL(rc);

	// PCILOGIC_SE
	if ((rc = add_dev(model, model->center_y, LEFT_IO_ROUTING,
		DEV_PCILOGIC_SE, 0))) FAIL(rc);
	if ((rc = add_dev(model, model->center_y, model->x_width
		- RIGHT_IO_DEVS_O, DEV_PCILOGIC_SE, 0))) FAIL(rc);

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
	for (i = 0; i < model->die->num_rows; i++) {
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
		if (!is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x)
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
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBS))) goto fail;
			if ((rc = add_dev(model, y, x, DEV_IOB, IOBM))) goto fail;
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
		if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x)
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
		if (!is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			// M and L are at index 0 (DEV_LOG_M_OR_L),
			// X is at index 1 (DEV_LOG_X).
			if (YX_TILE(model, y, x)->flags & TF_LOGIC_XM_DEV) {
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_M))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_X))) goto fail;
			}
			if (YX_TILE(model, y, x)->flags & TF_LOGIC_XL_DEV) {
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_L))) goto fail;
				if ((rc = add_dev(model, y, x, DEV_LOGIC, LOGIC_X))) goto fail;
			}
		}
	}
	return 0;
fail:
	return rc;
}

void free_devices(struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y, i;

	// leave model->rc untouched
	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);
			if (!tile->num_devs)
				continue;
			if (!tile->devs) {
				HERE();
				continue;
			}
			for (i = 0; i < tile->num_devs; i++) {
				fdev_delete(model, y, x, tile->devs[i].type,
					fdev_typeidx(model, y, x, i));
			}
			free(tile->devs);
			tile->devs = 0;
			tile->num_devs = 0;
		}
	}
}

#define DEV_INCREMENT 4

static int add_dev(struct fpga_model* model,
	int y, int x, int type, int subtype)
{
	struct fpga_tile* tile;
	int new_dev_i;
	int rc;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	if (!(tile->num_devs % DEV_INCREMENT)) {
		void* new_ptr = realloc(tile->devs,
			(tile->num_devs+DEV_INCREMENT)*sizeof(*tile->devs));
		EXIT(!new_ptr);
		memset(new_ptr + tile->num_devs * sizeof(*tile->devs),
			0, DEV_INCREMENT*sizeof(*tile->devs));
		tile->devs = new_ptr;
	}
	new_dev_i = tile->num_devs;
	tile->num_devs++;

	// init new device
	tile->devs[new_dev_i].type = type;
	tile->devs[new_dev_i].subtype = subtype;
	if (type == DEV_IOB) {
		rc = init_iob(model, y, x, new_dev_i);
		if (rc) FAIL(rc);
	} else if (type == DEV_LOGIC) {
		rc = init_logic(model, y, x, new_dev_i);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int init_iob(struct fpga_model* model, int y, int x, int idx)
{
	struct fpga_tile* tile;
	const char* prefix;
	int type_idx, rc;
	char tmp_str[128];

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	type_idx = fdev_typeidx(model, y, x, idx);
	if (!y)
		prefix = "TIOB";
	else if (y == model->y_height - BOT_OUTER_ROW)
		prefix = "BIOB";
	else if (x == 0)
		prefix = "LIOB";
	else if (x == model->x_width - RIGHT_OUTER_O)
		prefix = "RIOB";
	else
		FAIL(EINVAL);

	tile->devs[idx].pinw = calloc((IOB_LAST_OUTPUT_PINW+1)
		*sizeof(tile->devs[idx].pinw[0]), /*elsize*/ 1);
	if (!tile->devs[idx].pinw) FAIL(ENOMEM);
	tile->devs[idx].num_pinw_total = IOB_LAST_OUTPUT_PINW+1;
	tile->devs[idx].num_pinw_in = IOB_LAST_INPUT_PINW+1;

	snprintf(tmp_str, sizeof(tmp_str), "%s_O%i_PINW", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_IN_O], 0);
	if (rc) FAIL(rc);
	snprintf(tmp_str, sizeof(tmp_str), "%s_T%i_PINW", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_IN_T], 0);
	if (rc) FAIL(rc);
	snprintf(tmp_str, sizeof(tmp_str), "%s_DIFFI_IN%i", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_IN_DIFFI_IN], 0);
	if (rc) FAIL(rc);
	snprintf(tmp_str, sizeof(tmp_str), "%s_DIFFO_IN%i", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_IN_DIFFO_IN], 0);
	if (rc) FAIL(rc);

	snprintf(tmp_str, sizeof(tmp_str), "%s_IBUF%i_PINW", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_OUT_I], 0);
	if (rc) FAIL(rc);
	snprintf(tmp_str, sizeof(tmp_str), "%s_PADOUT%i", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_OUT_PADOUT], 0);
	if (rc) FAIL(rc);
	snprintf(tmp_str, sizeof(tmp_str), "%s_DIFFO_OUT%i", prefix, type_idx);
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_OUT_DIFFO_OUT], 0);
	if (rc) FAIL(rc);

	if (!x && y == model->center_y - CENTER_TOP_IOB_O && type_idx == 1)
		strcpy(tmp_str, "LIOB_TOP_PCI_RDY0");
	else if (!x && y == model->center_y + CENTER_BOT_IOB_O && type_idx == 0)
		strcpy(tmp_str, "LIOB_BOT_PCI_RDY0");
	else if (x == model->x_width-RIGHT_OUTER_O && y == model->center_y - CENTER_TOP_IOB_O && type_idx == 0)
		strcpy(tmp_str, "RIOB_BOT_PCI_RDY0");
	else if (x == model->x_width-RIGHT_OUTER_O && y == model->center_y + CENTER_BOT_IOB_O && type_idx == 1)
		strcpy(tmp_str, "RIOB_TOP_PCI_RDY1");
	else {
		snprintf(tmp_str, sizeof(tmp_str),
			"%s_PCI_RDY%i", prefix, type_idx);
	}
	rc = add_connpt_name(model, y, x, tmp_str, /*dup_warn*/ 1,
		&tile->devs[idx].pinw[IOB_OUT_PCI_RDY], 0);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

static int init_logic(struct fpga_model* model, int y, int x, int idx)
{
	struct fpga_tile* tile;
	const char* pre;
	int i, j, rc;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	if (tile->devs[idx].subtype == LOGIC_M)
		pre = "M_";
	else if (tile->devs[idx].subtype == LOGIC_L)
		pre = "L_";
	else if (tile->devs[idx].subtype == LOGIC_X) {
		pre = is_atx(X_FABRIC_LOGIC_XL_COL|X_CENTER_LOGIC_COL, model, x)
			? "XX_" : "X_";
	} else FAIL(EINVAL);

	tile->devs[idx].pinw = calloc((LO_LAST+1)
		*sizeof(tile->devs[idx].pinw[0]), /*elsize*/ 1);
	if (!tile->devs[idx].pinw) FAIL(ENOMEM);
	tile->devs[idx].num_pinw_total = LO_LAST+1;
	tile->devs[idx].num_pinw_in = LI_LAST+1;

	for (i = 0; i < 4; i++) { // 'A' to 'D'
		for (j = 0; j < 6; j++) {
			rc = add_connpt_name(model, y, x, pf("%s%c%i", pre, 'A'+i, j+1),
				/*dup_warn*/ 1,
				&tile->devs[idx].pinw[LI_A1+i*6+j], 0);
			if (rc) FAIL(rc);
		}
		rc = add_connpt_name(model, y, x, pf("%s%cX", pre, 'A'+i),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LI_AX+i], 0);
		if (rc) FAIL(rc);
		if (tile->devs[idx].subtype == LOGIC_M) {
			rc = add_connpt_name(model, y, x, pf("%s%cI", pre, 'A'+i),
				/*dup_warn*/ 1,
				&tile->devs[idx].pinw[LI_AI+i], 0);
			if (rc) FAIL(rc);
		} else
			tile->devs[idx].pinw[LI_AI+i] = STRIDX_NO_ENTRY;
		rc = add_connpt_name(model, y, x, pf("%s%c", pre, 'A'+i),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LO_A+i], 0);
		if (rc) FAIL(rc);
		rc = add_connpt_name(model, y, x, pf("%s%cMUX", pre, 'A'+i),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LO_AMUX+i], 0);
		if (rc) FAIL(rc);
		rc = add_connpt_name(model, y, x, pf("%s%cQ", pre, 'A'+i),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LO_AQ+i], 0);
		if (rc) FAIL(rc);
	}
	rc = add_connpt_name(model, y, x, pf("%sCLK", pre),
		/*dup_warn*/ 1,
		&tile->devs[idx].pinw[LI_CLK], 0);
	if (rc) FAIL(rc);
	rc = add_connpt_name(model, y, x, pf("%sCE", pre),
		/*dup_warn*/ 1,
		&tile->devs[idx].pinw[LI_CE], 0);
	if (rc) FAIL(rc);
	rc = add_connpt_name(model, y, x, pf("%sSR", pre),
		/*dup_warn*/ 1,
		&tile->devs[idx].pinw[LI_SR], 0);
	if (rc) FAIL(rc);
	if (tile->devs[idx].subtype == LOGIC_M) {
		rc = add_connpt_name(model, y, x, pf("%sWE", pre),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LI_WE], 0);
		if (rc) FAIL(rc);
	} else
		tile->devs[idx].pinw[LI_WE] = STRIDX_NO_ENTRY;
	if (tile->devs[idx].subtype != LOGIC_X) {
		// Wire connections will go to some CIN later
		// (and must not warn about duplicates), but we
		// have to add the connection point here so
		// that pinw[LI_CIN] is initialized.
		rc = add_connpt_name(model, y, x, pf("%sCIN", pre),
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LI_CIN], 0);
		if (rc) FAIL(rc);
	} else
		tile->devs[idx].pinw[LI_CIN] = STRIDX_NO_ENTRY;
	if (tile->devs[idx].subtype == LOGIC_M) {
		rc = add_connpt_name(model, y, x, "M_COUT",
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LO_COUT], 0);
		if (rc) FAIL(rc);
	} else if (tile->devs[idx].subtype == LOGIC_L) {
		rc = add_connpt_name(model, y, x, "XL_COUT",
			/*dup_warn*/ 1,
			&tile->devs[idx].pinw[LO_COUT], 0);
		if (rc) FAIL(rc);
	} else 
		tile->devs[idx].pinw[LO_COUT] = STRIDX_NO_ENTRY;

	return 0;
fail:
	return rc;
}
