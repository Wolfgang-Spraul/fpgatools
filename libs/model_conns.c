//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"
#include "parts.h"

static int term_topbot(struct fpga_model *model);
static int term_leftright(struct fpga_model *model);
static int term_to_io(struct fpga_model *model, enum extra_wires wire);
static int pcice(struct fpga_model *model);
static int clkpll(struct fpga_model *model);
static int ckpin(struct fpga_model *model);
static int clkindirect_feedback(struct fpga_model *model, enum extra_wires wire);
static int run_gclk(struct fpga_model *model);
static int run_gclk_horiz_regs(struct fpga_model *model);
static int run_gclk_vert_regs(struct fpga_model *model);
static int run_logic_inout(struct fpga_model *model);
static int run_io_wires(struct fpga_model *model);
static int run_gfan(struct fpga_model *model);
static int connect_clk_sr(struct fpga_model *model, const char *clk_sr);
static int connect_logic_carry(struct fpga_model *model);
static int run_dirwires(struct fpga_model *model);

int init_conns(struct fpga_model *model)
{
	int rc;

	if ((rc = term_topbot(model))) FAIL(rc);
	if ((rc = term_leftright(model))) FAIL(rc);

	if ((rc = term_to_io(model, IOCE))) FAIL(rc);
	if ((rc = term_to_io(model, IOCLK))) FAIL(rc);
	if ((rc = term_to_io(model, PLLCE))) FAIL(rc);
	if ((rc = term_to_io(model, PLLCLK))) FAIL(rc);

	if ((rc = clkpll(model))) FAIL(rc);
	if ((rc = ckpin(model))) FAIL(rc);
	if ((rc = clkindirect_feedback(model, CLK_INDIRECT))) FAIL(rc);
	if ((rc = clkindirect_feedback(model, CLK_FEEDBACK))) FAIL(rc);

	if ((rc = pcice(model))) FAIL(rc);
	if ((rc = run_gclk(model))) FAIL(rc);
	if ((rc = run_gclk_horiz_regs(model))) FAIL(rc);
	if ((rc = run_gclk_vert_regs(model))) FAIL(rc);

	if ((rc = connect_logic_carry(model))) FAIL(rc);
	if ((rc = connect_clk_sr(model, "CLK"))) FAIL(rc);
	if ((rc = connect_clk_sr(model, "SR"))) FAIL(rc);
	if ((rc = run_gfan(model))) FAIL(rc);
	if ((rc = run_io_wires(model))) FAIL(rc);
	if ((rc = run_logic_inout(model))) FAIL(rc);
	if ((rc = run_dirwires(model))) FAIL(rc);
	return 0;
fail:
	return rc;
}

