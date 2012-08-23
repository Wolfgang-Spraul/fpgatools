//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

static int run_gclk(struct fpga_model* model);
static int run_gclk_horiz_regs(struct fpga_model* model);
static int run_gclk_vert_regs(struct fpga_model* model);
static int run_logic_inout(struct fpga_model* model);
static int run_term_wires(struct fpga_model* model);
static int run_io_wires(struct fpga_model* model);
static int run_direction_wires(struct fpga_model* model);
static int run_gfan(struct fpga_model* model);
static int connect_clk_sr(struct fpga_model* model, const char* clk_sr);
static int connect_logic_carry(struct fpga_model* model);

int init_conns(struct fpga_model* model)
{
	int rc;

	rc = connect_logic_carry(model);
	if (rc) goto xout;

	rc = connect_clk_sr(model, "CLK");
	if (rc) goto xout;
	rc = connect_clk_sr(model, "SR");
	if (rc) goto xout;

	rc = run_gfan(model);
	if (rc) goto xout;

	rc = run_term_wires(model);
	if (rc) goto xout;

	rc = run_io_wires(model);
	if (rc) goto xout;

	rc = run_direction_wires(model);
	if (rc) goto xout;

	rc = run_logic_inout(model);
	if (rc) goto xout;

	rc = run_gclk(model);
	if (rc) goto xout;

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
							0,
							{{ "M_CIN",		0, y-2, x },
							 { "REGH_CLEXM_COUT",	0, y-1, x },
							 { "M_COUT_N",		0,   y, x },
							 { "" }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y-1)
					    && has_device_type(model, y-2, x, DEV_LOGIC, LOGIC_M)) {
						struct w_net net = {
							0,
							{{ "M_CIN",		0, y-2, x },
							 { "HCLK_CLEXM_COUT",	0, y-1, x },
							 { "M_COUT_N",		0,   y, x },
							 { "" }}};
						if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					} else if (has_device_type(model, y-1, x, DEV_LOGIC, LOGIC_M)) {
						if ((rc = add_conn_bi(model, y, x, "M_COUT_N", y-1, x, "M_CIN"))) goto xout;
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
	int x, y, rc;

	// fabric logic, bram, macc
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_%s%%i", clk_sr), 0))) goto xout;
				if (has_device(model, y, x+2, DEV_BRAM16)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("BRAM_%s%%i_INT3", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("BRAM_%s%%i_INT2", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("BRAM_%s%%i_INT1", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F,   y, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("BRAM_%s%%i_INT0", clk_sr), 0))) goto xout;
				}
				if (has_device(model, y, x+2, DEV_MACC)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("MACC_%s%%i_INT3", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("MACC_%s%%i_INT2", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("MACC_%s%%i_INT1", clk_sr), 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F,   y, x, pf("%s%%i", clk_sr), 0, 1, y, x+2, pf("MACC_%s%%i_INT0", clk_sr), 0))) goto xout;
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
							goto xout;
				} else if (has_device_type(model, y, x+1, DEV_LOGIC, LOGIC_L)) {
					if ((rc = add_conn_range(model,
						NOPREF_BI_F,
						y, x, pf("%s%%i", clk_sr), 0, 1,
						y, x+1, pf("CLEXL_%s%%i", clk_sr), 0)))
							goto xout;
				} else if (has_device(model, y, x+1, DEV_ILOGIC)) {
					if ((rc = add_conn_range(model,
						NOPREF_BI_F,
						y, x, pf("%s%%i", clk_sr), 0, 1,
						y, x+1, pf("IOI_%s%%i", clk_sr), 0)))
							goto xout;
				}
			}
		}
	}
	// center PLLs and DCMs
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y-1, model->center_x-CENTER_ROUTING_O, pf("%s%%i", clk_sr), 0, 1,
		model->center_y-1, model->center_x-CENTER_LOGIC_O, pf("INT_INTERFACE_REGC_%s%%i", clk_sr), 0)))
			goto xout;

	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
			if (has_device(model, y-1, model->center_x-CENTER_CMTPLL_O, DEV_PLL)) {
				{ struct w_net net = {
					1,
					{{ pf("%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { pf("PLL_CLB2_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
				{ struct w_net net = {
					1,
					{{ pf("%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { pf("PLL_CLB1_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
			}
			if (has_device(model, y-1, model->center_x-CENTER_CMTPLL_O, DEV_DCM)) {
				{ struct w_net net = {
					1,
					{{ pf("%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { pf("DCM_CLB2_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
				{ struct w_net net = {
					1,
					{{ pf("%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { pf("INT_INTERFACE_%s%%i", clk_sr), 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { pf("DCM_CLB1_%s%%i", clk_sr), 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
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
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_LOCAL_%s%%i", clk_sr), 0))) goto xout;
		} else if (is_aty(Y_LEFT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("IOI_%s%%i", clk_sr), 0))) goto xout;
		} else {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_%s%%i", clk_sr), 0))) goto xout;
		}

		x = model->x_width - RIGHT_IO_ROUTING_O;
		if (y < TOP_IO_TILES+RIGHT_LOCAL_HEIGHT || y > model->y_height-BOT_IO_TILES-RIGHT_LOCAL_HEIGHT-1) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_LOCAL_%s%%i", clk_sr), 0))) goto xout;
		} else if (is_aty(Y_RIGHT_WIRED, model, y)) {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("IOI_%s%%i", clk_sr), 0))) goto xout;
		} else {
			if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, pf("%s%%i", clk_sr), 0, 1, y, x+1, pf("INT_INTERFACE_%s%%i", clk_sr), 0))) goto xout;
		}
	}
	return 0;
xout:
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
					1,
					{{ "INT_IOI_GFAN%i", 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { "DCM2_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
				{ struct w_net net = {
					1,
					{{ "INT_IOI_GFAN%i", 0, y+1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y+1, model->center_x-CENTER_LOGIC_O },
					 { "DCM1_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
			} else if (YX_TILE(model, y-1, model->center_x-CENTER_CMTPLL_O)->flags & TF_PLL_DEV) {
				struct w_net net = {
					1,
					{{ "INT_IOI_GFAN%i", 0, y-1, model->center_x-CENTER_ROUTING_O },
					 { "INT_INTERFACE_GFAN%i", 0, y-1, model->center_x-CENTER_LOGIC_O },
					 { "PLL_CLB2_GFAN%i", 0, y-1, model->center_x-CENTER_CMTPLL_O },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int pcice_conn(struct fpga_model* model, int y, int x, int i)
{
	static const char* src_str;
	int to_center;

	to_center = (i < x) ^ (x < model->center_x);
	if (is_atx(X_FABRIC_BRAM_COL, model, x))
		src_str = to_center ?
			"BRAM_TTERM_PCICE_OUT" : "BRAM_TTERM_PCICE_IN";
	else if (is_atx(X_FABRIC_MACC_COL, model, x))
		src_str = to_center ?
			"MACCSITE2_TTERM_PCICE_OUT" : "MACCSITE2_TTERM_PCICE_IN";
	else
		EXIT(1);
	return add_conn_bi(model, y, x, src_str, y, i, "BTERM_CLB_PCICE");
}

static int run_term_wires(struct fpga_model* model)
{
	struct w_net net;
	int x, y, i, next_net_o, rightmost_local_net, rc;

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
				3,
				{{ "TTERM_CLB_IOCE%i_S",  0,   y,   x },
				 { "TIOI_IOCE%i",	  0, y+1,   x },
				 { "TIOI_INNER_IOCE%i",	  0, y+2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
			{struct w_net n = {
				3,
				{{ "TTERM_CLB_IOCLK%i_S",  0,   y,   x },
				 { "TIOI_IOCLK%i",	   0, y+1,   x },
				 { "TIOI_INNER_IOCLK%i",   0, y+2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
			{struct w_net n = {
				0,
				{{ "TTERM_CLB_PCICE_S",    0,   y,   x },
				 { "IOI_PCI_CE",	   0, y+1,   x },
				 { "IOI_PCI_CE",	   0, y+2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
			{struct w_net n = {
				1,
				{{ "TTERM_CLB_PLLCE%i_S",     0,   y,   x },
				 { "TIOI_PLLCE%i",	      0, y+1,   x },
				 { "TIOI_INNER_PLLCE%i",      0, y+2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
			{struct w_net n = {
				1,
				{{ "TTERM_CLB_PLLCLK%i_S",    0,   y,   x },
				 { "TIOI_PLLCLK%i",	      0, y+1,   x },
				 { "TIOI_INNER_PLLCLK%i",     0, y+2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
		}

		//
		// bottom
		//

		y = model->y_height - BOT_INNER_ROW;

		// wiring up of IOLOGIC devices:
		if (has_device(model, y-1, x, DEV_ILOGIC)) {

			// IOCE
			{struct w_net n = {
				3,
				{{ "BTERM_CLB_CEOUT%i_N", 0,   y,   x },
				 { "TIOI_IOCE%i",	  0, y-1,   x },
				 { "BIOI_INNER_IOCE%i",	  0, y-2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }

			// IOCLK
			{struct w_net n = {
				3,
				{{ "BTERM_CLB_CLKOUT%i_N", 0,   y,   x },
				 { "TIOI_IOCLK%i",	   0, y-1,   x },
				 { "BIOI_INNER_IOCLK%i",   0, y-2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }

			// PCI_CE
			{struct w_net n = {
				0,
				{{ "BTERM_CLB_PCICE_N",    0,   y,   x },
				 { "IOI_PCI_CE",	   0, y-1,   x },
				 { "IOI_PCI_CE",	   0, y-2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }

			// PLLCE
			{struct w_net n = {
				1,
				{{ "BTERM_CLB_PLLCEOUT%i_N",  0,   y,   x },
				 { "TIOI_PLLCE%i",	      0, y-1,   x },
				 { "BIOI_INNER_PLLCE%i",      0, y-2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }

			// PLLCLK
			{struct w_net n = {
				1,
				{{ "BTERM_CLB_PLLCLKOUT%i_N", 0,   y,   x },
				 { "TIOI_PLLCLK%i",	      0, y-1,   x },
				 { "BIOI_INNER_PLLCLK%i",     0, y-2,   x },
				 { "" }}};
			if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
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
						if (rc) goto xout;
					} else if (x == model->center_x) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 4, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCEOUT%i", 4);
						if (rc) goto xout;
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCEOUT%i", x < model->center_x ? 0 : 4);
						if (rc) goto xout;
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
						if (rc) goto xout;
					} else if (x == model->center_x) {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 4, 7,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCLKOUT%i", 4);
						if (rc) goto xout;
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y-1, model->center_x-CENTER_CMTPLL_O,
							"REGT_IOCLKOUT%i", x < model->center_x ? 0 : 4);
						if (rc) goto xout;
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

			next_net_o = 0;
			// The leftmost and rightmost columns of the fabric area are exempt.
			for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
				EXIT(next_net_o+2 > sizeof(net.pts)/sizeof(net.pts[0]));
				EXIT(!model->tmp_str[x]);
	
				// left and right half separate only for CEOUT and CLKOUT
				if (i < 2 && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					// connect left side
					// left side connects to 0:3, right side to 4:7
					net.pts[next_net_o].start_count = 0;
					net.pts[next_net_o].x = x;
					net.pts[next_net_o].y = TOP_INNER_ROW;
					net.pts[next_net_o].name = model->tmp_str[x];
					net.pts[next_net_o+1].name = "";
					if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					// start right half
					net.pts[0].start_count = 4;
					net.pts[0].x = x;
					net.pts[0].y = TOP_INNER_ROW;
					net.pts[0].name = model->tmp_str[x];
					x++;
					net.pts[1].start_count = 4;
					net.pts[1].x = x;
					net.pts[1].y = TOP_INNER_ROW;
					net.pts[1].name = model->tmp_str[x];
					next_net_o = 2;
				} else {
					net.pts[next_net_o].start_count = 0;
					net.pts[next_net_o].x = x;
					net.pts[next_net_o].y = TOP_INNER_ROW;
					net.pts[next_net_o].name = model->tmp_str[x];
					next_net_o++;
				}
			}
			// connect all (PLL) or just right side
			net.pts[next_net_o].name = "";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
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
						if (rc) goto xout;
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCEOUT%i", x < model->center_x ? 4 : 0);
						if (rc) goto xout;
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
						if (rc) goto xout;
					} else {
						rc = add_conn_range(model, NOPREF_BI_F, y, x,
							model->tmp_str[x], 0, 3,
							y+1, model->center_x-CENTER_CMTPLL_O,
							"REGB_IOCLKOUT%i", x < model->center_x ? 4 : 0);
						if (rc) goto xout;
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

			next_net_o = 0;
			// The leftmost and rightmost columns of the fabric area are exempt.
			for (x = LEFT_SIDE_WIDTH+1; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
				EXIT(next_net_o+2 > sizeof(net.pts)/sizeof(net.pts[0]));
				EXIT(!model->tmp_str[x]);
	
				// left and right half separate only for CEOUT and CLKOUT
				if (i < 2 && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					// connect left side
					// left side connects to 4:7, right side to 0:3
					net.pts[next_net_o].start_count = 4;
					net.pts[next_net_o].x = x;
					net.pts[next_net_o].y = model->y_height - BOT_INNER_ROW;
					net.pts[next_net_o].name = model->tmp_str[x];
					net.pts[next_net_o+1].name = "";
					if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
					// start right half
					next_net_o = 0;
				}
	
				net.pts[next_net_o].start_count = 0;
				net.pts[next_net_o].x = x;
				net.pts[next_net_o].y = model->y_height - BOT_INNER_ROW;
				net.pts[next_net_o].name = model->tmp_str[x];
				next_net_o++;
			}
			// connect all (PLL) or just right side
			net.pts[next_net_o].name = "";
			if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
		}
	}

	//
	// PCICE east-west wiring
	//
	
	// First find BRAM or MACC device columns which are the focal
	// points of the east-west PCICE wiring.
	rightmost_local_net = -1;
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL, model, x)) {
			// From the BRAM/MACC focal points, search left and right
			// for local east-west hubs
			net.last_inc = 0;
			next_net_o = 0;
			for (i = x-1; i >= LEFT_SIDE_WIDTH; i--) {
				if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL|X_CENTER_REGS_COL, model, i))
					break;
				// center logic cannot be found when going left.
				if ((is_atx(X_FABRIC_LOGIC_COL, model, i)
				     && !is_atx(X_ROUTING_NO_IO, model, i-1))
				    || is_atx(X_FABRIC_MACC_VIA_COL, model, i)) {
					rc = pcice_conn(model, y, x, i);
					if (rc) goto xout;
					net.pts[next_net_o].start_count = 0;
					net.pts[next_net_o].x = i;
					net.pts[next_net_o].y = y;
					net.pts[next_net_o].name = "BTERM_CLB_PCICE";
					next_net_o++;
				}
			}
			// TODO: there are more sub-cases here: all points in the
			//       subnet must be connected to the x coords that are
			//       not in the net... and some more cases on left and right
			//       side...
			if (next_net_o && net.pts[0].x > rightmost_local_net) {
				net.pts[next_net_o].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}

			net.last_inc = 0;
			next_net_o = 0;
			for (i = x+1; i < model->x_width - RIGHT_SIDE_WIDTH; i++) {
				if (is_atx(X_FABRIC_BRAM_COL|X_FABRIC_MACC_COL|X_CENTER_CMTPLL_COL, model, i))
					break;
				if ((is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, i)
				     && !is_atx(X_ROUTING_NO_IO, model, i-1))
				    || is_atx(X_FABRIC_MACC_VIA_COL, model, i)) {
					rc = pcice_conn(model, y, x, i);
					if (rc) goto xout;
					net.pts[next_net_o].start_count = 0;
					net.pts[next_net_o].x = i;
					net.pts[next_net_o].y = y;
					net.pts[next_net_o].name = "BTERM_CLB_PCICE";
					next_net_o++;
				}
			}
			if (next_net_o) {
				rightmost_local_net = net.pts[next_net_o-1].x;
				net.pts[next_net_o].name = "";
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
					1,
					{{ pf("TIOB_%s%%i",		s[i]), 0,   y,   x },
					 { pf("IOI_TTERM_IOIUP_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIUP_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i",	s[i]), 0, y+2, x+1 },
					 { "" }}};
				struct w_net net2 = {
					1,
					{{ pf("TIOB_%s%%i",		s[i]), 2,   y,   x },
					 { pf("IOI_TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1,   x },
					 { pf("TTERM_IOIBOT_%s%%i",	s[i]), 0, y+1, x+1 },
					 { pf("TIOI_OUTER_%s%%i_EXT",	s[i]), 0, y+2, x+1 },
					 { pf("TIOI_INNER_%s%%i",	s[i]), 0, y+3, x+1 },
					 { "" }}};

				if ((rc = add_conn_net(model, NOPREF_BI_F, &net1))) goto xout;
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net2))) goto xout;
			}
		}

		y = model->y_height - BOT_OUTER_ROW;
		if (has_device(model, y, x, DEV_IOB)) {
			for (i = 0; s[i][0]; i++) {
				struct w_net net1 = {
					1,
					{{ pf("BIOI_INNER_%s%%i",	s[i]), 0, y-3, x+1 },
					 { pf("BIOI_OUTER_%s%%i_EXT",	s[i]), 0, y-2, x+1 },
					 { pf("BTERM_IOIUP_%s%%i",	s[i]), 0, y-1, x+1 },
					 { pf("IOI_BTERM_IOIUP_%s%%i",	s[i]), 0, y-1,   x },
					 { pf("BIOB_%s%%i",		s[i]), 0,   y,   x },
					 { "" }}};

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
					1,
					{{ pf("LIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("LTERM_IOB_%s%%i",	s[i]), 0, y, x+1 },
					 { pf("LIOI_INT_%s%%i",		s[i]), 0, y, x+2 },
					 { pf("LIOI_IOB_%s%%i",		s[i]), 0, y, x+3 },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
		if (has_device(model, y, model->x_width - RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			x = model->x_width - RIGHT_OUTER_O;
			for (i = 0; s[i][0]; i++) {
				struct w_net net = {
					1,
					{{ pf("RIOB_%s%%i",		s[i]), 0, y,   x },
					 { pf("RTERM_IOB_%s%%i",	s[i]), 0, y, x-1 },
					 { pf("MCB_%s%%i",		s[i]), 0, y, x-2 },
					 { pf("RIOI_IOB_%s%%i",		s[i]), 0, y, x-3 },
					 { "" }}};
				if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int run_gclk(struct fpga_model* model)
{
	int x, i, rc, row, row_top_y, is_break, next_net_o;
	struct w_net gclk_net;

	for (row = model->cfg_rows-1; row >= 0; row--) {
		row_top_y = TOP_IO_TILES + (model->cfg_rows-1-row)*(8+1/* middle of row */+8);
		if (row < (model->cfg_rows/2)) row_top_y++; // center regs

		// net that connects the hclk of half the chip together horizontally
		gclk_net.last_inc = 15;
		next_net_o = 0;
		for (x = LEFT_IO_ROUTING;; x++) {
			if (next_net_o+2 > sizeof(gclk_net.pts) / sizeof(gclk_net.pts[0])) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
			gclk_net.pts[next_net_o].start_count = 0;
			gclk_net.pts[next_net_o].x = x;
			gclk_net.pts[next_net_o].y = row_top_y+8;

			if (is_atx(X_LEFT_IO_ROUTING_COL|X_FABRIC_ROUTING_COL|X_CENTER_ROUTING_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_INT";
			} else if (is_atx(X_LEFT_MCB, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_MCB";
			} else if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL|X_LEFT_IO_DEVS_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_CLB";
			} else if (is_atx(X_FABRIC_BRAM_VIA_COL|X_FABRIC_MACC_VIA_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_BRAM_INTER";
			} else if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_BRAM";
			} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_DSP";
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				gclk_net.pts[next_net_o].y = row_top_y+7;
				gclk_net.pts[next_net_o++].name = "HCLK_CMT_GCLK%i_CLB";
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "CLKV_BUFH_LEFT_L%i";

				// connect left half
				gclk_net.pts[next_net_o].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;

				// start right half
				gclk_net.pts[0].start_count = 0;
				gclk_net.pts[0].x = x;
				gclk_net.pts[0].y = row_top_y+8;
				gclk_net.pts[0].name = "CLKV_BUFH_RIGHT_R%i";
				next_net_o = 1;

			} else if (is_atx(X_RIGHT_IO_ROUTING_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_INT";

				// connect right half
				gclk_net.pts[next_net_o].name = "";
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
					gclk_net.pts[i].name = "GCLK%i";
					gclk_net.pts[i].start_count = 0;
					gclk_net.pts[i].y = row_top_y+i;
					gclk_net.pts[i].x = x;
				}
				gclk_net.last_inc = 15;
				gclk_net.pts[8].name = "";
				if ((rc = add_conn_net(model, NOPREF_BI_F, &gclk_net))) goto xout;
				for (i = 0; i < 8; i++)
					gclk_net.pts[i].y += 9;
				if (is_break)
					gclk_net.pts[7].name = "GCLK%i_BRK";
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
	rc = run_gclk_horiz_regs(model);
	if (rc) goto xout;
	rc = run_gclk_vert_regs(model);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

static int run_gclk_horiz_regs(struct fpga_model* model)
{
	int x, i, rc, left_half;
	int gclk_sep_pos, start1, last1, start2;

	//
	// Run a set of wire strings horizontally through the entire
	// chip from left to right over the center reg row.
	// The wires meet at the middle of each half of the chip on
	// the left and right side, as well as in the center.
	//

	//
	// 1. clk pll 0:1
	//

	{
		struct seed_data clkpll_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLKPLL%i" },
			{ X_INNER_LEFT, 		"REGL_LTERM_CLKPLL%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"INT_CLKPLL%i" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_VIA_COL
			  | X_FABRIC_MACC_VIA_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL%i" },
			{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL%i" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_IO_RT%i" },
			{ X_CENTER_LOGIC_COL, 		"REGC_CLECLKPLL_IO_LT%i" },
			{ X_CENTER_REGS_COL, 		"CLKC_PLL_IO_RT%i" },
			{ X_INNER_RIGHT, 		"REGR_RTERM_CLKPLL%i" },
			{ X_OUTER_RIGHT, 		"REGR_CLKPLL%i" },
			{ 0 }};
	
		left_half = 1;
		seed_strx(model, clkpll_seeds);

		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;

			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				model->tmp_str[x] = left_half
				  ? "REGC_CLKPLL_IO_LT%i"
				  : "REGC_CLKPLL_IO_RT%i";
			if (!model->tmp_str[x])
				continue;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], 0, 1,
				model->center_y,
				left_half ? model->left_gclk_sep_x
					: model->right_gclk_sep_x,
				"INT_CLKPLL%i", 0))) goto xout;
			if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				left_half = 0;
				x--; // wire up cmtpll col on right side as well
			}
		}
	}

	//
	// 2. clk pll lock 0:1
	//

	{
		struct seed_data clkpll_lock_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_LOCKED%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_LOCKED%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"INT_CLKPLL_LOCK%i" },
			{ X_LEFT_IO_DEVS_COL
			  | X_FABRIC_BRAM_VIA_COL
			  | X_FABRIC_MACC_VIA_COL
			  | X_FABRIC_LOGIC_COL
			  | X_RIGHT_IO_DEVS_COL, 	"CLE_CLKPLL_LOCK%i" },
			{ X_FABRIC_MACC_COL, 		"DSP_CLKPLL_LOCK%i" },
			{ X_CENTER_ROUTING_COL, 	"REGC_INT_CLKPLL_LOCK_RT%i" },
			{ X_CENTER_LOGIC_COL,	 	"REGC_CLECLKPLL_LOCK_LT%i" },
			{ X_CENTER_REGS_COL, 		"CLKC_PLL_LOCK_RT%i" },
			{ X_INNER_RIGHT, 		"REGH_RTERM_LOCKED%i" },
			{ X_OUTER_RIGHT, 		"REGR_LOCKED%i" },
			{ 0 }};
	
		left_half = 1;
		seed_strx(model, clkpll_lock_seeds);

		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;

			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				model->tmp_str[x] = left_half
				  ? "CLK_PLL_LOCK_LT%i"
				  : "CLK_PLL_LOCK_RT%i";
			if (!model->tmp_str[x])
				continue;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], 0, 1,
				model->center_y,
				left_half ? model->left_gclk_sep_x
					: model->right_gclk_sep_x,
				"INT_CLKPLL_LOCK%i", 0))) goto xout;
			if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				left_half = 0;
				x--; // wire up cmtpll col on right side as well
			}
		}
	}

	//
	// 3. clk indirect 0:7
	// 4. clk feedback 0:7
	//

	{
		struct seed_data clk_indirect_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLK_INDIRECT%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_CLKINDIRECT%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"REGH_CLKINDIRECT_LR%i_INT" },
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
		struct seed_data clk_feedback_seeds[] = {
			{ X_OUTER_LEFT, 		"REGL_CLK_FEEDBACK%i" },
			{ X_INNER_LEFT, 		"REGH_LTERM_CLKFEEDBACK%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL, 	"REGH_CLKFEEDBACK_LR%i_INT" },
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
		struct seed_data* seeds[2] = {clk_indirect_seeds, clk_feedback_seeds};
		const char* gclk_sep_str[2] = {"REGH_CLKINDIRECT_LR%i_INT", "REGH_CLKFEEDBACK_LR%i_INT"};

		for (i = 0; i <= 1; i++) { // indirect and feedback
			left_half = 1;
			seed_strx(model, seeds[i]);
			for (x = 0; x < model->x_width; x++) {
				if (x == model->left_gclk_sep_x
				    || x == model->right_gclk_sep_x)
					continue;
				if (!model->tmp_str[x])
					continue;

				if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
					start1 = 8;
					last1 = 15;
					start2 = 0;
				} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					if (left_half) {
						start1 = 8;
						last1 = 15;
						start2 = 0;
					} else {
						start1 = 0;
						last1 = 3;
						start2 = 4;
					}
				} else if (is_atx(X_CENTER_REGS_COL|X_INNER_RIGHT|X_OUTER_RIGHT, model, x)) {
					start1 = 0;
					last1 = 3;
					start2 = 4;
				} else {
					start1 = 0;
					last1 = 7;
					start2 = 0;
				}
	
				if ((rc = add_conn_range(model, NOPREF_BI_F,
					model->center_y, x, model->tmp_str[x], start1, last1,
					model->center_y, left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x,
					gclk_sep_str[i], start2))) goto xout;
				if (last1 == 3) {
					if (start1 || start2 != 4) {
						fprintf(stderr, "Internal error in line %i.\n", __LINE__);
						goto xout;
					}
					if ((rc = add_conn_range(model, NOPREF_BI_F,
						model->center_y, x, model->tmp_str[x], 4, 7,
						model->center_y, left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x,
						gclk_sep_str[i], 0))) goto xout;
				}

				if (left_half && is_atx(X_CENTER_CMTPLL_COL, model, x)) {
					left_half = 0;
					x--; // wire up cmtpll col on right side as well
				}
			}
		}
	}

	//
	// 5. ckpin 0:7
	//

	{
		struct seed_data ckpin_seeds[] = {
			{ X_OUTER_LEFT,			"REGL_CKPIN%i" },
			{ X_INNER_LEFT,			"REGH_LTERM_CKPIN%i" },
			{ X_FABRIC_ROUTING_COL
			  | X_LEFT_IO_ROUTING_COL
			  | X_RIGHT_IO_ROUTING_COL,	"REGH_CKPIN%i_INT" },
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
		char* gclk_sep_str;
	
		left_half = 1;
		seed_strx(model, ckpin_seeds);
		for (x = 0; x < model->x_width; x++) {
			if (x == model->left_gclk_sep_x
			    || x == model->right_gclk_sep_x)
				continue;
			if (!model->tmp_str[x])
				continue;
	
			if (is_atx(X_CENTER_ROUTING_COL|X_CENTER_LOGIC_COL|X_CENTER_CMTPLL_COL, model, x))
				start1 = 8;
			else if (is_atx(X_CENTER_REGS_COL, model, x))
				start1 = left_half ? 8 : 0;
			else
				start1 = 0;
	
			gclk_sep_pos = left_half ? model->left_gclk_sep_x : model->right_gclk_sep_x;
			gclk_sep_str = ((x > gclk_sep_pos) ^ left_half) ? "REGH_DSP_IN_CKPIN%i" : "REGH_DSP_OUT_CKPIN%i";
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y, x, model->tmp_str[x], start1, start1+7,
				model->center_y, gclk_sep_pos,
				gclk_sep_str, 0))) goto xout;
	
			// In this loop we tie around the CENTER_REGS_COL, not the
			// CENTER_CMTPLL_COL as before.
			if (left_half && is_atx(X_CENTER_REGS_COL, model, x)) {
				left_half = 0;
				x--; // wire up center regs col on right side as well
			}
		}
	}
	// some local nets around the center on the left side
	{ struct w_net net = {
		3,
		{{ "REGL_GCLK%i", 0, model->center_y, LEFT_OUTER_COL },
		 { "REGH_LTERM_GCLK%i", 0, model->center_y, LEFT_INNER_COL },
		 { "REGH_IOI_INT_GCLK%i", 0, model->center_y, LEFT_IO_ROUTING },
		 { "" }}};
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
		3,
		{{ "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "MCB_REGH_GCLK%i", 0, model->center_y, model->x_width-RIGHT_MCB_O },
		 { "REGR_GCLK%i", 0, model->center_y, model->x_width-RIGHT_OUTER_O },
		 { "" }}};
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
		1,
		{{ "INT_BUFPLL_GCLK%i", 0, model->center_y-1, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_INT_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 0, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGH_RTERM_GCLK%i", 0, model->center_y, model->x_width-RIGHT_INNER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	{ struct w_net net = {
		1,
		{{ "REGH_IOI_INT_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 2, model->center_y, model->x_width-RIGHT_IO_DEVS_O },
		 { "REGR_RTERM_GCLK%i", 2, model->center_y, model->x_width-RIGHT_INNER_O },
		 { "" }}};
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
	for (i = 0; i <= 17; i++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+10))
			net.pts[i].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (i == 9) // row 19
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+10;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 19..53
	net.last_inc = 15;
	for (i = 0; i <= 34; i++) { // row 19..53
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+19))
			net.pts[i].name = "REGV_GCLKH_MAIN%i";
		else if (is_aty(Y_CHIP_HORIZ_REGS, model, i+19))
			net.pts[i].name = "CLKC_GCLK_MAIN%i";
		else if (i == 16) // row 35
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BRK";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+19;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// net tying together 15 gclk lines from row 45..62
	net.last_inc = 15;
	for (i = 0; i <= 17; i++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, i+45))
			net.pts[i].name = "CLKV_GCLKH_MAIN%i_FOLD";
		else if (i == 8) // row 53
			net.pts[i].name = "CLKV_GCLK_MAIN%i_BUF";
		else
			net.pts[i].name = "CLKV_GCLK_MAIN%i_FOLD";
		net.pts[i].start_count = 0;
		net.pts[i].y = i+45;
		net.pts[i].x = model->center_x;
	}
	net.pts[i].name = "";
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout;

	// a few local gclk networks at the center top and bottom
	{ struct w_net n = {
		1,
		{{ "REGT_GCLK%i",	0, TOP_OUTER_ROW, model->center_x-1 },
		 { "REGT_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x-1 },
		 { "REGV_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x },
		 { "BUFPLL_TOP_GCLK%i", 0, TOP_INNER_ROW, model->center_x+1 },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &n))) goto xout; }
	{ struct w_net n = {
		1,
		{{ "REGB_GCLK%i",	0, model->y_height-1, model->center_x-1 },
		 { "REGB_BTERM_GCLK%i", 0, model->y_height-2, model->center_x-1 },
		 { "REGV_BTERM_GCLK%i", 0, model->y_height-2, model->center_x },
		 { "BUFPLL_BOT_GCLK%i", 0, model->y_height-2, model->center_x+1 },
		 { "" }}};
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
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXM_LOGICOUT%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXL_LOGICOUT%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) goto xout;
				}
			}
		}
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL|X_FABRIC_MACC_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
				if (YX_TILE(model, y, x)[2].flags & TF_BRAM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT3", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT2", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT1", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "BRAM_LOGICOUT%i_INT0", 0))) goto xout;
				}
				if (YX_TILE(model, y, x)[2].flags & TF_MACC_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-3, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT3", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-2, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT2", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT1", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y, x+2, "MACC_LOGICOUT%i_INT0", 0))) goto xout;
				}
			}
		}
		if (is_atx(X_CENTER_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x, "LOGICOUT%i", 0, 23, y+1, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
					if (YX_TILE(model, y-1, x+2)->flags & TF_DCM_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB2_LOGICOUT%i", 0))) goto xout;
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "DCM_CLB1_LOGICOUT%i", 0))) goto xout;
					} else if (YX_TILE(model, y-1, x+2)->flags & TF_PLL_DEV) {
						if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB2_LOGICOUT%i", 0))) goto xout;
						if ((rc = add_conn_range(model, NOPREF_BI_F, y+1, x+1, "INT_INTERFACE_LOGICOUT_%i", 0, 23, y-1, x+2, "PLL_CLB1_LOGICOUT%i", 0))) goto xout;
					}
				}
				if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y-1, x, "LOGICOUT%i", 0, 23, y-1, x+1, "INT_INTERFACE_REGC_LOGICOUT%i", 0))) goto xout;
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
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOCAL_LOGICOUT%i", 0))) goto xout;
				} else if (is_aty(wired_side, model, y)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) goto xout;
				} else {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "INT_INTERFACE_LOGICOUT%i", 0))) goto xout;
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
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB2_LOGICINB%i", 11))) goto xout;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "DCM_CLB1_LOGICINB%i", 11))) goto xout;
		} else { // PLL
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  0,  3,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 0))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN4",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB4"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i",  5,  9,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 5))) goto xout;
			if ((rc = add_conn_bi(model,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_IOI_LOGICBIN10",
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB10"))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y-1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 11, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB2_LOGICINB%i", 11))) goto xout;

			if ((rc = add_conn_range(model, NOPREF_BI_F,
				y+1, model->center_x-CENTER_LOGIC_O, "INT_INTERFACE_LOGICBIN%i", 0, 62,
				y-1, model->center_x-CENTER_CMTPLL_O, "PLL_CLB1_LOGICINB%i", 0))) goto xout;
		}
	}

	for (y = 0; y < model->y_height; y++) {
		for (x = 0; x < model->x_width; x++) {
			tile = &model->tiles[y * model->x_width + x];

			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				static const int north_p[4] = {21, 28, 52, 60};
				static const int south_p[4] = {20, 36, 44, 62};

				for (i = 0; i < sizeof(north_p)/sizeof(north_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN%i", north_p[i])))) goto xout;
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-1, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model, y-1, x, pf("LOGICIN_N%i", north_p[i]), y-2, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN_N%i", north_p[i]), y+1, x, pf("LOGICIN_N%i", north_p[i])))) goto xout;
					}
				}
				for (i = 0; i < sizeof(south_p)/sizeof(south_p[0]); i++) {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN_S%i", south_p[i]), y-1, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					}
					if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
						if ((rc = add_conn_bi_pref(model, y+1, x, pf("LOGICIN%i", south_p[i]), y+2, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x))
							if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN%i", south_p[i])))) goto xout;
					} else {
						if ((rc = add_conn_bi_pref(model, y, x, pf("LOGICIN%i", south_p[i]), y+1, x, pf("LOGICIN_S%i", south_p[i])))) goto xout;
					}
				}

				if (tile[1].flags & TF_LOGIC_XM_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "CLEXM_LOGICIN_B%i", 0))) goto xout;
				}
				if (tile[1].flags & TF_LOGIC_XL_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i",  0, 35, y, x+1, "CLEXL_LOGICIN_B%i",  0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 37, 43, y, x+1, "CLEXL_LOGICIN_B%i", 37))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 45, 52, y, x+1, "CLEXL_LOGICIN_B%i", 45))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 54, 60, y, x+1, "CLEXL_LOGICIN_B%i", 54))) goto xout;
				}
				if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 3, y, x+1, "IOI_LOGICINB%i", 0))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 5, 9, y, x+1, "IOI_LOGICINB%i", 5))) goto xout;
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 11, 62, y, x+1, "IOI_LOGICINB%i", 11))) goto xout;
				}
				if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
					if (tile[2].flags & TF_BRAM_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "BRAM_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) goto xout;
						}
					}
				}
				if (is_atx(X_FABRIC_MACC_ROUTING_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
					if (tile[2].flags & TF_MACC_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "MACC_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) goto xout;
						}
					}
				}
				if (is_atx(X_CENTER_REGS_COL, model, x+3)) {
					if (tile[2].flags & (TF_PLL_DEV|TF_DCM_DEV)) {
						const char* prefix = (tile[2].flags & TF_PLL_DEV) ? "PLL_CLB2" : "DCM_CLB2";

						for (i = 0;; i = 2) {
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN4"))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 5))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y+i, x+1, "INT_INTERFACE_IOI_LOGICBIN10"))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y+i, x+1, "INT_INTERFACE_LOGICBIN%i", 11))) goto xout;

							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  0,  3,   y, x+2, pf("%s_LOGICINB%%i", prefix), 0))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B4", y, x+2, pf("%s_LOGICINB4", prefix)))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i",  5,  9,   y, x+2, pf("%s_LOGICINB%%i", prefix), 5))) goto xout;
							if ((rc = add_conn_bi(model,   y+i, x, "INT_IOI_LOGICIN_B10", y, x+2, pf("%s_LOGICINB10", prefix)))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F,   y+i, x, "LOGICIN_B%i", 11, 62,   y, x+2, pf("%s_LOGICINB%%i", prefix), 11))) goto xout;

							if (tile[2].flags & TF_PLL_DEV) {
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62, y+2, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
								if ((rc = add_conn_range(model, NOPREF_BI_F, y+2, x, "LOGICIN_B%i",  0, 62,   y, x+2, "PLL_CLB1_LOGICINB%i", 0))) goto xout;
								break;
							}
							if (i == 2) break;
							prefix = "DCM_CLB1";
						}
					}
					if (is_aty(Y_CHIP_HORIZ_REGS, model, y+1)) {
						if ((rc = add_conn_range(model, NOPREF_BI_F,   y, x, "LOGICIN_B%i",  0, 62,   y, x+1, "INT_INTERFACE_REGC_LOGICBIN%i", 0))) goto xout;
						int clk_pins[16] = { 24, 15, 7, 42, 5, 12, 62, 16, 47, 20, 38, 23, 48, 57, 44, 4 };
						for (i = 0; i <= 15; i++) {
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+1, pf("REGC_CLE_SEL%i", i)))) goto xout;
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+2, pf("REGC_CMT_SEL%i", i)))) goto xout;
							if ((rc = add_conn_bi(model,   y, x, pf("LOGICIN_B%i", clk_pins[i]), y+1, x+3, pf("CLKC_SEL%i_PLL", i)))) goto xout;
						}
					}
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