static int term_topbot(struct fpga_model* model)
{
	struct w_net net;
	int x, y, i, rc;

	//
	// wires going from the top and bottom term tiles vertically to
	// support the two ILOGIC/OLOGIC/IODELAY tiles below or above the
	// term tile.
	//
	for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {

		//
		// top
		//

		y = TOP_INNER_ROW; 

		if (has_device(model, y+1, x, DEV_ILOGIC)) {
			{struct w_net n = {
				.last_inc = 3, .num_pts = 3, .pt =
				{{ "TTERM_CLB_IOCE%i_S",  0,   y,   x },
				 { "TIOI_IOCE%i",	  0, y+1,   x },
				 { "TIOI_INNER_IOCE%i",	  0, y+2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
			{struct w_net n = {
				.last_inc = 3, .num_pts = 3, .pt =
				{{ "TTERM_CLB_IOCLK%i_S",  0,   y,   x },
				 { "TIOI_IOCLK%i",	   0, y+1,   x },
				 { "TIOI_INNER_IOCLK%i",   0, y+2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
			{struct w_net n = {
				.last_inc = 0, .num_pts = 3, .pt =
				{{ "TTERM_CLB_PCICE_S",    0,   y,   x },
				 { "IOI_PCI_CE",	   0, y+1,   x },
				 { "IOI_PCI_CE",	   0, y+2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
			{struct w_net n = {
				.last_inc = 1, .num_pts = 3, .pt =
				{{ "TTERM_CLB_PLLCE%i_S",     0,   y,   x },
				 { "TIOI_PLLCE%i",	      0, y+1,   x },
				 { "TIOI_INNER_PLLCE%i",      0, y+2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
			{struct w_net n = {
				.last_inc = 1, .num_pts = 3, .pt =
				{{ "TTERM_CLB_PLLCLK%i_S",    0,   y,   x },
				 { "TIOI_PLLCLK%i",	      0, y+1,   x },
				 { "TIOI_INNER_PLLCLK%i",     0, y+2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
		}

		//
		// bottom
		//

		y = model->y_height - BOT_INNER_ROW;

		// wiring up of IOLOGIC devices:
		if (has_device(model, y-1, x, DEV_ILOGIC)) {

			// IOCE
			{struct w_net n = {
				.last_inc = 3, .num_pts = 3, .pt =
				{{ "BTERM_CLB_CEOUT%i_N", 0,   y,   x },
				 { "TIOI_IOCE%i",	  0, y-1,   x },
				 { "BIOI_INNER_IOCE%i",	  0, y-2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }

			// IOCLK
			{struct w_net n = {
				.last_inc = 3, .num_pts = 3, .pt =
				{{ "BTERM_CLB_CLKOUT%i_N", 0,   y,   x },
				 { "TIOI_IOCLK%i",	   0, y-1,   x },
				 { "BIOI_INNER_IOCLK%i",   0, y-2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }

			// PCI_CE
			{struct w_net n = {
				.last_inc = 0, .num_pts = 3, .pt =
				{{ "BTERM_CLB_PCICE_N",    0,   y,   x },
				 { "IOI_PCI_CE",	   0, y-1,   x },
				 { "IOI_PCI_CE",	   0, y-2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }

			// PLLCE
			{struct w_net n = {
				.last_inc = 1, .num_pts = 3, .pt =
				{{ "BTERM_CLB_PLLCEOUT%i_N",  0,   y,   x },
				 { "TIOI_PLLCE%i",	      0, y-1,   x },
				 { "BIOI_INNER_PLLCE%i",      0, y-2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }

			// PLLCLK
			{struct w_net n = {
				.last_inc = 1, .num_pts = 3, .pt =
				{{ "BTERM_CLB_PLLCLKOUT%i_N", 0,   y,   x },
				 { "TIOI_PLLCLK%i",	      0, y-1,   x },
				 { "BIOI_INNER_PLLCLK%i",     0, y-2,   x }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }
		}
	}

	//
	// Horizontally connect the top and bottom center of the chip
	// to the left and right side termination tiles. IOCE and IOCLK
	// have two separate nets on the left and right side, PLLCE
	// and PLLCLK have one net covering the entire chip from
	// left to right.
	//

	//
	// top
	//
	{
		// strings are filled in below - must match offsets
		struct seed_data seeds[] = {
			/* 0 */ { X_FABRIC_LOGIC_ROUTING_COL | X_CENTER_ROUTING_COL },
			/* 1 */ { X_FABRIC_LOGIC_COL | X_CENTER_LOGIC_COL },
			/* 2 */	{ X_FABRIC_BRAM_ROUTING_COL | X_FABRIC_BRAM_COL },
			/* 3 */ { X_FABRIC_BRAM_VIA_COL },
			/* 4 */	{ X_FABRIC_MACC_ROUTING_COL | X_FABRIC_MACC_COL },
			/* 5 */	{ X_FABRIC_MACC_VIA_COL },
			/* 6 */ { X_CENTER_CMTPLL_COL },
			/* 7 */ { X_CENTER_REGS_COL },
			{ 0 }};

		y = TOP_INNER_ROW;
		for (i = 0; i < 4; i++) {
			if (i == 0) {		// 0 = IOCE
				seeds[0].str = "IOI_TTERM_IOCE%i";
				seeds[1].str = "TTERM_CLB_IOCE%i";
				seeds[2].str = "BRAM_TTERM_IOCE%i";
				seeds[3].str = "BRAM_INTER_TTERM_IOCE%i";
				seeds[4].str = "DSP_TTERM_IOCE%i";
				seeds[5].str = "DSP_INTER_TTERM_IOCE%i";
				seeds[6].str = "REGT_TTERM_IOCEOUT%i";
				seeds[7].str = "REGV_TTERM_IOCEOUT%i";
				net.last_inc = 3;
				seed_strx(model, seeds);
				for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
					// CEOUT to center
					if (x == model->center_x-CENTER_CMTPLL_O) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCEOUT%i", 0);
						if (rc) FAIL(rc);
					} else if (x == model->center_x) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 4, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCEOUT%i", 4);
						if (rc) FAIL(rc);
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCEOUT%i", x < model->center_x ? 0 : 4);
						if (rc) FAIL(rc);
					}
				}
			} else if (i == 1) {	// 1 = IOCLK
				seeds[0].str = "IOI_TTERM_IOCLK%i";
				seeds[1].str = "TTERM_CLB_IOCLK%i";
				seeds[2].str = "BRAM_TTERM_IOCLK%i";
				seeds[3].str = "BRAM_INTER_TTERM_IOCLK%i";
				seeds[4].str = "DSP_TTERM_IOCLK%i";
				seeds[5].str = "DSP_INTER_TTERM_IOCLK%i";
				seeds[6].str = "REGT_TTERM_IOCLKOUT%i";
				seeds[7].str = "REGV_TTERM_IOCLKOUT%i";
				net.last_inc = 3;
				seed_strx(model, seeds);
				for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
					// CLKOUT to center
					if (x == model->center_x-CENTER_CMTPLL_O) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCLKOUT%i", 0);
						if (rc) FAIL(rc);
					} else if (x == model->center_x) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 4, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCLKOUT%i", 4);
						if (rc) FAIL(rc);
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCLKOUT%i", x < model->center_x ? 0 : 4);
						if (rc) FAIL(rc);
					}
				}
			} else if (i == 2) {	// 2 = PLLCE
				seeds[0].str = "IOI_TTERM_PLLCE%i";
				seeds[1].str = "TTERM_CLB_PLLCE%i";
				seeds[2].str = "BRAM_TTERM_PLLCE%i";
				seeds[3].str = "BRAM_INTER_TTERM_PLLCE%i";
				seeds[4].str = "DSP_TTERM_PLLCE%i";
				seeds[5].str = "DSP_INTER_TTERM_PLLCE%i";
				seeds[6].str = "REGT_TTERM_PLL_CEOUT%i";
				seeds[7].str = "REGV_TTERM_CEOUT%i";
				net.last_inc = 1;
				seed_strx(model, seeds);
			} else {		// 3 = PLLCLK
				seeds[0].str = "IOI_TTERM_PLLCLK%i";
				seeds[1].str = "TTERM_CLB_PLLCLK%i";
				seeds[2].str = "BRAM_TTERM_PLLCLK%i";
				seeds[3].str = "BRAM_INTER_TTERM_PLLCLK%i";
				seeds[4].str = "DSP_TTERM_PLLCLK%i";
				seeds[5].str = "DSP_INTER_TTERM_PLLCLK%i";
				seeds[6].str = "REGT_TTERM_PLL_CLKOUT%i";
				seeds[7].str = "REGV_TTERM_CLKOUT%i";
				net.last_inc = 1;
				seed_strx(model, seeds);
			}

			net.num_pts = 0;
			// The leftmost and rightmost columns of the fabric area are exempt.
			for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
				EXIT(net.num_pts >= sizeof(net.pt)/sizeof(net.pt[0]));
				EXIT(!model->tmp_str[x]);
	
				// left and right half separate only for CEOUT and CLKOUT
				if (i < 2 && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					// connect left side
					// left side connects to 0:3, right side to 4:7
					net.pt[net.num_pts].start_count = 0;
					net.pt[net.num_pts].x = x;
					net.pt[net.num_pts].y = TOP_INNER_ROW;
					net.pt[net.num_pts].name = model->tmp_str[x];
					net.num_pts++;
					if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
					// start right half
					net.pt[0].start_count = 4;
					net.pt[0].x = x;
					net.pt[0].y = TOP_INNER_ROW;
					net.pt[0].name = model->tmp_str[x];
					x++;
					net.pt[1].start_count = 4;
					net.pt[1].x = x;
					net.pt[1].y = TOP_INNER_ROW;
					net.pt[1].name = model->tmp_str[x];
					net.num_pts = 2;
				} else {
					net.pt[net.num_pts].start_count = 0;
					net.pt[net.num_pts].x = x;
					net.pt[net.num_pts].y = TOP_INNER_ROW;
					net.pt[net.num_pts].name = model->tmp_str[x];
					net.num_pts++;
				}
			}
			// connect all (PLL) or just right side
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
		}
	}
	//
	// bottom
	//
	{
		// strings are filled in below - must match offsets
		struct seed_data seeds[] = {
			/* 0 */ { X_FABRIC_ROUTING_COL | X_FABRIC_MACC_COL
			 	  | X_FABRIC_BRAM_COL | X_CENTER_ROUTING_COL },
			/* 1 */ { X_FABRIC_LOGIC_COL | X_FABRIC_BRAM_VIA_COL
				  | X_FABRIC_MACC_VIA_COL | X_CENTER_LOGIC_COL },
			/* 2 */ { X_CENTER_CMTPLL_COL },
			/* 3 */ { X_CENTER_REGS_COL },
			{ 0 }};

		y = model->y_height - BOT_INNER_ROW;
		for (i = 0; i < 4; i++) {
			if (i == 0) {		// 0 = CEOUT
				seeds[0].str = "IOI_BTERM_CEOUT%i";
				seeds[1].str = "BTERM_CLB_CEOUT%i";
				seeds[2].str = "REGB_BTERM_IOCEOUT%i";
				seeds[3].str = "REGV_BTERM_CEOUT%i";
				net.last_inc = 3;
				seed_strx(model, seeds);
				for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
					// CEOUT to center
					if (x == model->center_x-CENTER_CMTPLL_O) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 7,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCEOUT%i", 0);
						if (rc) FAIL(rc);
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCEOUT%i", x < model->center_x ? 4 : 0);
						if (rc) FAIL(rc);
					}
				}
			} else if (i == 1) {	// 1 = CLKOUT
				seeds[0].str = "IOI_BTERM_CLKOUT%i";
				seeds[1].str = "BTERM_CLB_CLKOUT%i";
				seeds[2].str = "REGB_BTERM_IOCLKOUT%i";
				seeds[3].str = "REGV_BTERM_CLKOUT%i";
				net.last_inc = 3;
				seed_strx(model, seeds);
				for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
					// CECLK to center
					if (x == model->center_x-CENTER_CMTPLL_O) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 7,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCLKOUT%i", 0);
						if (rc) FAIL(rc);
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCLKOUT%i", x < model->center_x ? 4 : 0);
						if (rc) FAIL(rc);
					}
				}
			} else if (i == 2) {	// 2 = PLLCEOUT
				seeds[0].str = "IOI_BTERM_PLLCEOUT%i";
				seeds[1].str = "BTERM_CLB_PLLCEOUT%i";
				seeds[2].str = "REGB_BTERM_PLL_CEOUT%i";
				seeds[3].str = "REGV_BTERM_PLLCEOUT%i";
				net.last_inc = 1;
				seed_strx(model, seeds);
			} else {		// 3 = PLLCLKOUT
				seeds[0].str = "IOI_BTERM_PLLCLKOUT%i";
				seeds[1].str = "BTERM_CLB_PLLCLKOUT%i";
				seeds[2].str = "REGB_BTERM_PLL_CLKOUT%i";
				seeds[3].str = "REGV_BTERM_PLLCLKOUT%i";
				net.last_inc = 1;
				seed_strx(model, seeds);
			}

			net.num_pts = 0;
			// The leftmost and rightmost columns of the fabric area are exempt.
			for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
				EXIT(net.num_pts >= sizeof(net.pt)/sizeof(net.pt[0]));
				EXIT(!model->tmp_str[x]);
	
				// left and right half separate only for CEOUT and CLKOUT
				if (i < 2 && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					// connect left side
					// left side connects to 4:7, right side to 0:3
					net.pt[net.num_pts].start_count = 4;
					net.pt[net.num_pts].x = x;
					net.pt[net.num_pts].y = model->y_height - BOT_INNER_ROW;
					net.pt[net.num_pts].name = model->tmp_str[x];
					net.num_pts++;
					if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
					// start right half
					net.num_pts = 0;
				}
	
				net.pt[net.num_pts].start_count = 0;
				net.pt[net.num_pts].x = x;
				net.pt[net.num_pts].y = model->y_height - BOT_INNER_ROW;
				net.pt[net.num_pts].name = model->tmp_str[x];
				net.num_pts++;
			}
			// connect all (PLL) or just right side
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int term_leftright_x(struct fpga_model *model, int x)
{
	struct w_net net;
	int y, i, rc;
	const char *tmp_str;
	// strings are filled in below - must match offsets
	struct seed_data seeds[] = {
		/* 0 */ { Y_REGULAR_ROW },
		/* 1 */ { Y_ROW_HORIZ_AXSYMM },
		/* 2 */ { Y_CHIP_HORIZ_REGS },
		{ 0 }};

	//
	// Similar to term_topbot(), there are vertical nets connecting
	// IOCE/IOCLK and PLLCE/PLLCLK through the left and right term
	// tiles. The IO nets are covering the entire chip top-to-bottom,
	// the PLL nets are split between top and bottom net.
	// The upmost and bottommost half-row are not included in the nets.
	//

	for (i = 0; i < 4; i++) { // IOCE (0), IOCLK (1), PLLCE (2), PLLCLK (3)
		// 1. seeding and single connections from inner_col
		//    to outer_col center (IOCE and IOCLK only).
		if (i == 0) {		// 0 = IOCE
			if (x == LEFT_INNER_COL) {
				seeds[0].str = "IOI_LTERM_IOCE%i";
				seeds[1].str = "HCLK_IOI_LTERM_IOCE%i";
				seeds[2].str = "REGH_LTERM_IOCEOUT%i";
				tmp_str = "REGL_IOCEOUT%i";
			} else if (x == model->x_width - RIGHT_INNER_O) {
				seeds[0].str = "IOI_RTERM_IOCE%i";
				seeds[1].str = "HCLK_IOI_RTERM_IOCE%i";
				seeds[2].str = "REGH_RTERM_IOCEOUT%i";
				tmp_str = "REGR_IOCEOUT%i";
			} else FAIL(EINVAL);
			net.last_inc = 3;
			seed_stry(model, seeds);
			for (y = TOP_FIRST_REGULAR + HALF_ROW; y <= model->y_height
					- BOT_LAST_REGULAR_O - HALF_ROW; y++) {
				rc = add_conn_range(model, NOPREF_BI_F, y, x,
					model->tmp_str[y], 0, (y == model->center_y) ? 7 : 3,
					model->center_y,
					(x == LEFT_INNER_COL) ? LEFT_OUTER_COL : model->x_width-RIGHT_OUTER_O,
					tmp_str, (y == model->center_y) ? 0
					  : (((y < model->center_y) ^ (x != LEFT_INNER_COL)) ? 4 : 0));
				if (rc) FAIL(rc);
			}
		} else if (i == 1) {	// 1 = IOCLK
			if (x == LEFT_INNER_COL) {
				seeds[0].str = "IOI_LTERM_IOCLK%i";
				seeds[1].str = "HCLK_IOI_LTERM_IOCLK%i";
				seeds[2].str = "REGH_LTERM_IOCLKOUT%i";
				tmp_str = "REGL_IOCLKOUT%i";
			} else if (x == model->x_width - RIGHT_INNER_O) {
				seeds[0].str = "IOI_RTERM_IOCLK%i";
				seeds[1].str = "HCLK_IOI_RTERM_IOCLK%i";
				seeds[2].str = "REGH_RTERM_IOCLKOUT%i";
				tmp_str = "REGR_IOCLKOUT%i";
			} else FAIL(EINVAL);
			net.last_inc = 3;
			seed_stry(model, seeds);
			for (y = TOP_FIRST_REGULAR + HALF_ROW; y <= model->y_height
					- BOT_LAST_REGULAR_O - HALF_ROW; y++) {
				rc = add_conn_range(model, NOPREF_BI_F, y, x,
					model->tmp_str[y], 0, (y == model->center_y) ? 7 : 3,
					model->center_y,
					(x == LEFT_INNER_COL) ? LEFT_OUTER_COL : model->x_width-RIGHT_OUTER_O,
					tmp_str, (y == model->center_y) ? 0
					  : (((y < model->center_y) ^ (x != LEFT_INNER_COL)) ? 4 : 0));
				if (rc) FAIL(rc);
			}
		} else if (i == 2) {	// 2 = PLLCE
			if (x == LEFT_INNER_COL) {
				seeds[0].str = "IOI_LTERM_PLLCE%i";
				seeds[1].str = "HCLK_IOI_LTERM_PLLCE%i";
				seeds[2].str = "REGH_LTERM_PLL_CEOUT%i";
			} else if (x == model->x_width - RIGHT_INNER_O) {
				seeds[0].str = "IOI_RTERM_PLLCEOUT%i";
				seeds[1].str = "HCLK_IOI_RTERM_PLLCEOUT%i";
				seeds[2].str = "REGH_RTERM_PLL_CEOUT%i";
			} else FAIL(EINVAL);
			net.last_inc = 1;
			seed_stry(model, seeds);
		} else {		// 3 = PLLCLK
			if (x == LEFT_INNER_COL) {
				seeds[0].str = "IOI_LTERM_PLLCLK%i";
				seeds[1].str = "HCLK_IOI_LTERM_PLLCLK%i";
				seeds[2].str = "REGH_LTERM_PLL_CLKOUT%i";
			} else if (x == model->x_width - RIGHT_INNER_O) {
				seeds[0].str = "IOI_RTERM_PLLCLKOUT%i";
				seeds[1].str = "HCLK_IOI_RTERM_PLLCLKOUT%i";
				seeds[2].str = "REGH_RTERM_PLL_CLKOUT%i";
			} else FAIL(EINVAL);
			net.last_inc = 1;
			seed_stry(model, seeds);
		}

		// 2. vertical net inside inner_col
		net.num_pts = 0;
		for (y = TOP_FIRST_REGULAR + HALF_ROW; y <= model->y_height
				- BOT_LAST_REGULAR_O - HALF_ROW; y++) {
			ASSERT(net.num_pts < sizeof(net.pt)/sizeof(net.pt[0])
					&& model->tmp_str[y]);

			// top and bottom half separate for IOCE and IOCLK
			if (i < 2 && is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				// connect top half:
				// left top to 4:7, left bottom to 0:3
				// right top to 0:3, right bottom to 4:7
				net.pt[net.num_pts].start_count =
					(x == LEFT_INNER_COL) ? 4 : 0;
				net.pt[net.num_pts].x = x;
				net.pt[net.num_pts].y = y;
				net.pt[net.num_pts].name = model->tmp_str[y];
				net.num_pts++;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
				// start bottom
				net.pt[0].start_count =
					(x == LEFT_INNER_COL) ? 0 : 4;
				net.pt[0].x = x;
				net.pt[0].y = y;
				net.pt[0].name = model->tmp_str[y];
				net.num_pts = 1;
				continue;
			}
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].x = x;
			net.pt[net.num_pts].y = y;
			net.pt[net.num_pts].name = model->tmp_str[y];
			net.num_pts++;
		}
		// connect all (PLL) or just top/bottom (IO)
		if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int term_leftright(struct fpga_model *model)
{
	int rc;

	rc = term_leftright_x(model, LEFT_INNER_COL);
	if (rc) FAIL(rc);
	rc = term_leftright_x(model, model->x_width - RIGHT_INNER_O);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

static int net_up_down(struct fpga_model *model, int hclk_y, int up_down_x,
	enum extra_wires wire, struct w_net *up_net, struct w_net *down_net)
{
	const char *s1;
	int last_inc, i, rc;

	if (wire == IOCE) {
		s1 = "IOCE";
		last_inc = 3;
	} else if (wire == IOCLK) {
		s1 = "IOCLK";
		last_inc = 3;
	} else if (wire == PLLCE) {
		s1 = "PLLCE";
		last_inc = 1;
	} else if (wire == PLLCLK) {
		s1 = "PLLCLK";
		last_inc = 1;
	} else FAIL(EINVAL);

	up_net->last_inc = last_inc;
	up_net->pt[0].name = pf("HCLK_IOIL_%s%%i_UP", s1);
	up_net->pt[0].start_count = 0;
	up_net->pt[0].y = hclk_y;
	up_net->pt[0].x = up_down_x;
	up_net->num_pts = 1;

	down_net->last_inc = last_inc;
	down_net->pt[0].name = pf("HCLK_IOIL_%s%%i_DOWN", s1);
	down_net->pt[0].start_count = 0;
	down_net->pt[0].y = hclk_y;
	down_net->pt[0].x = up_down_x;
	down_net->num_pts = 1;

	for (i = 0; i < HALF_ROW; i++) {
		if (has_device(model, hclk_y-1-i, up_down_x, DEV_IODELAY)) {
			while (up_net->pt[up_net->num_pts-1].y > hclk_y-1-i+1) {
				up_net->pt[up_net->num_pts].name = pf("INT_INTERFACE_%s%%i", s1);
				up_net->pt[up_net->num_pts].start_count = 0;
				up_net->pt[up_net->num_pts].y = up_net->pt[up_net->num_pts-1].y-1;
				up_net->pt[up_net->num_pts].x = up_down_x;
				up_net->num_pts++;
			}
			up_net->pt[up_net->num_pts].name = pf("%s_%s%%i",
				up_down_x < model->center_x ? "IOI" : "RIOI", s1);
			up_net->pt[up_net->num_pts].start_count = 0;
			up_net->pt[up_net->num_pts].y = hclk_y-1-i;
			up_net->pt[up_net->num_pts].x = up_down_x;
			up_net->num_pts++;
		}
		if (has_device(model, hclk_y+1+i, up_down_x, DEV_IODELAY)) {
			while (down_net->pt[down_net->num_pts-1].y < hclk_y+1+i-1) {
				down_net->pt[down_net->num_pts].name = pf("INT_INTERFACE_%s%%i", s1);
				down_net->pt[down_net->num_pts].start_count = 0;
				down_net->pt[down_net->num_pts].y = down_net->pt[down_net->num_pts-1].y+1;
				down_net->pt[down_net->num_pts].x = up_down_x;
				down_net->num_pts++;
			}
			down_net->pt[down_net->num_pts].name = pf("%s_%s%%i",
				up_down_x < model->center_x ? "IOI" : "RIOI", s1);
			down_net->pt[down_net->num_pts].start_count = 0;
			down_net->pt[down_net->num_pts].y = hclk_y+1+i;
			down_net->pt[down_net->num_pts].x = up_down_x;
			down_net->num_pts++;
		}
	}
	return 0;
fail:
	return rc;
}

static int term_to_io(struct fpga_model *model, enum extra_wires wire)
{
	struct w_net up_net, down_net, net;
	const char *s1;
	int last_inc, y, i, rc;

	// from termination into fabric via hclk
	if (wire == IOCE) {
		s1 = "IOCE";
		last_inc = 3;
	} else if (wire == IOCLK) {
		s1 = "IOCLK";
		last_inc = 3;
	} else if (wire == PLLCE) {
		s1 = "PLLCE";
		last_inc = 1;
	} else if (wire == PLLCLK) {
		s1 = "PLLCLK";
		last_inc = 1;
	} else FAIL(EINVAL);

	//
	// left
	//

	for (y = TOP_FIRST_REGULAR; y <= model->y_height - BOT_LAST_REGULAR_O; y++) {
		if (!is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
			continue;
		{ struct w_net n = {
			.last_inc = last_inc, .num_pts = 3, .pt =
			{{ pf("HCLK_IOI_LTERM_%s%%i_E", s1), 0, y,   LEFT_INNER_COL },
			 { pf("HCLK_IOI_INT_%s%%i", s1),     0, y,   LEFT_IO_ROUTING },
			 { pf("HCLK_IOIL_%s%%i", s1),        0, y,   LEFT_IO_DEVS }}};

		if (wire == PLLCLK
		    && model->xci->mcb_ypos >= y-HALF_ROW
		    && model->xci->mcb_ypos <= y+HALF_ROW ) {
			n.pt[3].name = "HCLK_MCB_PLLCLKOUT%i_W";
			n.pt[3].start_count = 0;
			n.pt[3].y = y;
			n.pt[3].x = LEFT_MCB_COL;
			n.num_pts = 4;
			rc = add_conn_bi(model,
				y, LEFT_MCB_COL, "MCB_HCLK_PLLCLKO_PINW",
				model->xci->mcb_ypos, LEFT_MCB_COL, "MCB_L_PLLCLK0_PW");
			if (rc) FAIL(rc);
			rc = add_conn_bi(model,
				y, LEFT_MCB_COL, "MCB_HCLK_PLLCLK1_PINW",
				model->xci->mcb_ypos, LEFT_MCB_COL, "MCB_L_PLLCLK1_PW");
			if (rc) FAIL(rc);
		} else if (wire == PLLCE
		    && model->xci->mcb_ypos >= y-HALF_ROW
		    && model->xci->mcb_ypos <= y+HALF_ROW ) {
			n.pt[3].name = "HCLK_MCB_PLLCEOUT%i_W";
			n.pt[3].start_count = 0;
			n.pt[3].y = y;
			n.pt[3].x = LEFT_MCB_COL;
			n.num_pts = 4;
			rc = add_conn_range(model, NOPREF_BI_F,
				y, LEFT_MCB_COL, "MCB_HCLK_PLLCE%i_PINW", 0, 1,
				model->xci->mcb_ypos, LEFT_MCB_COL, "MCB_L_PLLCE%i_PW", 0);
			if (rc) FAIL(rc);
		}
		if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc); }

		// wire up and down the row (8 each) to reach all io devices
		if ((rc = net_up_down(model, y, LEFT_IO_DEVS,
			wire, &up_net, &down_net))) FAIL(rc);
		if (up_net.num_pts > 1
		    && (rc = add_conn_net(model, NOPREF_BI_F, &up_net))) FAIL(rc);
		if (down_net.num_pts > 1
		    && (rc = add_conn_net(model, NOPREF_BI_F, &down_net))) FAIL(rc);
	}

	//
	// right
	//

	for (y = TOP_FIRST_REGULAR; y <= model->y_height - BOT_LAST_REGULAR_O; y++) {
		if (!is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
			continue;

		if (wire == IOCE || wire == IOCLK) {
			net.last_inc = 0;
			net.num_pts = 3;
			for (i = 0; i <= 3; i++) {
				net.pt[0].name = pf("HCLK_IOI_RTERM_%s%i_W", s1, i);
				net.pt[0].start_count = 0;
				net.pt[0].y = y;
				net.pt[0].x = model->x_width - RIGHT_INNER_O;
				net.pt[1].name = pf("HCLK_MCB_%s%i_W", s1, i);
				net.pt[1].start_count = 0;
				net.pt[1].y = y;
				net.pt[1].x = model->x_width - RIGHT_MCB_O;
				net.pt[2].name = pf("HCLK_IOIL_%s%i", s1, 3-i);
				net.pt[2].start_count = 0;
				net.pt[2].y = y;
				net.pt[2].x = model->x_width - RIGHT_IO_DEVS_O;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			}
		} else { // PLLCE/PLLCLK
			struct w_net n = {
				.last_inc = last_inc, .num_pts = 3, .pt =
				{{ wire == PLLCE ? "HCLK_IOI_RTERM_PLLCEOUT%i_W"
					: "HCLK_IOI_RTERM_PLLCLKOUT%i_W",
					0, y, model->x_width - RIGHT_INNER_O },
				 { wire == PLLCE ? "HCLK_MCB_PLLCEOUT%i_W"
					: "HCLK_MCB_PLLCLKOUT%i_W",
					0, y, model->x_width - RIGHT_MCB_O },
				 { pf("HCLK_IOIL_%s%%i", s1), 0, y, model->x_width - RIGHT_IO_DEVS_O }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc);
		}

		// wire up and down the row (8 each) to reach all io devices
		if ((rc = net_up_down(model, y, model->x_width - RIGHT_IO_DEVS_O,
			wire, &up_net, &down_net))) FAIL(rc);
		if (up_net.num_pts > 1
		    && (rc = add_conn_net(model, NOPREF_BI_F, &up_net))) FAIL(rc);
		if (down_net.num_pts > 1
		    && (rc = add_conn_net(model, NOPREF_BI_F, &down_net))) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int pcice_conn(struct fpga_model *model, int y, int x, int i);

static int pcice(struct fpga_model *model)
{
	struct w_net net;
	int x, y, i, rightmost_local_net, rc;

	//
	// PCICE east-west wiring
	//
	
	// First find BRAM or MACC device columns which are the focal
	// points of the east-west PCICE wiring.
	rightmost_local_net = -1;
	y = model->y_height - BOT_INNER_ROW;
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL, model, x)) {
			// From the BRAM/MACC focal points, search left and right
			// for local east-west hubs
			net.last_inc = 0;
			net.num_pts = 0;
			for (i = x-1; i >= LEFT_SIDE_WIDTH; i--) {
				if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL|X_CENTER_REGS_COL, model, i))
					break;
				// center logic cannot be found when going left.
				if ((is_atx(X_FABRIC_LOGIC_COL, model, i)
				     && !is_atx(X_ROUTING_NO_IO, model, i-1))
				    || is_atx(X_FABRIC_MACC_VIA_COL, model, i)) {
					rc = pcice_conn(model, y, x, i);
					if (rc) FAIL(rc);
					net.pt[net.num_pts].start_count = 0;
					net.pt[net.num_pts].x = i;
					net.pt[net.num_pts].y = y;
					net.pt[net.num_pts].name = "BTERM_CLB_PCICE";
					net.num_pts++;
				}
			}
			// TODO: there are more sub-cases here: all points in the
			//       subnet must be connected to the x coords that are
			//       not in the net... and some more cases on left and right
			//       side...
			if (net.num_pts && net.pt[0].x > rightmost_local_net) {
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			}

			net.last_inc = 0;
			net.num_pts = 0;
			for (i = x+1; i < model->x_width - RIGHT_SIDE_WIDTH; i++) {
				if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL|X_CENTER_CMTPLL_COL, model, i))
					break;
				if ((is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, i)
				     && !is_atx(X_ROUTING_NO_IO, model, i-1))
				    || is_atx(X_FABRIC_MACC_VIA_COL, model, i)) {
					rc = pcice_conn(model, y, x, i);
					if (rc) FAIL(rc);
					net.pt[net.num_pts].start_count = 0;
					net.pt[net.num_pts].x = i;
					net.pt[net.num_pts].y = y;
					net.pt[net.num_pts].name = "BTERM_CLB_PCICE";
					net.num_pts++;
				}
			}
			if (net.num_pts) {
				rightmost_local_net = net.pt[net.num_pts-1].x;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int pcice_conn(struct fpga_model *model, int y, int x, int i)
{
	static const char* src_str;
	int to_center, rc;

	to_center = (i < x) ^ (x < model->center_x);
	if (is_atx(X_FABRIC_BRAM_COL, model, x))
		src_str = to_center ?
			"BRAM_TTERM_PCICE_OUT" : "BRAM_TTERM_PCICE_IN";
	else if (is_atx(X_FABRIC_MACC_COL, model, x))
		src_str = to_center ?
			"MACCSITE2_TTERM_PCICE_OUT" : "MACCSITE2_TTERM_PCICE_IN";
	else
		FAIL(EINVAL);
	return add_conn_bi(model, y, x, src_str, y, i, "BTERM_CLB_PCICE");
fail:
	return rc;
}

//
// clkpll() ckpin() and clkindirect_feedback() run a set of wire strings
// horizontally through the entire chip from left to right over the center
// reg row. The wires meet at the middle of each half of the chip on
// the left and right side, as well as in the center.
//

static int clkpll(struct fpga_model *model)
{
	int x, rc;
	struct w_net net;

	//
	// clk pll 0:1
	//

	// two nets - one for left side, one for right side
	{ struct seed_data seeds[] = {
		{ X_OUTER_LEFT, 		"REGL_CLKPLL%i" },
		{ X_INNER_LEFT, 		"REGL_LTERM_CLKPLL%i" },
		{ X_LEFT_MCB | X_RIGHT_MCB,	"MCB_CLKPLL%i" },
		{ X_FABRIC_ROUTING_COL
		  | X_LEFT_IO_ROUTING_COL
		  | X_RIGHT_IO_ROUTING_COL
		  | X_FABRIC_BRAM_COL,	 	"INT_CLKPLL%i" },
		{ X_LEFT_IO_DEVS_COL
		  | X_FABRIC_BRAM_VIA_COL
		  | X_FABRIC_MACC_VIA_COL
		  | X_FABRIC_LOGIC_COL
		  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL%i" },
		{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL%i" },
		{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_IO_RT%i" },
		{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKPLL_IO_LT%i" },
		{ X_CENTER_CMTPLL_COL, 		"REGC_CLKPLL_IO_LT%i" },
		{ X_CENTER_REGS_COL, 		"CLKC_PLL_IO_RT%i" },
		{ X_INNER_RIGHT, 		"REGR_RTERM_CLKPLL%i" },
		{ X_OUTER_RIGHT, 		"REGR_CLKPLL%i" },
		{ 0 }};
	seed_strx(model, seeds);
	net.last_inc = 1;
	net.num_pts = 0;
	for (x = 0; x < model->x_width; x++) {
		ASSERT(model->tmp_str[x]);
		ASSERT(net.num_pts < sizeof(net.pt)/sizeof(net.pt[0]));

		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].x = x;
		net.pt[net.num_pts].y = model->center_y;
		net.pt[net.num_pts].name = model->tmp_str[x];
		net.num_pts++;

		if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
			// left-side net
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

			// start right side
			net.pt[0].start_count = 0;
			net.pt[0].x = x;
			net.pt[0].y = model->center_y;
			net.pt[0].name = "REGC_CLKPLL_IO_RT%i";
			net.num_pts = 1;
		}
	}
	// right-side net
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }

	//
	// clk pll lock 0:1
	//

	// two nets - one for left side, one for right side
	{ struct seed_data seeds[] = {
		{ X_OUTER_LEFT, 		"REGL_LOCKED%i" },
		{ X_INNER_LEFT, 		"REGH_LTERM_LOCKED%i" },
		{ X_LEFT_MCB | X_RIGHT_MCB,	"MCB_CLKPLL_LOCK%i" },
		{ X_FABRIC_ROUTING_COL
		  | X_LEFT_IO_ROUTING_COL
		  | X_RIGHT_IO_ROUTING_COL
		  | X_FABRIC_BRAM_COL,	 	"INT_CLKPLL_LOCK%i" },
		{ X_LEFT_IO_DEVS_COL
		  | X_FABRIC_BRAM_VIA_COL
		  | X_FABRIC_MACC_VIA_COL
		  | X_FABRIC_LOGIC_COL
		  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL_LOCK%i" },
		{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL_LOCK%i" },
		{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_LOCK_RT%i" },
		{ X_CENTER_LOGIC_COL,	 	"REGC_CLECLKPLL_LOCK_LT%i" },
		{ X_CENTER_CMTPLL_COL, 		"CLK_PLL_LOCK_LT%i" },
		{ X_CENTER_REGS_COL, 		"CLKC_PLL_LOCK_RT%i" },
		{ X_INNER_RIGHT, 		"REGH_RTERM_LOCKED%i" },
		{ X_OUTER_RIGHT, 		"REGR_LOCKED%i" },
		{ 0 }};
	seed_strx(model, seeds);
	net.last_inc = 1;
	net.num_pts = 0;
	for (x = 0; x < model->x_width; x++) {
		ASSERT(model->tmp_str[x]);
		ASSERT(net.num_pts < sizeof(net.pt)/sizeof(net.pt[0]));

		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].x = x;
		net.pt[net.num_pts].y = model->center_y;
		net.pt[net.num_pts].name = model->tmp_str[x];
		net.num_pts++;

		if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
			// left-side net
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

			// start right side
			net.pt[0].start_count = 0;
			net.pt[0].x = x;
			net.pt[0].y = model->center_y;
			net.pt[0].name = "CLK_PLL_LOCK_RT%i";
			net.num_pts = 1;
		}
	}
	// right-side net
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
	return 0;
fail:
	return rc;
}

static int ckpin(struct fpga_model *model)
{
	int y, x, rc;
	struct w_net net;

	//
	// ckpin is made out of 8 nets.
	//
	// 4 vertical through center_x:
	//	1. top edge to top midbuf
	//	2. top midbuf to center
	// 	3. center to bottom midbuf
	//	4. bottom midbuf to bottom edge
	// 4 horizontal through center_y:
	//	1. left edge to left_gclk_sep_x
	//	2. left_gclk_sep_x to center
	//	3. center to right_gclk_sep_x
	//	4. right_gclk_sep_x to right edge
	//

	// vertical
	{ struct seed_data y_seeds[] = {
		{ Y_INNER_TOP,		"REGV_TTERM_CKPIN%i" },
		{ Y_ROW_HORIZ_AXSYMM,	"CLKV_HCLK_CKPIN%i" },
		{ Y_CHIP_HORIZ_REGS,	"CLKC_CKTB%i" },
		{ Y_INNER_BOTTOM,	"REGV_BTERM_CKPIN%i" },
		{ Y_REGULAR_ROW,	"CLKV_CKPIN%i" },
		{ 0 }};
	seed_stry(model, y_seeds);

	net.last_inc = 7;
	net.pt[0].start_count = 0;
	net.pt[0].x = model->center_x-CENTER_CMTPLL_O;
	net.pt[0].y = TOP_OUTER_ROW;
	net.pt[0].name = "REGT_CKPIN%i";
	net.pt[1].start_count = 0;
	net.pt[1].x = model->center_x-CENTER_CMTPLL_O;
	net.pt[1].y = TOP_INNER_ROW;
	net.pt[1].name = "REGT_TTERM_CKPIN%i";
	net.num_pts = 2;

	for (y = TOP_INNER_ROW; y <= model->y_height - BOT_INNER_ROW; y++) {
		ASSERT(model->tmp_str[y]);
		ASSERT(net.num_pts < sizeof(net.pt)/sizeof(net.pt[0]));

		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].x = model->center_x;
		net.pt[net.num_pts].y = y;
		net.pt[net.num_pts].name = model->tmp_str[y];
		net.num_pts++;

		if (y < model->center_y
		    && YX_TILE(model, y+1, model->center_x)->flags & TF_CENTER_MIDBUF) {
			net.pt[net.num_pts-1].name = "CLKV_CKPIN_BUF%i";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 0;
			net.pt[0].x = model->center_x;
			net.pt[0].y = y;
			net.pt[0].name = "CLKV_MIDBUF_TOP_CKPIN%i";
			net.num_pts = 1;
		} else if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 8;
			net.pt[0].x = model->center_x;
			net.pt[0].y = y;
			net.pt[0].name = "CLKC_CKTB%i";
			net.num_pts = 1;
		} else if (y > model->center_y
		    && YX_TILE(model, y-1, model->center_x)->flags & TF_CENTER_MIDBUF) {
			net.pt[net.num_pts-1].name = "CLKV_CKPIN_BOT_BUF%i";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 0;
			net.pt[0].x = model->center_x;
			net.pt[0].y = y;
			net.pt[0].name = "CLKV_MIDBUF_BOT_CKPIN%i";
			net.num_pts = 1;
		}
	}
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].x = model->center_x-CENTER_CMTPLL_O;
	net.pt[net.num_pts].y = model->y_height-BOT_INNER_ROW;
	net.pt[net.num_pts].name = "REGB_BTERM_CKPIN%i";
	net.num_pts++;
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].x = model->center_x-CENTER_CMTPLL_O;
	net.pt[net.num_pts].y = model->y_height-BOT_OUTER_ROW;
	net.pt[net.num_pts].name = "REGB_CKPIN%i";
	net.num_pts++;
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }

	// horizontal
	{ struct seed_data x_seeds[] = {
		{ X_OUTER_LEFT,			"REGL_CKPIN%i" },
		{ X_INNER_LEFT,			"REGH_LTERM_CKPIN%i" },
		{ X_LEFT_MCB | X_RIGHT_MCB, 	"MCB_REGH_CKPIN%i" },
		{ X_FABRIC_ROUTING_COL
		  | X_LEFT_IO_ROUTING_COL
		  | X_RIGHT_IO_ROUTING_COL
		  | X_FABRIC_BRAM_COL,		"REGH_CKPIN%i_INT" },
		{ X_LEFT_IO_DEVS_COL
		  | X_FABRIC_BRAM_VIA_COL
		  | X_FABRIC_MACC_VIA_COL
		  | X_FABRIC_LOGIC_COL
		  | X_RIGHT_IO_DEVS_COL,	"REGH_CKPIN%i_CLB" },
		{ X_FABRIC_MACC_COL,		"REGH_CKPIN%i_DSP" },
		{ X_CENTER_ROUTING_COL,		"REGC_INT_CKPIN%i_INT" },
		{ X_CENTER_LOGIC_COL,		"REGC_CLECKPIN%i_CLB" },
		{ X_CENTER_CMTPLL_COL,		"REGC_CMT_CKPIN%i" },
		{ X_CENTER_REGS_COL,		"CLKC_CKLR%i" },
		{ X_INNER_RIGHT,		"REGH_RTERM_CKPIN%i" },
		{ X_OUTER_RIGHT,		"REGR_CKPIN%i" },
		{ 0 }};
	seed_strx(model, x_seeds);

	net.last_inc = 7;
	net.num_pts = 0;
	for (x = 0; x < model->x_width; x++) {
		ASSERT(model->tmp_str[x]);
		ASSERT(net.num_pts < sizeof(net.pt)/sizeof(net.pt[0]));

		net.pt[net.num_pts].start_count =
			is_atx(X_CENTER_MAJOR, model, x) ? 8 : 0;
		net.pt[net.num_pts].x = x;
		net.pt[net.num_pts].y = model->center_y;
		net.pt[net.num_pts].name = model->tmp_str[x];
		net.num_pts++;

		if (x == model->left_gclk_sep_x) {
			net.pt[net.num_pts-1].name = "REGH_DSP_IN_CKPIN%i";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 0;
			net.pt[0].x = x;
			net.pt[0].y = model->center_y;
			net.pt[0].name = "REGH_DSP_OUT_CKPIN%i";
			net.num_pts = 1;
		} else if (x == model->center_x) {
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 0;
			net.pt[0].x = x;
			net.pt[0].y = model->center_y;
			net.pt[0].name = "CLKC_CKLR%i";
			net.num_pts = 1;
		} else if (x == model->right_gclk_sep_x) {
			net.pt[net.num_pts-1].name = "REGH_DSP_OUT_CKPIN%i";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			net.pt[0].start_count = 0;
			net.pt[0].x = x;
			net.pt[0].y = model->center_y;
			net.pt[0].name = "REGH_DSP_IN_CKPIN%i";
			net.num_pts = 1;
		}
	}
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
	return 0;
fail:
	return rc;
}

static int clkindirect_feedback(struct fpga_model *model, enum extra_wires wire)
{
	const struct seed_data clk_indirect_seeds[] = {
		{ X_OUTER_LEFT, 		"REGL_CLK_INDIRECT%i" },
		{ X_INNER_LEFT, 		"REGH_LTERM_CLKINDIRECT%i" },
		{ X_LEFT_MCB | X_RIGHT_MCB,	"MCB_REGH_CLKINDIRECT_LR%i" },
		{ X_FABRIC_ROUTING_COL
		  | X_LEFT_IO_ROUTING_COL
		  | X_RIGHT_IO_ROUTING_COL
		  | X_FABRIC_BRAM_COL,	 	"REGH_CLKINDIRECT_LR%i_INT" },
		{ X_LEFT_IO_DEVS_COL
		  | X_FABRIC_BRAM_VIA_COL
		  | X_FABRIC_MACC_VIA_COL
		  | X_FABRIC_LOGIC_COL
		  | X_RIGHT_IO_DEVS_COL, 	"REGH_CLKINDIRECT_LR%i_CLB" },
		{ X_FABRIC_MACC_COL, 		"REGH_CLKINDIRECT_LR%i_DSP" },
		{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKINDIRECT_LR%i_INT" },
		{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKINDIRECT_LR%i_CLB" },
		{ X_CENTER_CMTPLL_COL, 		"REGC_CMT_CLKINDIRECT_LR%i" },
		{ X_CENTER_REGS_COL, 		"REGC_CLKINDIRECT_LR%i" },
		{ X_INNER_RIGHT, 		"REGH_RTERM_CLKINDIRECT%i" },
		{ X_OUTER_RIGHT, 		"REGR_CLK_INDIRECT%i" },
		{ 0 }};
	const struct seed_data clk_feedback_seeds[] = {
		{ X_OUTER_LEFT, 		"REGL_CLK_FEEDBACK%i" },
		{ X_INNER_LEFT, 		"REGH_LTERM_CLKFEEDBACK%i" },
		{ X_LEFT_MCB | X_RIGHT_MCB,	"MCB_REGH_CLKFEEDBACK_LR%i" },
		{ X_FABRIC_ROUTING_COL
		  | X_LEFT_IO_ROUTING_COL
		  | X_RIGHT_IO_ROUTING_COL
		  | X_FABRIC_BRAM_COL,	 	"REGH_CLKFEEDBACK_LR%i_INT" },
		{ X_LEFT_IO_DEVS_COL
		  | X_FABRIC_BRAM_VIA_COL
		  | X_FABRIC_MACC_VIA_COL
		  | X_FABRIC_LOGIC_COL
		  | X_RIGHT_IO_DEVS_COL, 	"REGH_CLKFEEDBACK_LR%i_CLB" },
		{ X_FABRIC_MACC_COL, 		"REGH_CLKFEEDBACK_LR%i_DSP" },
		{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKFEEDBACK_LR%i_INT" },
		{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKFEEDBACK_LR%i_CLB" },
		{ X_CENTER_CMTPLL_COL, 		"REGC_CMT_CLKFEEDBACK_LR%i" },
		{ X_CENTER_REGS_COL, 		"REGC_CLKFEEDBACK_LR%i" },
		{ X_INNER_RIGHT, 		"REGH_RTERM_CLKFEEDBACK%i" },
		{ X_OUTER_RIGHT, 		"REGR_CLK_FEEDBACK%i" },
		{ 0 }};
	int y, x, i, top_pll_y, top_dcm_y, bot_pll_y, bot_dcm_y;
	int regular_pts, rc;
	const char *wstr;
	struct w_net net;

	//
	// clk_indirect and clk_feedback have 6 nets:
	//
	// 2 small nets - one for the top side, one for the bottom side, that
	// connect the pll and dcm devices on each side to their termination
	// tiles.
	//
	// Then 4 larger nets each covering one quarter of the chip (bottom-left,
	// bottom-right, top-left and top-right). They connect together 4 wires
	// of the dcm and pll devices on one side to the entire center_y row
	// (separately for left and right columns). The bottom pll/dcm go to
	// center row 0:3, the top pll/dcm go to center row 4:7.
	//

	if (wire == CLK_INDIRECT) {
		seed_strx(model, clk_indirect_seeds);
		wstr = "INDIRECT";
	} else if (wire == CLK_FEEDBACK) {
		seed_strx(model, clk_feedback_seeds);
		wstr = "FEEDBACK";
	} else FAIL(EINVAL);

	//
	// 2 termination to pll/dcm nets (top and bottom side)
	//

	top_pll_y = -1;
	top_dcm_y = -1;
	bot_pll_y = -1;
	bot_dcm_y = -1;

	net.last_inc = 7;
	net.pt[0].name = pf("REGT_CLK_%s%%i", wstr);
	net.pt[0].start_count = 0;
	net.pt[0].y = TOP_OUTER_ROW;
	net.pt[0].x = model->center_x - CENTER_CMTPLL_O;
	net.pt[1].name = pf("REGT_TTERM_CLK%s%%i", wstr);
	net.pt[1].start_count = 0;
	net.pt[1].y = TOP_INNER_ROW;
	net.pt[1].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts = 2;
	for (y = TOP_FIRST_REGULAR; y <= model->y_height - BOT_LAST_REGULAR_O; y++) {
		if (y < model->center_y
		    && has_device(model, y, model->center_x-CENTER_CMTPLL_O, DEV_PLL)) {
			top_pll_y = y;
			net.pt[net.num_pts].name = pf("PLL_CLK_%s_TB%%i", wstr);
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
		} else if (y < model->center_y
		    && has_device(model, y, model->center_x-CENTER_CMTPLL_O, DEV_DCM)) {
			top_dcm_y = y;
			net.pt[net.num_pts].name = pf("DCM_CLK_%s_LR_TOP%%i", wstr);
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
		} else if (y == model->center_y) {
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
			// start bottom side net
			net.num_pts = 0;
		} else if (y > model->center_y
		    && has_device(model, y, model->center_x-CENTER_CMTPLL_O, DEV_PLL)) {
			bot_pll_y = y;
			net.pt[net.num_pts].name = pf("CMT_PLL_CLK_%s_LRBOT%%i", wstr);
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
		} else if (y > model->center_y
		    && has_device(model, y, model->center_x-CENTER_CMTPLL_O, DEV_DCM)) {
			bot_dcm_y = y;
			net.pt[net.num_pts].name = pf("DCM_CLK_%s_TB_BOT%%i", wstr);
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
		}
	}
	net.pt[net.num_pts].name = pf("REGB_BTERM_CLK%s%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->y_height - BOT_INNER_ROW;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	net.pt[net.num_pts].name = pf("REGB_CLK_%s%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->y_height - BOT_OUTER_ROW;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

	// todo: this function doesn't support more than one pll or dcm
	//       tile per chip side right now.
	ASSERT(top_pll_y != -1 && top_dcm_y != -1
		&& bot_pll_y != -1 && bot_dcm_y != -1);

	//
	// 4 nets for each quarter of the chip
	//
	
	net.last_inc = 3;
	net.num_pts = 0;
	for (x = LEFT_IO_ROUTING; x < model->x_width - RIGHT_INNER_O; x++) {
		if (is_atx(X_CENTER_ROUTING_COL, model, x)) {

			// finish bottom net on left side
			for (i = 0; i < net.num_pts; i++)
				net.pt[i].start_count = 0;
			regular_pts = net.num_pts;
			// pll and dcm
			net.pt[net.num_pts].name = pf("PLL_CLK_%s_TB%%i", wstr);
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = bot_pll_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
			net.pt[net.num_pts].name = pf("DCM_CLK_%s_LR_TOP%%i", wstr);
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = bot_dcm_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
			// left termination
			net.pt[net.num_pts].name = model->tmp_str[LEFT_OUTER_COL];
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = LEFT_OUTER_COL;
			net.num_pts++;
			net.pt[net.num_pts].name = model->tmp_str[LEFT_INNER_COL];
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = LEFT_INNER_COL;
			net.num_pts++;
			// 3 columns from center major
			net.pt[net.num_pts].start_count = 0;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_ROUTING_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			net.pt[net.num_pts].start_count = 8;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_LOGIC_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			net.pt[net.num_pts].start_count = 8;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

			// finish top net on left side
			net.num_pts = regular_pts;
			for (i = 0; i < net.num_pts; i++)
				net.pt[i].start_count = 4;
			// pll and dcm
			net.pt[net.num_pts].name = pf("CMT_PLL_CLK_%s_LRBOT%%i", wstr);
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = top_pll_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
			net.pt[net.num_pts].name = pf("DCM_CLK_%s_TB_BOT%%i", wstr);
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = top_dcm_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.num_pts++;
			// left termination
			net.pt[net.num_pts].name = model->tmp_str[LEFT_OUTER_COL];
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = LEFT_OUTER_COL;
			net.num_pts++;
			net.pt[net.num_pts].name = model->tmp_str[LEFT_INNER_COL];
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = LEFT_INNER_COL;
			net.num_pts++;
			// 3 colums from center major
			net.pt[net.num_pts].start_count = 4;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_ROUTING_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			net.pt[net.num_pts].start_count = 12;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_LOGIC_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			net.pt[net.num_pts].start_count = 12;
			net.pt[net.num_pts].y = model->center_y;
			net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
			net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
			net.num_pts++;
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

			// continue on right side
			net.num_pts = 0;
			x = model->center_x;
			continue;
		}
		net.pt[net.num_pts].name = model->tmp_str[x];
		net.pt[net.num_pts].y = model->center_y;
		net.pt[net.num_pts].x = x;
		net.num_pts++;
	}

	// finish bottom net on right side
	for (i = 0; i < net.num_pts; i++)
		net.pt[i].start_count = 0;
	regular_pts = net.num_pts;
	// pll and dcm
	net.pt[net.num_pts].name = pf("PLL_CLK_%s_TB%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = bot_pll_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	net.pt[net.num_pts].name = pf("DCM_CLK_%s_LR_TOP%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = bot_dcm_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	// right termination
	net.pt[net.num_pts].start_count = 4;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->x_width - RIGHT_OUTER_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	net.pt[net.num_pts].start_count = 4;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->x_width - RIGHT_INNER_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	// 2 columns from center major
	net.pt[net.num_pts].start_count = 4;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	net.pt[net.num_pts].start_count = 4;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->center_x;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);

	// finish top net on right side
	net.num_pts = regular_pts;
	for (i = 0; i < net.num_pts; i++)
		net.pt[i].start_count = 4;
	// pll and dcm
	net.pt[net.num_pts].name = pf("CMT_PLL_CLK_%s_LRBOT%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = top_pll_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	net.pt[net.num_pts].name = pf("DCM_CLK_%s_TB_BOT%%i", wstr);
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = top_dcm_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.num_pts++;
	// right termination
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->x_width - RIGHT_OUTER_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->x_width - RIGHT_INNER_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	// 2 columns from center major
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->center_x - CENTER_CMTPLL_O;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	net.pt[net.num_pts].start_count = 0;
	net.pt[net.num_pts].y = model->center_y;
	net.pt[net.num_pts].x = model->center_x;
	net.pt[net.num_pts].name = model->tmp_str[net.pt[net.num_pts].x];
	net.num_pts++;
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc);
	return 0;
fail:
	return rc;
}

static int run_gclk(struct fpga_model* model)
{
	int x, i, rc, row, row_top_y, is_break;
	struct w_net gclk_net;

	for (row = model->cfg_rows-1; row >= 0; row--) {
		row_top_y = TOP_IO_TILES + (model->cfg_rows-1-row)*(8+1/* middle of row */+8);
		if (row < (model->cfg_rows/2)) row_top_y++; // center regs

		// net that connects the hclk of half the chip together horizontally
		gclk_net.last_inc = 15;
		gclk_net.num_pts = 0;
		for (x = LEFT_IO_ROUTING;; x++) {
			if (gclk_net.num_pts >= sizeof(gclk_net.pt) / sizeof(gclk_net.pt[0])) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
			gclk_net.pt[gclk_net.num_pts].start_count = 0;
			gclk_net.pt[gclk_net.num_pts].x = x;
			gclk_net.pt[gclk_net.num_pts].y = row_top_y+8;

			if (is_atx(X_LEFT_IO_ROUTING_COL|X_FABRIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_INT";
			} else if (is_atx(X_LEFT_MCB, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_MCB";
			} else if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL|X_LEFT_IO_DEVS_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_CLB";
			} else if (is_atx(X_FABRIC_BRAM_VIA_COL|X_FABRIC_MACC_VIA_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_BRAM_INTER";
			} else if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_BRAM";
			} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_DSP";
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts].y = row_top_y+7;
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_CMT_GCLK%i_CLB";
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "CLKV_BUFH_LEFT_L%i";

				// connect left half
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;

				// start right half
				gclk_net.pt[0].start_count = 0;
				gclk_net.pt[0].x = x;
				gclk_net.pt[0].y = row_top_y+8;
				gclk_net.pt[0].name = "CLKV_BUFH_RIGHT_R%i";
				gclk_net.num_pts = 1;

			} else if (is_atx(X_RIGHT_IO_ROUTING_COL, model, x)) {
				gclk_net.pt[gclk_net.num_pts++].name = "HCLK_GCLK%i_INT";

				// connect right half
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;
				break;
			}
			if (x >= model->x_width) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
		}
	}
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (row = model->cfg_rows-1; row >= 0; row--) {
				row_top_y = 2 /* top IO */ + (model->cfg_rows-1-row)*(8+1/* middle of row */+8);
				if (row < (model->cfg_rows/2)) row_top_y++; // center regs

				is_break = 0;
 				if (is_atx(X_LEFT_IO_ROUTING_COL|X_RIGHT_IO_ROUTING_COL, model, x)) {
					if (row && row != model->cfg_rows/2)
						is_break = 1;
				} else {
					if (row)
						is_break = 1;
					else if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x))
						is_break = 1;
				}

				// vertical net inside row, pulling together 16 gclk
				// wires across top (8 tiles) and bottom (8 tiles) half
				// of the row.
				for (i = 0; i < 8; i++) {
					gclk_net.pt[i].name = "GCLK%i";
					gclk_net.pt[i].start_count = 0;
					gclk_net.pt[i].y = row_top_y+i;
					gclk_net.pt[i].x = x;
				}
				gclk_net.last_inc = 15;
				gclk_net.num_pts = 8;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;
				for (i = 0; i < 8; i++)
					gclk_net.pt[i].y += 9;
				if (is_break)
					gclk_net.pt[7].name = "GCLK%i_BRK";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;

				// vertically connects gclk of each row tile to
				// hclk tile at the middle of the row
				for (i = 0; i < 8; i++) {
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						row_top_y+i, x, "GCLK%i",  0, 15,
						row_top_y+8, x, "HCLK_GCLK_UP%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						row_top_y+9+i, x, (i == 7 && is_break) ? "GCLK%i_BRK" : "GCLK%i", 0, 15,
						row_top_y+8, x, "HCLK_GCLK%i", 0))) goto xout;
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int run_gclk_horiz_regs(struct fpga_model* model)
{
	int x, rc;

	// local nets around the center on the left side
	{ struct w_net net = {
		.last_inc = 3, .num_pts = 3, .pt =
		{{ "REGL_GCLK%i", 0, model->center_y, LEFT_OUTER_COL },
		 { "REGH_LTERM_GCLK%i", 0, model->center_y, LEFT_INNER_COL },
		 { "REGH_IOI_INT_GCLK%i", 0, model->center_y, LEFT_IO_ROUTING }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{
		const char* str[3] = {"REGL_GCLK%i", "REGH_LTERM_GCLK%i", "REGH_IOI_INT_GCLK%i"};
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->center_y-2, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 2, 3,
			model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i_EXT", 2))) goto xout;
		for (x = LEFT_OUTER_COL; x <= LEFT_IO_ROUTING; x++) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 0, 1,
				model->center_y, x, str[x], 0))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i_EXT", 2, 3,
				model->center_y, x, str[x], 2))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-2, LEFT_IO_ROUTING, "INT_BUFPLL_GCLK%i", 2, 3,
				model->center_y, x, str[x], 2))) goto xout;
		}
	}
	// and right side
	{ struct w_net net = {
		.last_inc = 3, .num_pts = 3, .pt =
		{{ "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "MCB_REGH_GCLK%i", 0, model->center_y, model->x_width-RIGHT_MCB_O },
		 { "REGR_GCLK%i", 0, model->center_y, model->x_width-RIGHT_OUTER_O }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{
		const char* str[5] = {"REGH_IOI_INT_GCLK%i", "REGH_RIOI_GCLK%i", "MCB_REGH_GCLK%i", "REGR_RTERM_GCLK%i", "REGR_GCLK%i"};
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->center_y-2, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
			model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2))) goto xout;
		for (x = model->x_width-RIGHT_IO_ROUTING_O; x <= model->x_width-RIGHT_OUTER_O; x++) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2, 3,
				model->center_y, x, str[x-(model->x_width-RIGHT_IO_ROUTING_O)], 2))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-2, model->x_width-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
				model->center_y, x, str[x-(model->x_width-RIGHT_IO_ROUTING_O)], 2))) goto xout;
		}
	}
	{ struct w_net net = {
		.last_inc = 1, .num_pts = 4, .pt =
		{{ "INT_BUFPLL_GCLK%i", 0, model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_INT_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGH_RTERM_GCLK%i", 0, model->center_y, model->x_width-RIGHT_INNER_O }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	{ struct w_net net = {
		.last_inc = 1, .num_pts = 3, .pt =
		{{ "REGH_IOI_INT_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGR_RTERM_GCLK%i", 2, model->center_y, model->x_width-RIGHT_INNER_O }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	// the naming is a little messed up here, and the networks are
	// actually simpler than represented here (with full 0:3 connections).
	// But until we have better representation of wire networks, this has
	// to suffice.
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGR_RTERM_GCLK%i", 2))) goto xout;
	// same from MCB_REGH_GCLK...
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->x_width-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->x_width-RIGHT_INNER_O,
			"REGR_RTERM_GCLK%i", 2))) goto xout;
	return 0;
xout:
	return rc;
}

static int run_gclk_vert_regs(struct fpga_model* model)
{
	struct w_net net;
	int rc, i;

	// net tying together 15 gclk lines from row 10..27
	net.last_inc = 15;
	for (net.num_pts = 0; net.num_pts <= 17; net.num_pts++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, net.num_pts+10))
			net.pt[net.num_pts].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (net.num_pts == 9) // row 19
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].y = net.num_pts+10;
		net.pt[net.num_pts].x = model->center_x;
	}
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 19..53
	net.last_inc = 15;
	for (net.num_pts = 0; net.num_pts <= 34; net.num_pts++) { // row 19..53
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, net.num_pts+19))
			net.pt[net.num_pts].name = "REGV_GCLKH_MAIN%i";
		else if (is_aty(Y_CHIP_HORIZ_REGS, model, net.num_pts+19))
			net.pt[net.num_pts].name = "CLKC_GCLK_MAIN%i";
		else if (net.num_pts == 16) // row 35
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i_BRK";
		else
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i";
		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].y = net.num_pts+19;
		net.pt[net.num_pts].x = model->center_x;
	}
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 45..62
	net.last_inc = 15;
	for (net.num_pts = 0; net.num_pts <= 17; net.num_pts++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, net.num_pts+45))
			net.pt[net.num_pts].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (net.num_pts == 8) // row 53
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pt[net.num_pts].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pt[net.num_pts].start_count = 0;
		net.pt[net.num_pts].y = net.num_pts+45;
		net.pt[net.num_pts].x = model->center_x;
	}
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// a few local gclk networks at the center top and bottom
	{ struct w_net n = {
		.last_inc = 1, .num_pts = 4, .pt =
		{{ "REGT_GCLK%i",	0, TOP_OUTER_ROW, model->center_x-1 },
		 { "REGT_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x-1 },
		 { "REGV_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x },
		 { "BUFPLL_TOP_GCLK%i", 0, TOP_INNER_ROW, model->center_x+1 }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
	{ struct w_net n = {
		.last_inc = 1, .num_pts = 4, .pt =
		{{ "REGB_GCLK%i",	0, model->y_height-1, model->center_x-1 },
		 { "REGB_BTERM_GCLK%i", 0, model->y_height-2, model->center_x-1 },
		 { "REGV_BTERM_GCLK%i", 0, model->y_height-2, model->center_x },
		 { "BUFPLL_BOT_GCLK%i", 0, model->y_height-2, model->center_x+1 }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }

	// wire up gclk from tterm down to top 8 rows at center_x+1
	for (i = TOP_IO_TILES; i <= TOP_IO_TILES+HALF_ROW; i++) {
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			TOP_INNER_ROW, model->center_x+1,
				"IOI_TTERM_GCLK%i",  0, 15,
			i, model->center_x+1,
				(i == TOP_IO_TILES+HALF_ROW) ?
				"HCLK_GCLK_UP%i" : "GCLK%i", 0))) goto xout;
	}
	// same at the bottom upwards
	for (i = model->y_height-2-1; i >= model->y_height-2-HALF_ROW-1; i--) {
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->y_height-2, model->center_x+1,
				"IOI_BTERM_GCLK%i",  0, 15,
			i, model->center_x+1,
				(i == model->y_height-2-HALF_ROW-1) ?
				"HCLK_GCLK%i" : "GCLK%i", 0))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int connect_logic_carry(struct fpga_model* model)
{
	int x, y, rc;

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_M)) {
					if (is_aty(Y_CHIP_HORIZ_REGS, model, y-1)
					    && has_device_type(model, y-2, x, DEV_LOGIC, LOGIC_M)) {
						struct w_net net = {
							.last_inc = 0, .num_pts = 3, .pt =
							{{ "M_CIN",		0, y-2, x },
							 { "REGH_CLEXM_COUT",	0, y-1, x },
							 { "M_COUT_N",		0,   y, x }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y-1)
					    && has_device_type(model, y-2, x, DEV_LOGIC, LOGIC_M)) {
						struct w_net net = {
							0, 3,
							{{ "M_CIN",		0, y-2, x },
							 { "HCLK_CLEXM_COUT",	0, y-1, x },
							 { "M_COUT_N",		0,   y, x }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (has_device_type(model, y-1, x, DEV_LOGIC, LOGIC_M)) {
						if ((rc = add_conn_bi(model, y, x, "M_COUT_N", y-1, x, "M_CIN"))) goto xout;
					}
				}
				else if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_L)) {
					if (is_aty(Y_CHIP_HORIZ_REGS, model, y-1)) {
						if (x == model->center_x - CENTER_LOGIC_O) {
							struct w_net net = {
								.last_inc = 0, .num_pts = 4, .pt =
								{{ "L_CIN",			0, y-3, x },
								 { "INT_INTERFACE_COUT",	0, y-2, x },
								 { "REGC_CLE_COUT",		0, y-1, x },
								 { "XL_COUT_N",			0,   y, x }}};
							if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
						} else {
							struct w_net net = {
								.last_inc = 0, .num_pts = 3, .pt =
								{{ "L_CIN",		0, y-2, x },
								 { "REGH_CLEXL_COUT",	0, y-1, x },
								 { "XL_COUT_N",		0,   y, x }}};
							if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
						}
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y-1)) {
						struct w_net net = {
							.last_inc = 0, .num_pts = 3, .pt =
							{{ "L_CIN",		0, y-2, x },
							 { "HCLK_CLEXL_COUT",	0, y-1, x },
							 { "XL_COUT_N",		0,   y, x }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y-2)
						   && (x == model->center_x - CENTER_LOGIC_O)) {
						struct w_net net = {
							.last_inc = 0, .num_pts = 5, .pt =
							{{ "L_CIN",			0, y-4, x },
							 { "INT_INTERFACE_COUT",	0, y-3, x },
							 { "HCLK_CLEXL_COUT",		0, y-2, x },
							 { "INT_INTERFACE_COUT_BOT",	0, y-1, x },
							 { "XL_COUT_N",			0,   y, x }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (has_device_type(model, y-1, x, DEV_LOGIC, LOGIC_L)) {
						if ((rc = add_conn_bi(model, y, x, "XL_COUT_N", y-1, x, "L_CIN"))) goto xout;
					}
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int connect_clk_sr(struct fpga_model* model, const char* clk_sr)
{
	int x, y, i, rc;

	// fabric logic, bram, macc
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (has_device(model, y, x+2, DEV_BRAM16)) {
					for (i = 0; i <= 3; i++) {
						struct w_net n = {
							.last_inc = 1, .num_pts = 3, .pt =
							{{ pf("%s%%i", clk_sr), 0, y-i, x },
							 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-i, x+1 },
							 { pf("BRAM_%s%%i_INT%i", clk_sr, i), 0, y, x+2 }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc);
					}
				}
				if (has_device(model, y, x+2, DEV_MACC)) {
					for (i = 0; i <= 3; i++) {
						struct w_net n = {
							.last_inc = 1, .num_pts = 3, .pt =
							{{ pf("%s%%i", clk_sr), 0, y-i, x },
							 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-i, x+1 },
							 { pf("MACC_%s%%i_INT%i", clk_sr, i), 0, y, x+2 }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc);
					}
				}
			}
		}
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (has_device_type(model, y, x+1, DEV_LOGIC, LOGIC_M)) {
					if ((rc = add_conn_range(model,
						NOPREF_BI_F,
						y, x, pf("%s%%i", clk_sr), 0, 1,
						y, x+1, pf("CLEXM_%s%%i", clk_sr), 0)))
							FAIL(rc);
				} else if (has_device_type(model, y, x+1, DEV_LOGIC, LOGIC_L)) {
					if ((rc = add_conn_range(model,
						NOPREF_BI_F,
						y, x, pf("%s%%i", clk_sr), 0, 1,
						y, x+1, pf("CLEXL_%s%%i", clk_sr), 0)))
							FAIL(rc);
				} else if (has_device(model, y, x+1, DEV_ILOGIC)) {
					if ((rc = add_conn_range(model,
						NOPREF_BI_F,
						y, x, pf("%s%%i", clk_sr), 0, 1,
						y, x+1, pf("IOI_%s%%i", clk_sr), 0)))
							FAIL(rc);
				}
			}
		}
	}
	// center PLLs and DCMs
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y-1, model->center_x-CENTER_ROUTING_O, pf("%s%%i", clk_sr), 0, 1,
		model->center_y-1, model->center_x-CENTER_LOGIC_O, pf("INT_INTERFACE_REGC_%s%%i", clk_sr), 0)))
			FAIL(rc);

	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
			if (has_device(model, y-1, model->center_x-CENTER_CMTPLL_O, DEV_PLL)) {
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ pf("%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { pf("PLL_CLB2_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt = 
					{{ pf("%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { pf("PLL_CLB1_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
			}
			if (has_device(model, y-1, model->center_x-CENTER_CMTPLL_O, DEV_DCM)) {
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ pf("%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { pf("DCM_CLB2_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ pf("%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { pf("DCM_CLB1_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) FAIL(rc); }
			}
		}
	}
	// left and right side
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
				model, y))
			continue;

		x = LEFT_IO_ROUTING;
		if (y < TOP_IO_TILES+LEFT_LOCAL_HEIGHT || y > model->y_height-BOT_IO_TILES-LEFT_LOCAL_HEIGHT-1) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_LOCAL_%s%%i", clk_sr), 0))) FAIL(rc);
		} else if (is_aty(Y_LEFT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("IOI_%s%%i", clk_sr), 0))) FAIL(rc);
		} else {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_%s%%i", clk_sr), 0))) FAIL(rc);
		}

		x = model->x_width - RIGHT_IO_ROUTING_O;
		if (y < TOP_IO_TILES+RIGHT_LOCAL_HEIGHT || y > model->y_height-BOT_IO_TILES-RIGHT_LOCAL_HEIGHT-1) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_LOCAL_%s%%i", clk_sr), 0))) FAIL(rc);
		} else if (is_aty(Y_RIGHT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("IOI_%s%%i", clk_sr), 0))) FAIL(rc);
		} else {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_%s%%i", clk_sr), 0))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int run_gfan(struct fpga_model* model)
{
	int x, y, i, rc;

	// left and right IO devs
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		if (is_aty(Y_LEFT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y, LEFT_IO_ROUTING, "INT_IOI_GFAN%i", 0, 1,
				y, LEFT_IO_DEVS, "IOI_GFAN%i", 0))) goto xout;
		}
		if (is_aty(Y_RIGHT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y, model->x_width-RIGHT_IO_ROUTING_O,
					"INT_IOI_GFAN%i", 0, 1,
				y, model->x_width-RIGHT_IO_DEVS_O,
					"IOI_GFAN%i", 0))) goto xout;
		}
	}
	// top and bottom IO devs
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x)) {
			for (i = 0; i < TOPBOT_IO_ROWS; i++) {
				if ((rc = add_conn_range(model, NOPREF_BI_F,
					TOP_OUTER_IO+i, x,
						"INT_IOI_GFAN%i", 0, 1,
					TOP_OUTER_IO+i, x+1,
						"IOI_GFAN%i", 0))) goto xout;
				if ((rc = add_conn_range(model, NOPREF_BI_F,
					model->y_height-BOT_OUTER_IO-i, x,
						"INT_IOI_GFAN%i", 0, 1,
					model->y_height-BOT_OUTER_IO-i, x+1,
						"IOI_GFAN%i", 0))) goto xout;
			}
		}
	}
	// center devs
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
			if (YX_TILE(model, y-1, model->center_x-CENTER_CMTPLL_O)->flags & TF_DCM_DEV) {
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ "INT_IOI_GFAN%i", 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { "DCM2_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
				{ struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ "INT_IOI_GFAN%i", 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { "DCM1_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
			} else if (YX_TILE(model, y-1, model->center_x-CENTER_CMTPLL_O)->flags & TF_PLL_DEV) {
				struct w_net net = {
					.last_inc = 1, .num_pts = 3, .pt =
					{{ "INT_IOI_GFAN%i", 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { "PLL_CLB2_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int run_io_wires(struct fpga_model* model)
{
	static const char* s[] = { "IBUF", "O", "T", "" };
	int x, y, i, rc;

	//
	// 1. input wires from IBUF into the chip "IBUF"
	// 2. output wires from the chip into O "O"
	// 3. T wires "T"
	//

	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {

		y = 0;
		if (has_device(model, y, x, DEV_IOB)) {
			for (i = 0; s[i][0]; i++) {
				struct w_net net1 = {
					.last_inc = 1, .num_pts = 4, .pt =
					{{ pf("TIOB_%s%%i",		s[i]), 0,   y,   x },
					 { pf("IOI_TTERM_IOIUP_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIUP_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i",	s[i]), 0, y+2, x+1 }}};
				struct w_net net2 = {
					.last_inc = 1, .num_pts = 5, .pt =
					{{ pf("TIOB_%s%%i",		s[i]), 2,   y,   x },
					 { pf("IOI_TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i_EXT",	s[i]), 0, y+2, x+1 },
					 { pf("TIOI_INNER_%s%%i",	s[i]), 0, y+3, x+1 }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net1))) goto xout;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net2))) goto xout;
			}
		}

		y = model->y_height - BOT_OUTER_ROW;
		if (has_device(model, y, x, DEV_IOB)) {
			for (i = 0; s[i][0]; i++) {
				struct w_net net1 = {
					.last_inc = 1, .num_pts = 5, .pt =
					{{ pf("BIOI_INNER_%s%%i",	s[i]), 0, y-3, x+1 },
					 { pf("BIOI_OUTER_%s%%i_EXT",	s[i]), 0, y-2, x+1 },
					 { pf("BTERM_IOIUP_%s%%i",	s[i]), 0, y-1, x+1 },
					 { pf("IOI_BTERM_IOIUP_%s%%i",	s[i]), 0, y-1,   x },
					 { pf("BIOB_%s%%i",		s[i]), 0,   y,   x }}};

				if ((rc = add_conn_net(model, NOPREF_BI_F, &net1))) goto xout;
 				// The following is actually a net, but add_conn_net()/w_net
 				// does not support COUNT_DOWN ranges right now.
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-1, x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F,   y,   x, pf("BIOB_%s%%i", s[i]), 2, 3, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 1 | COUNT_DOWN);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1,   x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 0);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1,   x, pf("IOI_BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 0);
				if (rc) goto xout;
				rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, pf("BTERM_IOIBOT_%s%%i", s[i]), 0, 1, y-2, x+1, pf("BIOI_OUTER_%s%%i", s[i]), 0);
				if (rc) goto xout;
			}
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC)) {
			x = 0;
			for (i = 0; s[i][0]; i++) {
				struct w_net net = {
					.last_inc = 1, .num_pts = 4, .pt =
					{{ pf("LIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("LTERM_IOB_%s%%i",	s[i]), 0, y, x+1 },
					 { pf("LIOI_INT_%s%%i",		s[i]), 0, y, x+2 },
					 { pf("LIOI_IOB_%s%%i",		s[i]), 0, y, x+3 }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
		if (has_device(model, y, model->x_width - RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			x = model->x_width - RIGHT_OUTER_O;
			for (i = 0; s[i][0]; i++) {
				struct w_net net = {
					.last_inc = 1, .num_pts = 4, .pt =
					{{ pf("RIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("RTERM_IOB_%s%%i",	s[i]), 0, y, x-1 },
					 { pf("MCB_%s%%i",		s[i]), 0, y, x-2 },
					 { pf("RIOI_IOB_%s%%i",		s[i]), 0, y, x-3 }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int run_logic_inout(struct fpga_model* model)
{
	struct fpga_tile* tile;
	char buf[128];
	int x, y, i, rc;

	// LOGICOUT
	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_LOGIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
			for (y = 0; y < model->y_height; y++) {
				tile = &model->tiles[y * model->x_width + x];
				if (tile[1].flags & TF_LOGIC_XM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXM_LOGICOUT%i", 0))) FAIL(rc);
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXL_LOGICOUT%i", 0))) FAIL(rc);
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) FAIL(rc);
				}
			}
		}
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) FAIL(rc);
				if (YX_TILE(model, y, x)[2].flags & TF_BRAM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT3", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT2", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT1", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT0", 0))) FAIL(rc);
				}
				if (YX_TILE(model, y, x)[2].flags & TF_MACC_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT3", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT2", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT1", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT0", 0))) FAIL(rc);
				}
			}
		}
		if (is_atx(X_CENTER_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x, "LOGICOUT%i", 0, 23, y+1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) FAIL(rc);
					if (YX_TILE(model, y-1, x+2)->flags & TF_DCM_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB2_LOGICOUT%i", 0))) FAIL(rc);
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB1_LOGICOUT%i", 0))) FAIL(rc);
					} else if (YX_TILE(model, y-1, x+2)->flags & TF_PLL_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB2_LOGICOUT%i", 0))) FAIL(rc);
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB1_LOGICOUT%i", 0))) FAIL(rc);
					}
				}
				if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_REGC_LOGICOUT%i", 0))) FAIL(rc);
				}
			}
		}
		if (is_atx(X_LEFT_IO_ROUTING_COL|X_RIGHT_IO_ROUTING_COL, model, x)) {
			int wired_side, local_size;
			if (is_atx(X_LEFT_IO_ROUTING_COL, model, x)) {
				local_size = LEFT_LOCAL_HEIGHT;
				wired_side = Y_LEFT_WIRED;
			} else {
				local_size = RIGHT_LOCAL_HEIGHT;
				wired_side = Y_RIGHT_WIRED;
			}
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if (y < TOP_IO_TILES+local_size || y > model->y_height-BOT_IO_TILES-local_size-1) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOCAL_LOGICOUT%i", 0))) FAIL(rc);
				} else if (is_aty(wired_side, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) FAIL(rc);
				} else {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) FAIL(rc);
				}
			}
		}
	}
	// LOGICIN
	for (i = 0; i < model->cfg_rows; i++) {
		y = TOP_IO_TILES + HALF_ROW + i*ROW_SIZE;
		if (y > model->center_y) y++; // central regs

		if (i%2) { // DCM
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 0))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB4"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 5))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB10"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 11))) FAIL(rc);

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 0))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB4"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 5))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB10"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 11))) FAIL(rc);
		} else { // PLL
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 0))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB4"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 5))) FAIL(rc);
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB10"))) FAIL(rc);
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 11))) FAIL(rc);

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 0, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB1_LOGICINB%i", 0))) FAIL(rc);
		}
	}

	for (x = LEFT_IO_ROUTING;; x = model->x_width - RIGHT_IO_ROUTING_O) {
		for (y = TOP_FIRST_REGULAR; y <= model->y_height - BOT_LAST_REGULAR_O; y++) {
			if (has_device(model, y, x+1, DEV_BSCAN)
			    || has_device(model, y, x+1, DEV_OCT_CALIBRATE)
			    || has_device(model, y, x+1, DEV_STARTUP)) {
				if ((rc = add_conn_range(model, NOPREF_BI_F,
					y, x, "LOGICIN_B%i", 0, 62,
					y, x+1, "INT_INTERFACE_LOCAL_LOGICBIN%i", 0))) FAIL(rc);
			} else if (y > model->xci->mcb_ypos-6 && y <= model->xci->mcb_ypos+6) {
				struct w_net n = {
					.last_inc = 62, .num_pts = 3, .pt =
					{{ "LOGICIN_B%i", 0, y, x },
					 { "INT_INTERFACE_LOGICBIN%i", 0, y, x+1 },
					 { pf("MCB_L_CLEXM_LOGICIN_B%%i_%i", y-(model->xci->mcb_ypos-6)), 0, model->xci->mcb_ypos, x+2 }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc);
			} else {
				for (i = 0; i < model->xci->num_mui; i++) {
					if (y == model->xci->mui_pos[i] || y == model->xci->mui_pos[i]+1) {
						struct w_net n = {
							.last_inc = 62, .num_pts = 3, .pt =
							{{ "LOGICIN_B%i", 0, y, x },
							 { "INT_INTERFACE_LOGICBIN%i", 0, y, x+1 },
							 { pf("MUI_LOGICINB%%i_INT%i", 1-(y-model->xci->mui_pos[i])), 0, model->xci->mui_pos[i]+1, x+2 }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) FAIL(rc);
						break;
					}
				}
				if (i >= model->xci->num_mui
				    && !is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y)
				    && !has_device(model, y, x+1, DEV_IODELAY)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						y, x, "LOGICIN_B%i", 0, 62,
						y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) FAIL(rc);
				}
			}
		}
		if (x == model->x_width-RIGHT_IO_ROUTING_O)
			break;
	}
	for (y = 0; y < model->y_height; y++) {
		for (x = 0; x < model->x_width; x++) {
			tile = YX_TILE(model, y, x);

			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				static const int north_p[4] = {21, 28, 52, 60};
				static const int south_p[4] = {20, 36, 44, 62};

				for (i = 0; i < sizeof(north_p)/sizeof(north_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN%i", north_p[i])))) FAIL(rc);
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN_N%i", north_p[i])))) FAIL(rc);
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) FAIL(rc);
						if ((rc = add_conn_bi_pref(model, y-1, x, pf("LOGICIN_N%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) FAIL(rc);
					}
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN_N%i", north_p[i]), y+1, x, pf("LOGICIN_N%i", north_p[i])))) FAIL(rc);
					}
				}
				for (i = 0; i < sizeof(south_p)/sizeof(south_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN_S%i", south_p[i]), y-1, x, pf("LOGICIN_S%i", south_p[i])))) FAIL(rc);
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) FAIL(rc);
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) FAIL(rc);
						if ((rc = add_conn_bi_pref(model, y+1, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) FAIL(rc);
					} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x))
							if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) FAIL(rc);
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN_S%i", south_p[i])))) FAIL(rc);
					}
				}

				if (tile[1].flags & TF_LOGIC_XM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "CLEXM_LOGICIN_B%i", 0))) FAIL(rc);
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i",  0, 35, y, x+1, "CLEXL_LOGICIN_B%i",  0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 37, 43, y, x+1, "CLEXL_LOGICIN_B%i", 37))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 45, 52, y, x+1, "CLEXL_LOGICIN_B%i", 45))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 54, 60, y, x+1, "CLEXL_LOGICIN_B%i", 54))) FAIL(rc);
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 3, y, x+1, "IOI_LOGICINB%i", 0))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 5, 9, y, x+1, "IOI_LOGICINB%i", 5))) FAIL(rc);
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 11, 62, y, x+1, "IOI_LOGICINB%i", 11))) FAIL(rc);
				}
				if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) FAIL(rc);
					if (tile[2].flags & TF_BRAM_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "BRAM_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) FAIL(rc);
						}
					}
				}
				if (is_atx(X_FABRIC_MACC_ROUTING_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) FAIL(rc);
					if (tile[2].flags & TF_MACC_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "MACC_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) FAIL(rc);
						}
					}
				}
				if (is_atx(X_CENTER_REGS_COL, model, x+3)) {
					if (tile[2].flags & (TF_PLL_DEV|TF_DCM_DEV)) {
						const char* prefix = (tile[2].flags & TF_PLL_DEV) ? "PLL_CLB2" : "DCM_CLB2";

						for (i = 0;; i = 2) {
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN4"))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 5))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN10"))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 11))) FAIL(rc);

							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y, x+2, pf("%s_LOGICINB%%i", prefix), 0))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y, x+2, pf("%s_LOGICINB4", prefix)))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y, x+2, pf("%s_LOGICINB%%i", prefix), 5))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y, x+2, pf("%s_LOGICINB10", prefix)))) FAIL(rc);
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y, x+2, pf("%s_LOGICINB%%i", prefix), 11))) FAIL(rc);

							if (tile[2].flags & TF_PLL_DEV) {
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62, y+2, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) FAIL(rc);
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62,   y, x+2, "PLL_CLB1_LOGICINB%i", 0))) FAIL(rc);
								break;
							}
							if (i == 2) break;
							prefix = "DCM_CLB1";
						}
					}
					if (is_aty(Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_range(model, NOPREF_BI_F,   y, x, "LOGICIN_B%i",  0, 62,   y, x+1, "INT_INTERFACE_REGC_LOGICBIN%i", 0))) FAIL(rc);
						int clk_pins[16] = { 24, 15, 7, 42, 5, 12, 62, 16, 47, 20, 38, 23, 48, 57, 44, 4 };
						for (i = 0; i <= 15; i++) {
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+1, pf("REGC_CLE_SEL%i", i)))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+2, pf("REGC_CMT_SEL%i", i)))) FAIL(rc);
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+3, pf("CLKC_SEL%i_PLL", i)))) FAIL(rc);
						}
					}
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int net_includes_routing(struct fpga_model *model, const struct w_net *net)
{
	int i;
	for (i = 0; i < net->num_pts; i++)
		if (is_atyx(YX_ROUTING_TILE, model, net->pt[i].y, net->pt[i].x))
			return 1;
	return 0;
}

static int y_to_regular(struct fpga_model *model, int y)
{
	if (y < TOP_FIRST_REGULAR)
		return TOP_FIRST_REGULAR;
	if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
		return y-1;
	if (y > model->y_height-BOT_LAST_REGULAR_O)
		return model->y_height-BOT_LAST_REGULAR_O;
	return y;
}

static char dirwire_next_bamce(enum wire_type wire, char bamce)
{
	if (W_IS_LEN4(wire)) {
		// len-4 goes from B to A to M to C to E
		if (bamce == 'B') return 'A';
		if (bamce == 'A') return 'M';
		if (bamce == 'M') return 'C';
		if (bamce == 'C') return 'E';
		if (bamce == 'E') return 'B';
	} else if (W_IS_LEN2(wire)) {
		// len-2 goes from B to M to E
		if (bamce == 'B') return 'M';
		if (bamce == 'M') return 'E';
		if (bamce == 'E') return 'B';
	} else if (W_IS_LEN1(wire)) {
		// len-1 goes from B to E
		if (bamce == 'B') return 'E';
		if (bamce == 'E') return 'B';
	}
	HERE();
	return bamce;
}

static void dirwire_next_hop(struct fpga_model *model, enum wire_type wire, char bamce, int *y, int *x)
{
	if (wire == W_NN4 || wire == W_NN2 || wire == W_NL1 || wire == W_NR1)
		(*y)--;
	else if (wire == W_SS4 || wire == W_SS2 || wire == W_SL1 || wire == W_SR1)
		(*y)++;
	else if (wire == W_EE4 || wire == W_EE2 || wire == W_EL1 || wire == W_ER1)
		(*x)++;
	else if (wire == W_WW4 || wire == W_WW2 || wire == W_WL1 || wire == W_WR1)
		(*x)--;
	else if (wire == W_NW4 || wire == W_NW2) {
		if (bamce == 'B' || bamce == 'A')
			(*y)--;
		else if (bamce == 'M') {
			if (is_aty(Y_INNER_BOTTOM|Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, *y))
				(*y)--;
			else
				(*x)--;
		} else if (bamce == 'C' || bamce == 'E')
			(*x)--;
		else HERE();
	} else if (wire == W_NE4 || wire == W_NE2) {
		if (bamce == 'B' || bamce == 'A')
			(*y)--;
		else if (bamce == 'M') {
			if (is_aty(Y_INNER_BOTTOM|Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, *y))
				(*y)--;
			else
				(*x)++;
		} else if (bamce == 'C' || bamce == 'E')
			(*x)++;
		else HERE();
	} else if (wire == W_SW4 || wire == W_SW2) {
		if (bamce == 'B' || bamce == 'A')
			(*y)++;
		else if (bamce == 'M') {
			if (is_aty(Y_INNER_TOP, model, *y))
				(*y)++;
			else
				(*x)--;
		} else if (bamce == 'C' || bamce == 'E')
			(*x)--;
		else HERE();
	} else if (wire == W_SE4 || wire == W_SE2) {
		if (bamce == 'B' || bamce == 'A')
			(*y)++;
		else if (bamce == 'M') {
			if (is_aty(Y_INNER_TOP, model, *y))
				(*y)++;
			else
				(*x)++;
		} else if (bamce == 'C' || bamce == 'E')
			(*x)++;
		else HERE();
	} else HERE();
}