static const char* s_4wire = "BAMCE";

static int wire_SS4E_N3(struct fpga_model* model, const struct w_net* net)
{
	int i, j, rc, e_y, e_x, extra_n3;

	for (i = 0; net->pts[i].name[0] != 0; i++);
	if (!i || net->pts[i-1].name[3] != 'E') return 0;

	// i-1 is 'E', i-2 is 'C' which if it's double
	// because of HCLK is also in i-3
	e_y = net->pts[i-1].y;
	e_x = net->pts[i-1].x;
	if (e_y == BOT_TERM(model)-1
	    && !is_atx(X_FABRIC_BRAM_ROUTING_COL, model, e_x))
		if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E_N3", e_y+1, e_x, "SS4E_N3"))) goto xout;
	if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E3", e_y-1, e_x, "SS4E_N3"))) goto xout;
	if (pos_in_row(e_y-1, model) == HCLK_POS
	    || IS_CENTER_Y(e_y-1, model)) {
		if ((rc = add_conn_bi_pref(model, e_y, e_x, "SS4E3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-1, e_x, "SS4E_N3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-1, e_x, "SS4C3", e_y-2, e_x, "SS4E_N3"))) goto xout;
		if ((rc = add_conn_bi_pref(model, e_y-2, e_x, "SS4C3", e_y-1, e_x, "SS4E_N3"))) goto xout;
		extra_n3 = 1;
		j = i-4;
	} else {
		extra_n3 = 0;
		j = i-3;
	}
	for (; j >= 0; j--) {
		if ((rc = add_conn_bi_pref(model, net->pts[j].y, e_x, pf("%.4s3", net->pts[j].name), e_y-1, e_x, "SS4E_N3"))) goto xout;
		if (extra_n3)
			if ((rc = add_conn_bi_pref(model, net->pts[j].y, e_x, pf("%.4s3", net->pts[j].name), e_y-2, e_x, "SS4E_N3"))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int run_direction_wires(struct fpga_model* model)
{
	int x, y, i, j, _row_num, _row_pos, rc;
	struct w_net net;

	// SS4
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		// some wiring at the top
		net.last_inc = 3;
		for (i = 1; i < 5; i++) { // go through "BAMCE"
			net.pts[0].start_count = 0;
			net.pts[0].y = TOP_TERM(model);
			net.pts[0].x = x;
			net.pts[0].name = pf("SS4%c%%i", s_4wire[i]);
			for (j = i; j < 5; j++) {
				net.pts[j-i+1].start_count = 0;
				net.pts[j-i+1].y = TOP_TERM(model)+(j-i+1);
				net.pts[j-i+1].x = x;
				net.pts[j-i+1].name = pf("SS4%c%%i", s_4wire[j]);
			}
			net.pts[j-i+1].name = "";
			if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			if ((rc = wire_SS4E_N3(model, &net))) goto xout;
		}
		// rest going down to bottom termination
		for (y = 0; y < model->y_height; y++) {
			is_in_row(model, y, &_row_num, &_row_pos);
			if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)
			    && y > BOT_TERM(model)-5)
				break;
			if (_row_pos < 0 || _row_pos == 8)
				continue;

			net.last_inc = 3;
			j = 0;
			for (i = 0; i < 5; i++) { // go through "BAMCE"
				net.pts[j].start_count = 0;
				net.pts[j].y = y+j;
				net.pts[j].x = x;
				if (y+j == BOT_TERM(model)) {
					EXIT(!i);
					net.pts[j].name = pf("SS4%c%%i", s_4wire[i-1]);
					j++;
					break;
				}
				if (IS_CENTER_Y(y+j, model)
				    || pos_in_row(y+j, model) == HCLK_POS) {
					EXIT(!i);
					net.pts[j].name = pf("SS4%c%%i", s_4wire[i-1]);
					j++;
					net.pts[j].start_count = 0;
					net.pts[j].y = y+j;
					net.pts[j].x = x;
				}
				net.pts[j].name = pf("SS4%c%%i", s_4wire[i]);
				j++;
			}
			net.pts[j].name = "";
			if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			if ((rc = wire_SS4E_N3(model, &net))) goto xout;
		}
	}

	// NN4
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = 0; y < model->y_height; y++) {
			is_in_row(model, y, &_row_num, &_row_pos);
			if (_row_pos >= 0 && _row_pos != 8) {
				net.last_inc = 3;
				j = 0;
				for (i = 0; i < 5; i++) { // go through "BAMCE"
					net.pts[j].start_count = 0;
					net.pts[j].y = y-j;
					net.pts[j].x = x;
					if (y-j == TOP_INNER_ROW) {
						EXIT(!i);
						net.pts[j].name = pf("NN4%c%%i", s_4wire[i-1]);
						j++;
						break;
					}
					net.pts[j].name = pf("NN4%c%%i", s_4wire[i]);
					if (IS_CENTER_Y(y-j, model)
					    || pos_in_row(y-j, model) == HCLK_POS) {
						EXIT(!i);
						i--;
					}
					j++;
				}
				net.pts[j].name = "";
				if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			}
		}
		if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
			net.last_inc = 3;
			for (i = 1; i < 5; i++) { // go through "BAMCE"
				net.pts[0].start_count = 0;
				net.pts[0].y = BOT_TERM(model);
				net.pts[0].x = x;
				net.pts[0].name = pf("NN4%c%%i", s_4wire[i]);
				for (j = i; j < 5; j++) {
					net.pts[j-i+1].start_count = 0;
					net.pts[j-i+1].y = BOT_TERM(model)-(j-i+1);
					net.pts[j-i+1].x = x;
					net.pts[j-i+1].name = pf("NN4%c%%i", s_4wire[j]);
				}
				net.pts[j-i+1].name = "";
				if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout;
			}
		}
	}

	for (y = 0; y < model->y_height; y++) {
		for (x = 0; x < model->x_width; x++) {
			// NR1
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net n = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net n = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "NR1E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
				} else {
					{ struct w_net n = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
						{ struct w_net n = {
							3,
							{{ "NR1E%i", 0,   y, x },
							 { "NR1E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					}
				}
			}

			// NN2
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net n = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					{ struct w_net n = {
						0,
						{{ "NN2E_S0", 0,   y, x },
						 { "NN2E_S0", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
				} else if (is_aty(Y_INNER_TOP, model, y-2)) {
					{ struct w_net n = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net n = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi_pref(model, y-1, x, "NN2M0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-2)) {
					{ struct w_net n = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E0",   y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2M0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-3, x, "NN2E0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
				} else {
					{ struct w_net n = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi(model, y-2, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
					if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if ((rc = add_conn_bi(model, y, x, "NN2E_S0", y-1, x, "NN2E0"))) goto xout;
						if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
							{ struct w_net n = {
								3,
								{{ "NN2E%i", 0, y-1, x },
								 { "NN2M%i", 0,   y, x },
								 { "NN2M%i", 0, y+1, x },
								 { "" }}};
							if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
							if ((rc = add_conn_range(model, PREF_BI_F, y, x, "NN2E%i", 0, 3, y+1, x, "NN2E%i", 0))) goto xout;
							if ((rc = add_conn_bi(model, y, x, "NN2E0", y+1, x, "IOI_BTERM_NN2E_S0"))) goto xout;
							if ((rc = add_conn_bi(model, y, x, "NN2E_S0", y+1, x, "IOI_BTERM_NN2M0"))) goto xout;
						}
					}
				}
			}

			// SS2
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+2)) {
					{ struct w_net n = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+1, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2E_N3"))) goto xout;

					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;

					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2M3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2M3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2B3",   y+3, x, "SS2E_N3"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
					{ struct w_net n = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2B%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
				} else if (is_aty(Y_INNER_BOTTOM, model, y+2)) {
					if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
						{ struct w_net n = {
							3,
							{{ "SS2B%i", 0,   y, x },
							 { "SS2M%i", 0, y+1, x },
							 { "SS2M%i", 0, y+2, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					}
				} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
					if (!is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x)) {
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2B%i", 0, 3, y+1, x, "SS2B%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E_N3"))) goto xout;
					}
				} else {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						{ struct w_net n = {
							3,
							{{ "SS2M%i", 0, y-1, x },
							 { "SS2M%i", 0,   y, x },
							 { "SS2E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2E%i", 0, 3, y-1, x, "SS2E%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E3",   y-1, x, "SS2E_N3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y-1, x, "SS2M3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E3"))) goto xout;
					}
					{ struct w_net n = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2E%i", 0, y+2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &n))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+1, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2E3"))) goto xout;
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}