static int set_BAMCE_point(struct fpga_model *model, struct w_net *net,
	enum wire_type wire, char bamce, int num_0to3, int *cur_y, int *cur_x)
{
	int y_dist_to_dev, row_pos, i;

	while (1) {
		net->pt[net->num_pts].start_count = 0;
		net->pt[net->num_pts].y = *cur_y;
		net->pt[net->num_pts].x = *cur_x;
		if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL, model, *cur_x)) {
			row_pos = regular_row_pos(*cur_y, model);
			if (row_pos == -1) {
				net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
			} else {
				y_dist_to_dev = 3-(row_pos%4);
				if (y_dist_to_dev) {
					net->pt[net->num_pts].y += y_dist_to_dev;
					net->pt[net->num_pts].name = pf("%s%c%i_%i", wire_base(wire), bamce, num_0to3, y_dist_to_dev);
				} else
					net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
			}
		} else if (is_atx(X_CENTER_CMTPLL_COL, model, *cur_x)) {
			row_pos = regular_row_pos(*cur_y, model);
			if (row_pos == -1)
				net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
			else {
				y_dist_to_dev = 7-row_pos;
				if (y_dist_to_dev > 0) {
					net->pt[net->num_pts].y += y_dist_to_dev;
					net->pt[net->num_pts].name = pf("%s%c%i_%i", wire_base(wire), bamce, num_0to3, y_dist_to_dev);
				} else if (!y_dist_to_dev)
					net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
				else { // y_dist_to_dev < 0
					net->pt[net->num_pts].y += y_dist_to_dev - /*hclk*/ 1;
					net->pt[net->num_pts].name = pf("%s%c%i_%i", wire_base(wire), bamce, num_0to3, 16+y_dist_to_dev);
				}
			}
		} else if (is_atx(X_LEFT_MCB|X_RIGHT_MCB, model, *cur_x)) {
			if (*cur_y > model->xci->mcb_ypos-6 && *cur_y <= model->xci->mcb_ypos+6 ) {
				net->pt[net->num_pts].y = model->xci->mcb_ypos;
				net->pt[net->num_pts].name = pf("%s%c%i_%i", wire_base(wire), bamce, num_0to3, model->xci->mcb_ypos+6-*cur_y);
			} else {
				for (i = 0; i < model->xci->num_mui; i++) {
					if (*cur_y == model->xci->mui_pos[i]) {
						net->pt[net->num_pts].y++;
						net->pt[net->num_pts].name = pf("%s%c%i_1", wire_base(wire), bamce, num_0to3);
						break;
					}
					if (*cur_y == model->xci->mui_pos[i]+1) {
						net->pt[net->num_pts].name = pf("%s%c%i_0", wire_base(wire), bamce, num_0to3);
						break;
					}
				}
				if (i >= model->xci->num_mui)
					net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
			}
		} else {
			if (is_atx(X_INNER_LEFT, model, *cur_x) && wire == W_SE2 && bamce == 'E' && num_0to3 == 3)
				net->pt[net->num_pts].name = "SE2M3";
			else
				net->pt[net->num_pts].name = pf("%s%c%i", wire_base(wire), bamce, num_0to3);
		}
		net->num_pts++;

		if (bamce == 'E') {
			if (is_atyx(YX_ROUTING_TILE, model, *cur_y, *cur_x)) {
				 if ((wire == W_EL1 || wire == W_NE2 || wire == W_NW4 || wire == W_NW2 || wire == W_WW4 || wire == W_WR1) && num_0to3 == 0) {
					// add _S0
					net->pt[net->num_pts].start_count = 0;
					net->pt[net->num_pts].y = (*cur_y)+1;
					net->pt[net->num_pts].x = *cur_x;
					if (is_aty(Y_INNER_BOTTOM, model, (*cur_y)+1)) {
						if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, *cur_x)) {
							net->pt[net->num_pts].name = pf("%sE0", wire_base(wire));
							net->num_pts++;
						}
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, (*cur_y)+1)) {
						net->pt[net->num_pts].name = pf("%sE0", wire_base(wire));
						net->num_pts++;
						net->pt[net->num_pts].start_count = 0;
						net->pt[net->num_pts].y = (*cur_y)+2;
						net->pt[net->num_pts].x = *cur_x;
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
					} else {
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
					}
				} else if ((wire == W_NN2 || wire == W_NL1) && num_0to3 == 0) {
					// add _S0
					net->pt[net->num_pts].start_count = 0;
					net->pt[net->num_pts].y = (*cur_y)+1;
					net->pt[net->num_pts].x = *cur_x;
					if (is_aty(Y_INNER_BOTTOM, model, (*cur_y)+1)) {
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, (*cur_y)+1)) {
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
						net->pt[net->num_pts].start_count = 0;
						net->pt[net->num_pts].y = (*cur_y)+2;
						net->pt[net->num_pts].x = *cur_x;
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
					} else {
						net->pt[net->num_pts].name = pf("%sE_S0", wire_base(wire));
						net->num_pts++;
					}
				} else if ((wire == W_SW4 || wire == W_SW2 || wire == W_ER1 || wire == W_WW2 || wire == W_WL1) && num_0to3 == 3) {
					// add _N3
					net->pt[net->num_pts].start_count = 0;
					net->pt[net->num_pts].y = (*cur_y)-1;
					net->pt[net->num_pts].x = *cur_x;
					if (is_aty(Y_INNER_TOP, model, (*cur_y)-1)) {
						net->pt[net->num_pts].name = pf("%sE3", wire_base(wire));
						net->num_pts++;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, (*cur_y)-1)) {
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
						net->pt[net->num_pts].start_count = 0;
						net->pt[net->num_pts].y = (*cur_y)-2;
						net->pt[net->num_pts].x = *cur_x;
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
					} else {
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
					}
				} else if ((wire == W_SS4 || wire == W_SS2 || wire == W_SR1) && num_0to3 == 3) {
					// add _N3
					net->pt[net->num_pts].start_count = 0;
					net->pt[net->num_pts].y = (*cur_y)-1;
					net->pt[net->num_pts].x = *cur_x;
					if (is_aty(Y_INNER_TOP, model, (*cur_y)-1)) {
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, (*cur_y)-1)) {
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
						net->pt[net->num_pts].start_count = 0;
						net->pt[net->num_pts].y = (*cur_y)-2;
						net->pt[net->num_pts].x = *cur_x;
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
					} else {
						net->pt[net->num_pts].name = pf("%sE_N3", wire_base(wire));
						net->num_pts++;
					}
				}
				break;
			}
			dirwire_next_hop(model, wire, bamce, cur_y, cur_x);
		} else {
			int prior_x_major = model->x_major[*cur_x];
			int prior_y_regular = y_to_regular(model, *cur_y);
			dirwire_next_hop(model, wire, bamce, cur_y, cur_x);
			if (net_includes_routing(model, net)) {
				if (prior_x_major != model->x_major[*cur_x])
					break;
				if (prior_y_regular != y_to_regular(model, *cur_y))
					break;
			}
		}
		if (is_atyx(YX_OUTER_TERM, model, *cur_y, *cur_x))
			break;
	}
	return 0;
}

static int run_dirwire(struct fpga_model *model, int start_y, int start_x,
	enum wire_type wire, char bamce_start, int num_0to3)
{
	struct w_net net;
	int cur_y, cur_x, outer_term_hit, rc;
	char cur_bamce;

	net.last_inc = 0;
	net.num_pts = 0;
	cur_y = start_y;
	cur_x = start_x;
	cur_bamce = bamce_start;
	while (1) {
		rc = set_BAMCE_point(model, &net, wire, cur_bamce, num_0to3, &cur_y, &cur_x);
		if (rc) FAIL(rc);

		outer_term_hit = is_atyx(YX_OUTER_TERM, model, cur_y, cur_x);

		if (cur_bamce == 'E' || outer_term_hit) {
			if (net.num_pts < 2) FAIL(EINVAL);
			if ((rc = add_conn_net(model, PREF_BI_F, &net))) FAIL(rc);
			net.num_pts = 0;
			if (outer_term_hit)
				break;
			if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, cur_x)
			    && ((wire == W_SS4 && cur_y >= model->y_height-BOT_INNER_ROW-4)
			        || ((wire == W_SS2 || wire == W_SE4 || wire == W_SW4) && cur_y >= model->y_height-BOT_INNER_ROW-2)
			        || ((wire == W_SL1 || wire == W_SR1 || wire == W_SE2 || wire == W_SW2) && cur_y >= model->y_height-BOT_INNER_ROW-1)))
				break;
		}
		cur_bamce = dirwire_next_bamce(wire, cur_bamce);
	}
	return 0;
fail:
	return rc;
}

static int run_dirwire_0to3(struct fpga_model *model, int start_y, int start_x,
	enum wire_type wire, char bamce_start)
{
	int i, rc;
	for (i = 0; i <= 3; i++) {
		rc = run_dirwire(model, start_y, start_x, wire, bamce_start, i);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int run_dirwires(struct fpga_model* model)
{
	int y, x, i, rc;

	//
	// A EE4-wire goes through the chip from left to right like this:
	//
	// E/B  A   M   C  E/B  A   M   C  E/B
	//  C  E/B  A   M   C  E/B  A   M   C
	//  M   C  E/B  A   M   C  E/B  A   M
	//  A   M   C  E/B  A   M   C  E/B  A
	//
	// set_BAMCE_point() adds net entries for one such point, run_dirwire()
	// runs one wire through the chip, run_dirwires() goes through
	// all rows vertically.
	//

	//
	// EE4, EE2, EL1, ER1
	// WW4, WW2, WL1, WR1
	//

	for (y = TOP_OUTER_IO; y <= model->y_height-BOT_OUTER_IO; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
			continue;
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE4, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE4, 'A');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE2, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EE2, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_EL1, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_ER1, 'E');
		if (rc) FAIL(rc);

		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW4, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW4, 'A');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW2, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WW2, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WL1, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_WR1, 'E');
		if (rc) FAIL(rc);
	}
	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "WW4E_S0",
			TOP_FIRST_REGULAR, x, "WW4E_S0"))) FAIL(rc);
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "EL1E_S0",
			TOP_FIRST_REGULAR, x, "EL1E_S0"))) FAIL(rc);
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "WR1E_S0",
			TOP_FIRST_REGULAR, x, "WR1E_S0"))) FAIL(rc);

		if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "WW2E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "WW2E_N3"))) FAIL(rc);
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "ER1E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "ER1E_N3"))) FAIL(rc);
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "WL1E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "WL1E_N3"))) FAIL(rc);
		}
	}

	//
	// NN4, NN2, NL1, NR1
	// SS4, SS2, SL1, SR1
	//

	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;

		if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			for (i = 0; i <= 3; i++) {
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1-i, x, W_NN4, 'B');
				if (rc) FAIL(rc);
			}
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, x, W_NN2, 'B');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-2, x, W_NN2, 'B');
			if (rc) FAIL(rc);
			if ((rc = add_conn_bi_pref(model,
				model->y_height-BOT_INNER_ROW-2, x, "NN2E0",
				model->y_height-BOT_INNER_ROW-1, x, "NN2E_S0"))) FAIL(rc);

			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, x, W_NL1, 'B');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, x, W_NR1, 'B');
			if (rc) FAIL(rc);

		} else {
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN4, 'E');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN4, 'C');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN4, 'M');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN4, 'A');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN2, 'E');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NN2, 'M');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NL1, 'E');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NR1, 'E');
			if (rc) FAIL(rc);
		}

		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS4, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS4, 'A');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS2, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SS2, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SL1, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SR1, 'E');
		if (rc) FAIL(rc);
	}

	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "NN2E_S0",
			TOP_FIRST_REGULAR, x, "NN2E_S0"))) FAIL(rc);
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "NL1E_S0",
			TOP_FIRST_REGULAR, x, "NL1E_S0"))) FAIL(rc);
		if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "SS4E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "SS4E_N3"))) FAIL(rc);
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "SS2E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "SS2E_N3"))) FAIL(rc);
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "SR1E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "SR1E_N3"))) FAIL(rc);
		}
	}

	//
	// SE4, SE2, SW4, SW2
	//

	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SE4, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SE4, 'A');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SE2, 'M');
		if (rc) FAIL(rc);

		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SW4, 'M');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SW4, 'A');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, TOP_INNER_ROW, x, W_SW2, 'M');
		if (rc) FAIL(rc);

		if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "SW4E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "SW4E_N3"))) FAIL(rc);
			if ((rc = add_conn_bi_pref(model, model->y_height-BOT_INNER_ROW, x, "SW2E_N3",
				model->y_height-BOT_LAST_REGULAR_O, x, "SW2E_N3"))) FAIL(rc);
		}
	}
	for (y = TOP_OUTER_IO; y <= model->y_height-BOT_OUTER_IO; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
			continue;
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_SE4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_SE4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_SE2, 'E');
		if (rc) FAIL(rc);

		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_SW4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_SW4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_SW2, 'E');
		if (rc) FAIL(rc);
	}

	//
	// NE4, NE2
	//

	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;

		if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			// NE2B is one major right, NE4B is two majors right
			int plus_one_major, plus_two_majors;
			plus_one_major = x+1;
			while (plus_one_major != -1
			       && !is_atx(X_ROUTING_COL, model, plus_one_major)) {
				if (++plus_one_major >= model->x_width)
					plus_one_major = -1;
			}
			if (plus_one_major == -1)
				plus_two_majors = -1;
			else {
				plus_two_majors = plus_one_major + 1;
				while (plus_two_majors != -1
				       && !is_atx(X_ROUTING_COL, model, plus_two_majors)) {
					if (++plus_two_majors >= model->x_width)
						plus_two_majors = -1;
				}
			}
			if (plus_two_majors != -1) {
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, plus_two_majors, W_NE4, 'B');
				if (rc) FAIL(rc);
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-2, plus_two_majors, W_NE4, 'B');
				if (rc) FAIL(rc);
			}
			if (plus_one_major != -1) {
				struct w_net net;

				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, plus_one_major, W_NE2, 'B');
				if (rc) FAIL(rc);

				net.last_inc = 2;
				net.num_pts = 0;
				for (i = x; i <= plus_one_major; i++) {
					net.pt[net.num_pts].start_count = 1;
					net.pt[net.num_pts].y = model->y_height-BOT_INNER_ROW-1;
					net.pt[net.num_pts].x = i;
					net.pt[net.num_pts].name = (i == plus_one_major) ? "NE2E%i" : "NE2M%i";
					net.num_pts++;
				}
				rc = add_conn_net(model, PREF_BI_F, &net);
				if (rc) FAIL(rc);

				if ((rc = add_conn_bi_pref(model,
					model->y_height-BOT_INNER_ROW-1, plus_one_major, "NE2E0",
					model->y_height-BOT_INNER_ROW, plus_one_major, "NE2E0"))) FAIL(rc);
			}
		} else {
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NE4, 'M');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NE4, 'A');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NE2, 'M');
			if (rc) FAIL(rc);
		}
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "NE2E_S0",
			TOP_FIRST_REGULAR, x, "NE2E_S0"))) FAIL(rc);
	}
	for (y = TOP_OUTER_IO; y <= model->y_height-BOT_OUTER_IO; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
			continue;
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_NE4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_NE4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, LEFT_INNER_COL, W_NE2, 'E');
		if (rc) FAIL(rc);
	}

	//
	// NW4, NW2
	//

	for (x = LEFT_IO_ROUTING; x <= model->x_width-RIGHT_IO_ROUTING_O; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;

		if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			// NW2B is one major left, NW4B is two majors left
			int minus_one_major, minus_two_majors;
			minus_one_major = x-1;
			while (minus_one_major != -1
			       && !is_atx(X_ROUTING_COL, model, minus_one_major)) {
				if (--minus_one_major < LEFT_IO_ROUTING)
					minus_one_major = -1;
			}
			if (minus_one_major == -1)
				minus_two_majors = -1;
			else {
				minus_two_majors = minus_one_major - 1;
				while (minus_two_majors != -1
				       && !is_atx(X_ROUTING_COL, model, minus_two_majors)) {
					if (--minus_two_majors < LEFT_IO_ROUTING)
						minus_two_majors = -1;
				}
			}
			if (minus_two_majors != -1) {
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, minus_two_majors, W_NW4, 'B');
				if (rc) FAIL(rc);
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-2, minus_two_majors, W_NW4, 'B');
				if (rc) FAIL(rc);

				if ((rc = add_conn_bi_pref(model,
					model->y_height-BOT_INNER_ROW-2, minus_two_majors, "NW4E0",
					model->y_height-BOT_INNER_ROW-1, minus_two_majors, "NW4E_S0"))) FAIL(rc);
				if ((rc = add_conn_bi_pref(model,
					model->y_height-BOT_INNER_ROW-1, minus_two_majors, "NW4E0",
					model->y_height-BOT_INNER_ROW, minus_two_majors, "NW4E0"))) FAIL(rc);
			}
			if (minus_one_major != -1) {
				rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW-1, minus_one_major, W_NW2, 'B');
				if (rc) FAIL(rc);
				if ((rc = add_conn_bi_pref(model,
					model->y_height-BOT_INNER_ROW-1, minus_one_major, "NW2E0",
					model->y_height-BOT_INNER_ROW, minus_one_major, "NW2E0"))) FAIL(rc);
			}
		} else {
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NW4, 'M');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NW4, 'A');
			if (rc) FAIL(rc);
			rc = run_dirwire_0to3(model, model->y_height-BOT_INNER_ROW, x, W_NW2, 'M');
			if (rc) FAIL(rc);
		}
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "NW4E_S0",
			TOP_FIRST_REGULAR, x, "NW4E_S0"))) FAIL(rc);
		if ((rc = add_conn_bi_pref(model, TOP_INNER_ROW, x, "NW2E_S0",
			TOP_FIRST_REGULAR, x, "NW2E_S0"))) FAIL(rc);
	}
	for (y = TOP_OUTER_IO; y <= model->y_height-BOT_OUTER_IO; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
			continue;
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_NW4, 'E');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_NW4, 'C');
		if (rc) FAIL(rc);
		rc = run_dirwire_0to3(model, y, model->x_width-RIGHT_INNER_O, W_NW2, 'E');
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}
