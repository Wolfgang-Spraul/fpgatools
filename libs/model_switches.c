//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"
#include "parts.h"

static int init_ce_clk(struct fpga_model *model);
static int init_io(struct fpga_model *model);
static int init_routing(struct fpga_model *model);
static int init_north_south_dirwire_term(struct fpga_model *model);
static int init_east_west_dirwire_term(struct fpga_model *model);
static int init_iologic(struct fpga_model *model);
static int init_logic(struct fpga_model *model);
static int init_center(struct fpga_model *model);
static int init_hclk(struct fpga_model *model);
static int init_logicout_fw(struct fpga_model *model);
static int init_bram(struct fpga_model *model);
static int init_macc(struct fpga_model *model);
static int init_topbot_tterm_gclk(struct fpga_model *model);
static int init_bufio(struct fpga_model *model);
static int init_bscan(struct fpga_model *model);
static int init_dcm(struct fpga_model *model);
static int init_pll(struct fpga_model *model);
static int init_center_hclk(struct fpga_model *model);
static int init_center_midbuf(struct fpga_model *model);
static int init_center_reg_tblr(struct fpga_model *model);
static int init_center_topbot_cfb_dfb(struct fpga_model *model);

int init_switches(struct fpga_model *model, int routing_sw)
{
	int rc;

	if (routing_sw) {
		rc = init_routing(model);
		if (rc) FAIL(rc);
	}

	rc = init_logic(model);
	if (rc) FAIL(rc);

   	rc = init_iologic(model);
	if (rc) FAIL(rc);

   	rc = init_north_south_dirwire_term(model);
	if (rc) FAIL(rc);

   	rc = init_east_west_dirwire_term(model);
	if (rc) FAIL(rc);

   	rc = init_ce_clk(model);
	if (rc) FAIL(rc);

	rc = init_io(model);
	if (rc) FAIL(rc);

	rc = init_center(model);
	if (rc) FAIL(rc);

	rc = init_hclk(model);
	if (rc) FAIL(rc);

	rc = init_logicout_fw(model);
	if (rc) FAIL(rc);

	rc = init_bram(model);
	if (rc) FAIL(rc);

	rc = init_macc(model);
	if (rc) FAIL(rc);

	rc = init_topbot_tterm_gclk(model);
	if (rc) FAIL(rc);

	rc = init_bufio(model);
	if (rc) FAIL(rc);

	rc = init_bscan(model);
	if (rc) FAIL(rc);

	rc = init_dcm(model);
	if (rc) FAIL(rc);

	rc = init_pll(model);
	if (rc) FAIL(rc);

	rc = init_center_hclk(model);
	if (rc) FAIL(rc);

	rc = init_center_midbuf(model);
	if (rc) FAIL(rc);

	rc = init_center_reg_tblr(model);
	if (rc) FAIL(rc);

	rc = init_center_topbot_cfb_dfb(model);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

static int init_logic_tile(struct fpga_model *model, int y, int x)
{
	int rc, i, j, ml;
	const char* xp;

	if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_M)) {
		ml = 'M';
		xp = "X";
	} else if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_L)) {
		ml = 'L';
		xp = "XX";
	} else EXIT(1);

	if ((rc = add_switch(model, y, x,
		pf("CLEX%c_CLK0", ml), pf("%s_CLK", xp), 0 /* bidir */))) goto xout;
	if ((rc = add_switch(model, y, x,
		pf("CLEX%c_CLK1", ml), pf("%c_CLK", ml), 0 /* bidir */))) goto xout;
	if ((rc = add_switch(model, y, x,
		pf("CLEX%c_SR0", ml), pf("%s_SR", xp), 0 /* bidir */))) goto xout;
	if ((rc = add_switch(model, y, x,
		pf("CLEX%c_SR1", ml), pf("%c_SR", ml), 0 /* bidir */))) goto xout;
	for (i = X_A1; i <= X_DX; i++) {
		if ((rc = add_switch(model,y, x,
			pf("CLEX%c_LOGICIN_B%i", ml, i),
			pf("%s_%s", xp, logicin_str(i)),
			0 /* bidir */))) goto xout;
	}
	for (i = M_A1; i <= M_WE; i++) {
		if (ml == 'L' &&
		    (i == M_AI || i == M_BI || i == M_CI
		     || i == M_DI || i == M_WE))
			continue;
		if ((rc = add_switch(model,y, x,
			pf("CLEX%c_LOGICIN_B%i", ml, i),
			pf("%c_%s", ml, logicin_str(i)),
			0 /* bidir */))) goto xout;
	}
	for (i = X_A; i <= X_DQ; i++) {
		if ((rc = add_switch(model, y, x,
			pf("%s_%s", xp, logicout_str(i)),
			pf("CLEX%c_LOGICOUT%i", ml, i),
			0 /* bidir */))) goto xout;
	}
	for (i = M_A; i <= M_DQ; i++) {
		if ((rc = add_switch(model, y, x,
			pf("%c_%s", ml, logicout_str(i)),
			pf("CLEX%c_LOGICOUT%i", ml, i),
			0 /* bidir */))) goto xout;
	}
	for (i = 'A'; i <= 'D'; i++) {
		for (j = 1; j <= 6; j++) {
			if ((rc = add_switch(model, y, x,
				pf("%c_%c%i", ml, i, j),
				pf("%c_%c", ml, i),
				0 /* bidir */))) goto xout;
			if ((rc = add_switch(model, y, x,
				pf("%s_%c%i", xp, i, j),
				pf("%s_%c", xp, i),
				0 /* bidir */))) goto xout;
		}
		if ((rc = add_switch(model, y, x,
			pf("%c_%c", ml, i),
			pf("%c_%cMUX", ml, i),
			0 /* bidir */))) goto xout;
	}
	if (ml == 'L') {
		if (has_connpt(model, y, x, "XL_COUT_N")) {
			if ((rc = add_switch(model, y, x,
				"XL_COUT", "XL_COUT_N",
				0 /* bidir */))) goto xout;
		}
		if ((rc = add_switch(model, y, x,
			"XL_COUT", "L_DMUX", 0 /* bidir */))) goto xout;
	} else {
		if (has_connpt(model, y, x, "M_COUT_N")) {
			if ((rc = add_switch(model, y, x,
				"M_COUT", "M_COUT_N",
				0 /* bidir */))) goto xout;
		}
		if ((rc = add_switch(model, y, x,
			"M_COUT", "M_DMUX", 0 /* bidir */))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int init_logic(struct fpga_model *model)
{
	int x, y, rc;

	for (x = LEFT_SIDE_WIDTH; x < model->x_width-RIGHT_SIDE_WIDTH; x++) {
		if (!is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (has_device(model, y, x, DEV_LOGIC)) {
				rc = init_logic_tile(model, y, x);
				if (rc) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

static int init_iologic_tile(struct fpga_model *model, int y, int x)
{
	int i, j, rc;
	const char* io_prefix, *prefix, *prefix2;

	if (x < LEFT_SIDE_WIDTH) {
		EXIT(x != LEFT_IO_DEVS);
		io_prefix = "IOI_";
		prefix = "LIOI_";
		prefix2 = "LIOI_IOB_";
	} else if (x >= model->x_width-RIGHT_SIDE_WIDTH) {
		EXIT(x != model->x_width - RIGHT_IO_DEVS_O);
		io_prefix = "RIOI_";
		prefix = "RIOI_";
		prefix2 = "RIOI_IOB_";
	} else {
		if (y == TOP_OUTER_IO) {
			io_prefix = "TIOI_";
			prefix = "TIOI_";
			prefix2 = "TIOI_OUTER_";
		} else if (y == TOP_INNER_IO) {
			io_prefix = "TIOI_INNER_";
			prefix = "TIOI_";
			prefix2 = "TIOI_INNER_";
		} else if (y == model->y_height-BOT_INNER_IO) {
			io_prefix = "BIOI_INNER_";
			prefix = "BIOI_";
			prefix2 = "BIOI_INNER_";
		} else if (y == model->y_height-BOT_OUTER_IO) {
			io_prefix = "TIOI_";
			prefix = "BIOI_";
			prefix2 = "BIOI_OUTER_";
		} else
			EXIT(1);
	}

	for (i = 0; i <= 23; i++) {
		if ((rc = add_switch(model, y, x,
			pf("IOI_INTER_LOGICOUT%i", i),
			pf("IOI_LOGICOUT%i", i), 0 /* bidir */))) goto xout;
	}
	// switches going to IOI_INTER_LOGICOUT0:15
	{ static const char* logicout_src[16] = {
	  /*  0 */ "FABRICOUT_ILOGIC_SITE",
		   "Q1_ILOGIC_SITE", "Q2_ILOGIC_SITE",
		   "Q3_ILOGIC_SITE", "Q4_ILOGIC_SITE",
		   "INCDEC_ILOGIC_SITE", "VALID_ILOGIC_SITE",
	  /*  7 */ "FABRICOUT_ILOGIC_SITE_S",
	 	   "Q1_ILOGIC_SITE_S", "Q2_ILOGIC_SITE_S",
		   "Q3_ILOGIC_SITE_S", "Q4_ILOGIC_SITE_S",
	  /* 12 */ "", "",
	  /* 14 */ "BUSY_IODELAY_SITE", "BUSY_IODELAY_SITE_S" };
	for (i = 0; i < sizeof(logicout_src)/sizeof(logicout_src[0]); i++) {
		if (logicout_src[i][0]) {
			if ((rc = add_switch(model, y, x, logicout_src[i],
				pf("IOI_INTER_LOGICOUT%i", i),
				0 /* bidir */))) goto xout;
		}
	}}
	// The 6 CE lines (4*IO_CE and 2*PLL_CE) can be switched
	// to 4 IOCE destinations. Each IOCE line can be driven
	// by 6 CE lines.
	for (i = 0; i <= 3; i++) {
		for (j = 0; j <= 3; j++) {
			if ((rc = add_switch(model, y, x,
				pf("%sIOCE%i", io_prefix, j),
				pf("IOI_CLKDIST_IOCE%i%s",i/2,i%2?"_M":"_S"),
				0 /* bidir */))) goto xout;
		}
		for (j = 0; j <= 1; j++) {
			if ((rc = add_switch(model, y, x,
				pf("%sPLLCE%i", io_prefix, j),
				pf("IOI_CLKDIST_IOCE%i%s",i/2,i%2?"_M":"_S"),
				0 /* bidir */))) goto xout;
		}
	}
	// Incoming clocks and fan can be switched to intermediates
	// (5 sources per intermediate), and then to the ilogic/ologic
	// devices (3 sources each) or 2*CLK1 (2 sources each).
	for (i = 0; i < 4; i++) {
		if ((rc = add_switch(model, y, x,
			pf("IOI_CLK%i", i/2),
			pf("IOI_CLK%iINTER%s",i%2,i<2?"_M":"_S"),
			0 /* bidir */))) goto xout;
		if ((rc = add_switch(model, y, x,
			pf("IOI_GFAN%i", i/2),
			pf("IOI_CLK%iINTER%s",i%2,i<2?"_M":"_S"),
			0 /* bidir */))) goto xout;
		if ((rc = add_switch(model, y, x,
			pf("%sIOCLK%i", io_prefix, i),
			pf("IOI_CLK%iINTER%s",i%2,i<2?"_M":"_S"),
			0 /* bidir */))) goto xout;
		if ((rc = add_switch(model, y, x,
			pf("%sPLLCLK%i", io_prefix, i/2),
			pf("IOI_CLK%iINTER%s",i/2,i%2?"_M":"_S"),
			0 /* bidir */))) goto xout;
		// only PLLCLK goes to CLK2 intermediate
		if ((rc = add_switch(model, y, x,
			pf("%sPLLCLK%i", io_prefix, i/2),
			pf("IOI_CLK2INTER%s",i%2?"_S":"_M"),
			0 /* bidir */))) goto xout;
		// 2 sources each for IOI_CLKDIST_CLK1_M/_S
		if ((rc = add_switch(model, y, x,
			pf("IOI_CLK%iINTER%s", i%2, i<2?"_M":"_S"),
			pf("IOI_CLKDIST_CLK1%s", i<2?"_M":"_S"),
			0 /* bidir */))) goto xout;
	}
	// 3 sources each:
	for (i = 0; i < 6; i++) {
		if ((rc = add_switch(model, y, x,
			pf("IOI_CLK%iINTER%s", i%3, i<3?"_M":"_S"),
			pf("IOI_CLKDIST_CLK0_ILOGIC%s", i<3?"_M":"_S"),
			0 /* bidir */))) goto xout;
		if ((rc = add_switch(model, y, x,
			pf("IOI_CLK%iINTER%s", i%3, i<3?"_M":"_S"),
			pf("IOI_CLKDIST_CLK0_OLOGIC%s", i<3?"_M":"_S"),
			0 /* bidir */))) goto xout;
	}
	// logicin wires
	{
		static const char* iologic_logicin[] =
		{
			[X_A3] = "CAL_IODELAY_SITE",
			[X_A4] = "CAL_IODELAY_SITE_S",
			[X_A6] = "CE_IODELAY_SITE_S",
			[X_B1] = "INC_IODELAY_SITE_S",
			[X_B2] = "TRAIN_OLOGIC_SITE",
			[X_B3] = "TCE_OLOGIC_SITE_S",
			[X_B6] = "T3_OLOGIC_SITE_S",
			[X_C1] = "REV_OLOGIC_SITE_S",
			[X_C2] = "D1_OLOGIC_SITE_S",
			[X_C3] = "D2_OLOGIC_SITE_S",
			[X_C4] = "D3_OLOGIC_SITE_S",
			[X_C6] = "BITSLIP_ILOGIC_SITE_S",
			[X_CE] = "SR_ILOGIC_SITE_S",
			[X_D2] = "TCE_OLOGIC_SITE",
			[X_D3] = "T1_OLOGIC_SITE",
			[X_D4] = "T2_OLOGIC_SITE",
			[X_D5] = "T3_OLOGIC_SITE",
			[X_D6] = "T4_OLOGIC_SITE",
			[X_DX] = "TRAIN_OLOGIC_SITE_S",
		
			[M_A1] = "REV_OLOGIC_SITE",
			[M_A2] = "OCE_OLOGIC_SITE",
			[M_A3] = "D1_OLOGIC_SITE",
			[M_A4] = "D2_OLOGIC_SITE",
			[M_A6] = "D4_OLOGIC_SITE",
			[M_AI] = "SR_ILOGIC_SITE",
			[M_B1] = "REV_ILOGIC_SITE",
			[M_B2] = "CE0_ILOGIC_SITE",
			[M_B3] = "OCE_OLOGIC_SITE_S",
			[M_B5] = "RST_IODELAY_SITE_S",
			[M_B6] = "T2_OLOGIC_SITE_S",
			[M_BI] = "D3_OLOGIC_SITE",
			[M_C1] = "T1_OLOGIC_SITE_S",
			[M_C3] = "CE_IODELAY_SITE",
			[M_C4] = "D4_OLOGIC_SITE_S",
			[M_D1] = "T4_OLOGIC_SITE_S",
			[M_D2] = "RST_IODELAY_SITE",
			[M_D4] = "BITSLIP_ILOGIC_SITE",
			[M_D5] = "INC_IODELAY_SITE",
			[M_D6] = "REV_ILOGIC_SITE_S",
			[M_WE] = "CE0_ILOGIC_SITE_S",
		};
		for (i = 0; i < sizeof(iologic_logicin)/sizeof(*iologic_logicin); i++) {
			if (!iologic_logicin[i]) continue;
			if ((rc = add_switch(model, y, x,
				pf("IOI_LOGICINB%i", i),
				iologic_logicin[i], /*bidir*/ 0))) goto xout;
		}
	}
	// GND
	{
		static const char* s[] = { "REV_OLOGIC_SITE",
			"SR_OLOGIC_SITE", "TRAIN_OLOGIC_SITE" };
		for (i = 0; i < 6; i++) {
			if ((rc = add_switch(model, y, x,
				pf("%sGND_TIEOFF", prefix),
				pf("%s%s", s[i/2], i%2 ? "" : "_S"),
				/*bidir*/ 0))) goto xout;
		}
	}
	// VCC
	{
		static const char* s[] = { "IOCE_ILOGIC_SITE",
			"IOCE_OLOGIC_SITE" };
		for (i = 0; i < 4; i++) {
			if ((rc = add_switch(model, y, x,
				pf("%sVCC_TIEOFF", prefix),
				pf("%s%s", s[i/2], i%2 ? "" : "_S"),
				/*bidir*/ 0))) goto xout;
		}
	}
	// CLK
	{
		static const char* s[] = { "CLKDIV_ILOGIC_SITE",
			"CLKDIV_OLOGIC_SITE", "CLK_IODELAY_SITE" };
		for (i = 0; i < 6; i++) {
			if ((rc = add_switch(model, y, x,
				pf("IOI_CLK%i", !(i%2)),
				pf("%s%s", s[i/2], i%2 ? "" : "_S"),
				/*bidir*/ 0))) goto xout;
		}
		for (i = 0; i < 4; i++) {
			if ((rc = add_switch(model, y, x,
				pf("CLK%i_ILOGIC_SITE%s", i/2, i%2 ? "_S" : ""),
				pf("CFB%i_ILOGIC_SITE%s", i/2, i%2 ? "_S" : ""),
				/*bidir*/ 0))) goto xout;
		}
	}
	// SR
	{
		static const char* s[] = { "SR_ILOGIC_SITE",
			"SR_OLOGIC_SITE" };
		for (i = 0; i < 4; i++) {
			if ((rc = add_switch(model, y, x,
				pf("IOI_SR%i", !(i%2)),
				pf("%s%s", s[i/2], i%2 ? "" : "_S"),
				/*bidir*/ 0))) goto xout;
		}
	}
	// IOCLK
	{
		for (i = 0; i < 4; i++) {
			if ((rc = add_switch(model, y, x,
				pf("%sIOCLK%i", io_prefix, i),
				pf("IOI_CLK%iINTER%s", i%2, (i/2)?"_M":"_S"),
				/*bidir*/ 0))) goto xout;
		}
	}
	{
		const char* pairs[] = {
			"D1_OLOGIC_SITE",		"OQ_OLOGIC_SITE",
			"DATAOUT_IODELAY_SITE",		"DDLY_ILOGIC_SITE",
			"DDLY2_ILOGIC_SITE",		"FABRICOUT_ILOGIC_SITE",
			"DDLY_ILOGIC_SITE",		"DFB_ILOGIC_SITE",
			"D_ILOGIC_IDATAIN_IODELAY",	"D_ILOGIC_SITE",
			"D_ILOGIC_IDATAIN_IODELAY",	"IDATAIN_IODELAY_SITE",
			"D_ILOGIC_SITE",		"DFB_ILOGIC_SITE",
			"D_ILOGIC_SITE",		"FABRICOUT_ILOGIC_SITE",
			"T1_OLOGIC_SITE",		"TQ_OLOGIC_SITE",
			"TQ_OLOGIC_SITE",		"TFB_ILOGIC_SITE",
			"TQ_OLOGIC_SITE",		"T_IODELAY_SITE",
			"OQ_OLOGIC_SITE",		"ODATAIN_IODELAY_SITE",
			"OQ_OLOGIC_SITE",		"OFB_ILOGIC_SITE" };
		for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
			if ((rc = add_switch(model, y, x,
				pairs[i*2],
				pairs[i*2+1],
				/*bidir*/ 0))) goto xout;
			if ((rc = add_switch(model, y, x,
				pf("%s%s", pairs[i*2], "_S"),
				pf("%s%s", pairs[i*2+1], "_S"),
				/*bidir*/ 0))) goto xout;
		}
		if ((rc = add_switch(model, y, x,
			"DATAOUT2_IODELAY_SITE", "DDLY2_ILOGIC_SITE",
			/*bidir*/ 0))) goto xout;
		if ((rc = add_switch(model, y, x,
			"DATAOUT2_IODELAY2_SITE_S", "DDLY2_ILOGIC_SITE_S",
			/*bidir*/ 0))) goto xout;
	}
	for (i = 0; i < 2; i++) {
		if ((rc = add_switch(model, y, x, "IOI_PCI_CE",
			pf("OCE_OLOGIC_SITE%s", i?"_S":""),
			/*bidir*/ 0))) goto xout;
	}
	for (i = 0; i < 3; i++) {
		// 3 because IBUF1 cannot be switched to non-_S
		if ((rc = add_switch(model, y, x,
			pf("%sIBUF%i", prefix2, i/2),
			pf("D_ILOGIC_IDATAIN_IODELAY%s", !(i%2)?"_S":""),
			/*bidir*/ 0))) goto xout;
	}
	{
		const char* pairs[] = {
			"DOUT_IODELAY_SITE%s",	"%sO%i",
			"OQ_OLOGIC_SITE%s",	"%sO%i",
			"TOUT_IODELAY_SITE%s",	"%sT%i",
			"TQ_OLOGIC_SITE%s",	"%sT%i" };
		for (i = 0; i < 8; i++) {
			if ((rc = add_switch(model, y, x,
				pf(pairs[(i/2)*2], i%2?"_S":""),
				pf(pairs[(i/2)*2+1], prefix2, i%2),
				/*bidir*/ 0))) goto xout;
		}
	}
	{
		const char* pairs[] = {
			"SHIFTOUT1_OLOGIC_SITE",   "SHIFTIN1_OLOGIC_SITE_S",
			"SHIFTOUT2_OLOGIC_SITE",   "SHIFTIN2_OLOGIC_SITE_S",
			"SHIFTOUT3_OLOGIC_SITE_S", "SHIFTIN3_OLOGIC_SITE",
			"SHIFTOUT4_OLOGIC_SITE_S", "SHIFTIN4_OLOGIC_SITE",
			"SHIFTOUT_ILOGIC_SITE",    "SHIFTIN_ILOGIC_SITE_S",
			"SHIFTOUT_ILOGIC_SITE_S",  "SHIFTIN_ILOGIC_SITE" };
		for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
			if ((rc = add_switch(model, y, x,
				pairs[i*2], pairs[i*2+1],
				/*bidir*/ 0))) goto xout;
		}
	}
	{
		const char* pairs[] = {
			"IOI_CLKDIST_CLK0_ILOGIC%s",	"CLK0_ILOGIC_SITE%s",
			"IOI_CLKDIST_CLK0_ILOGIC%s",	"IOCLK_IODELAY_SITE%s",
			"IOI_CLKDIST_CLK0_OLOGIC%s",	"CLK0_OLOGIC_SITE%s",
			"IOI_CLKDIST_CLK0_OLOGIC%s",	"IOCLK_IODELAY_SITE%s",
			"IOI_CLKDIST_CLK1%s",		"CLK1_ILOGIC_SITE%s",
			"IOI_CLKDIST_CLK1%s",		"CLK1_OLOGIC_SITE%s",
			"IOI_CLKDIST_CLK1%s",		"IOCLK1_IODELAY_SITE%s",
			"IOI_CLKDIST_IOCE0%s",		"IOCE_ILOGIC_SITE%s",
			"IOI_CLKDIST_IOCE1%s",		"IOCE_OLOGIC_SITE%s" };
		for (i = 0; i < sizeof(pairs)/sizeof(*pairs); i++) {
			if ((rc = add_switch(model, y, x,
				pf(pairs[(i/2)*2], i%2?"_S":"_M"),
				pf(pairs[(i/2)*2+1], i%2?"_S":""),
				/*bidir*/ 0))) goto xout;
		}
	}
	{
		const char* pairs[] = {
			"IOI_MCB_DRPADD",       "CAL_IODELAY_SITE%s",
			"IOI_MCB_DRPBROADCAST", "RST_IODELAY_SITE%s",
			"IOI_MCB_DRPCLK",       "CLK_IODELAY_SITE%s",
			"IOI_MCB_DRPCS",        "INC_IODELAY_SITE%s",
			"IOI_MCB_DRPSDO",       "CE_IODELAY_SITE%s",
			"IOI_MCB_DRPTRAIN",     "TRAIN_OLOGIC_SITE%s" };
		for (i = 0; i < sizeof(pairs)/sizeof(*pairs); i++) {
			if ((rc = add_switch(model, y, x,
				pairs[(i/2)*2],
				pf(pairs[(i/2)*2+1], i%2?"_S":""),
				/*bidir*/ 0))) goto xout;
		}
	}
	{
		const char* pairs[] = {
			"IOI_MCB_OUTN_M",          "D2_OLOGIC_SITE",
			"IOI_MCB_OUTN_S",          "D2_OLOGIC_SITE_S",
			"IOI_MCB_OUTP_M",          "D1_OLOGIC_SITE",
			"IOI_MCB_OUTP_S",          "D1_OLOGIC_SITE_S",
			"IOI_MCB_DQIEN_M",         "T2_OLOGIC_SITE",
			"IOI_MCB_DQIEN_M",         "T2_OLOGIC_SITE_S",
			"IOI_MCB_DQIEN_S",         "T1_OLOGIC_SITE",
			"IOI_MCB_DQIEN_S",         "T1_OLOGIC_SITE_S",
			"FABRICOUT_ILOGIC_SITE",   "IOI_MCB_INBYP_M",
			"FABRICOUT_ILOGIC_SITE_S", "IOI_MCB_INBYP_S",
			"OUTP_IODELAY_SITE",       "IOI_MCB_IN_M",
			"STUB_OUTP_IODELAY_S",     "IOI_MCB_IN_S" };
		for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
			if ((rc = add_switch(model, y, x,
				pairs[i*2], pairs[i*2+1],
				/*bidir*/ 0))) goto xout;
		}
	}
	if (x < LEFT_SIDE_WIDTH
	    || x >= model->x_width-RIGHT_SIDE_WIDTH) {
		if ((rc = add_switch(model, y, x,
			"AUXSDOIN_IODELAY_M", "AUXSDO_IODELAY_M",
			/*bidir*/ 0))) goto xout;
		if ((rc = add_switch(model, y, x,
			"AUXSDOIN_IODELAY_S", "AUXSDO_IODELAY_S",
			/*bidir*/ 0))) goto xout;
	} else {
		if ((rc = add_switch(model, y, x,
			"AUXSDOIN_IODELAY_S_STUB", "AUXSDO_IODELAY_S_STUB",
			/*bidir*/ 0))) goto xout;
		if ((rc = add_switch(model, y, x,
			"AUXSDOIN_IODELAY_STUB", "AUXSDO_IODELAY_STUB",
			/*bidir*/ 0))) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int init_iologic(struct fpga_model *model)
{
	int x, y, rc;

	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (has_device(model, TOP_OUTER_IO, x, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				TOP_OUTER_IO, x))) goto xout;
		}
		if (has_device(model, TOP_INNER_IO, x, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				TOP_INNER_IO, x))) goto xout;
		}
		if (has_device(model, model->y_height-BOT_INNER_IO, x, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				model->y_height-BOT_INNER_IO, x))) goto xout;
		}
		if (has_device(model, model->y_height-BOT_OUTER_IO, x, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				model->y_height-BOT_OUTER_IO, x))) goto xout;
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				y, LEFT_IO_DEVS))) goto xout;
		}
		if (has_device(model, y, model->x_width-RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			if ((rc = init_iologic_tile(model,
				y, model->x_width-RIGHT_IO_DEVS_O))) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int init_north_south_dirwire_term(struct fpga_model *model)
{
	static const int logicin_pairs[] = {21,20, 28,36, 52,44, 60,62};
	int x, i, rc;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;

		// top
		for (i = 0; i < 4; i++) {
			rc = add_switch(model,
				TOP_INNER_ROW, x,
				pf("IOI_TTERM_LOGICIN%i", logicin_pairs[i*2]),
				pf("IOI_TTERM_LOGICIN_S%i", logicin_pairs[i*2+1]),
				0 /* bidir */);
			if (rc) goto xout;
		}
		{ const char* s0_switches[] = {
			"ER1E3",   "EL1E_S0",
			"SR1E_N3", "NL1E_S0",
			"SS2E_N3", "NN2E_S0",
			"SS4E3",   "NW4E_S0",
			"SW2E3",   "NE2E_S0",
			"SW4E3",   "WW4E_S0",
			"WL1E3",   "WR1E_S0",
			"WW2E3",   "NW2E_S0", "" };
		if ((rc = add_switch_set(model, TOP_INNER_ROW, x,
			"IOI_TTERM_", s0_switches, /*inc*/ 0))) goto xout; }
		{ const char* dir[] = {
			"NN4B", "SS4A", "NN4A", "SS4M", "NN4M", "SS4C", "NN4C", "SS4E",
			"NN2B", "SS2M", "NN2M", "SS2E",
			"NE4B", "SE4A", "NE4A", "SE4M",
			"NE2B", "SE2M",
			"NW4B", "SW4A", "NW4A", "SW4M", "NW2B", "SW2M",
			"NL1B", "SL1E",
			"NR1B", "SR1E", "" };
		if ((rc = add_switch_set(model, TOP_INNER_ROW, x,
			"IOI_TTERM_", dir, /*inc*/ 3))) goto xout; }

		// bottom
		if (is_atx(X_FABRIC_BRAM_ROUTING_COL, model, x))
			continue;
		for (i = 0; i < 4; i++) {
			rc = add_switch(model,
				model->y_height-BOT_INNER_ROW, x,
				pf("IOI_BTERM_LOGICIN%i", logicin_pairs[i*2+1]),
				pf("IOI_BTERM_LOGICIN_N%i", logicin_pairs[i*2]),
				0 /* bidir */);
			if (rc) goto xout;
		}
		{ const char* n3_switches[] = {
			"EL1E0",	"ER1E_N3",
			"NE2E0",	"SW2E_N3",
			"NL1E_S0",	"SR1E_N3",
			"NN2E_S0",	"SS2E_N3",
			"NW2E0",	"WW2E_N3",
			"NW4E0",	"SS4E_N3",
			"WR1E0",	"WL1E_N3",
			"WW4E0",	"SW4E_N3", "" };
		if ((rc = add_switch_set(model, model->y_height-BOT_INNER_ROW,
			x, "IOI_BTERM_", n3_switches, /*inc*/ 0))) goto xout; }
		{ const char* dir[] = {
			"SS4B", "NN4A", "SS4A", "NN4M", "SS4M", "NN4C", "SS4C", "NN4E",
			"SS2B", "NN2M", "SS2M", "NN2E",
			"SE4B", "NE4A", "SE4A", "NE4M",
			"SE2B", "NE2M",
			"SW4B", "NW4A", "SW4A", "NW4M", "SW2B", "NW2M",
			"NL1E", "SL1B",
			"SR1B", "NR1E", "" };
		if ((rc = add_switch_set(model, model->y_height-BOT_INNER_ROW,
			x, "IOI_BTERM_", dir, /*inc*/ 3))) goto xout; }
	}
	return 0;
xout:
	return rc;
}

static int init_east_west_dirwire_term(struct fpga_model *model)
{
	int y, rc;

	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y))
			continue;
		// left
		{ const char* s[] = {
			"NE4C", "NW4M", "SW4C", "SE4E", "SW4M", "SE4C",
			"WW4A", "EE4M", "WW4B", "EE4A", "WW4C", "EE4E", "WW4M", "EE4C",
			"NW4C", "NE4E",
			"WW2B", "EE2M", "WW2M", "EE2E", "NW2M", "NE2E",
			"WL1B", "EL1E", "WR1B", "ER1E", "" };
		if ((rc = add_switch_set(model, y, LEFT_INNER_COL,
			"LTERM_", s, /*inc*/ 3))) FAIL(rc); }
		{ const char* s[] = { "SW2M", "SE2E", "" };
		if ((rc = add_switch_set(model, y, LEFT_INNER_COL,
			"LTERM_", s, /*inc*/ 2))) FAIL(rc); }
		rc = add_switch(model, y, LEFT_INNER_COL,
			"LTERM_SW2M3", "LTERM_SE2M3", 0 /* bidir */);
		if (rc) FAIL(rc);

		// right
		{ const char* s[] = {
			"EE4A", "WW4M", "EE4B", "WW4A", "EE4C", "WW4E", "EE4M", "WW4C",
			"SE4C", "SW4E", "SE4M", "SW4C",
			"NE4C", "NW4E", "NE4M", "NW4C",
			"NE2M", "NW2E", "EE2B", "WW2M", "EE2M", "WW2E", "SE2M", "SW2E",
			"EL1B", "WL1E", "ER1B", "WR1E", "" };
		if ((rc = add_switch_set(model, y, model->x_width-RIGHT_INNER_O,
			"RTERM_", s, /*inc*/ 3))) FAIL(rc); }
	}
	return 0;
fail:
	return rc;
}

static int init_ce_clk(struct fpga_model *model)
{
	int x, y, i, rc;

	// There are CE and CLK wires for IO and PLL that are going
	// horizontally through the HCLK and vertically through the logic
	// dev columns (except no-io).
	// The following sets up their corresponding switches in the term
	// tiles.
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
			// left
			for (i = 0; i <= 3; i++) {
				rc = add_switch(model, y, LEFT_INNER_COL,
					pf("HCLK_IOI_LTERM_IOCE%i", i), 
					pf("HCLK_IOI_LTERM_IOCE%i_E", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, y, LEFT_INNER_COL,
					pf("HCLK_IOI_LTERM_IOCLK%i", i), 
					pf("HCLK_IOI_LTERM_IOCLK%i_E", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, LEFT_INNER_COL,
					pf("HCLK_IOI_LTERM_PLLCE%i", i), 
					pf("HCLK_IOI_LTERM_PLLCE%i_E", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, y, LEFT_INNER_COL,
					pf("HCLK_IOI_LTERM_PLLCLK%i", i), 
					pf("HCLK_IOI_LTERM_PLLCLK%i_E", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			// right
			for (i = 0; i <= 3; i++) {
				rc = add_switch(model, y, model->x_width-RIGHT_INNER_O,
					pf("HCLK_IOI_RTERM_IOCE%i", i), 
					pf("HCLK_IOI_RTERM_IOCE%i_W", 3-i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, y, model->x_width-RIGHT_INNER_O,
					pf("HCLK_IOI_RTERM_IOCLK%i", i), 
					pf("HCLK_IOI_RTERM_IOCLK%i_W", 3-i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, model->x_width-RIGHT_INNER_O,
					pf("HCLK_IOI_RTERM_PLLCEOUT%i", i), 
					pf("HCLK_IOI_RTERM_PLLCEOUT%i_W", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, y, model->x_width-RIGHT_INNER_O,
					pf("HCLK_IOI_RTERM_PLLCLKOUT%i", i), 
					pf("HCLK_IOI_RTERM_PLLCLKOUT%i_W", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
		}
	}
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x)
		    && !is_atx(X_ROUTING_NO_IO, model, x-1)) {
			// top
			for (i = 0; i <= 3; i++) {
				rc = add_switch(model, TOP_INNER_ROW, x,
					pf("TTERM_CLB_IOCE%i", i), 
					pf("TTERM_CLB_IOCE%i_S", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, TOP_INNER_ROW, x,
					pf("TTERM_CLB_IOCLK%i", i), 
					pf("TTERM_CLB_IOCLK%i_S", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, TOP_INNER_ROW, x,
					pf("TTERM_CLB_PLLCE%i", i), 
					pf("TTERM_CLB_PLLCE%i_S", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, TOP_INNER_ROW, x,
					pf("TTERM_CLB_PLLCLK%i", i), 
					pf("TTERM_CLB_PLLCLK%i_S", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, TOP_INNER_ROW, x,
				"TTERM_CLB_PCICE", 
				"TTERM_CLB_PCICE_S",
				0 /* bidir */);

			// bottom
			if (rc) goto xout;
			for (i = 0; i <= 3; i++) {
				rc = add_switch(model, model->y_height - BOT_INNER_ROW, x,
					pf("BTERM_CLB_CEOUT%i", i), 
					pf("BTERM_CLB_CEOUT%i_N", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, model->y_height - BOT_INNER_ROW, x,
					pf("BTERM_CLB_CLKOUT%i", i), 
					pf("BTERM_CLB_CLKOUT%i_N", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, model->y_height - BOT_INNER_ROW, x,
					pf("BTERM_CLB_PLLCEOUT%i", i), 
					pf("BTERM_CLB_PLLCEOUT%i_N", i),
					0 /* bidir */);
				if (rc) goto xout;
				rc = add_switch(model, model->y_height - BOT_INNER_ROW, x,
					pf("BTERM_CLB_PLLCLKOUT%i", i), 
					pf("BTERM_CLB_PLLCLKOUT%i_N", i),
					0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, model->y_height - BOT_INNER_ROW, x,
				"BTERM_CLB_PCICE", 
				"BTERM_CLB_PCICE_N",
				0 /* bidir */);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int init_io_tile(struct fpga_model *model, int y, int x)
{
	const char* prefix;
	int i, num_devs, rc;

	if (!y) {
		prefix = "TIOB";
		rc = add_switch(model, y, x,
			pf("%s_DIFFO_OUT2", prefix), 
			pf("%s_DIFFO_IN3", prefix), 0 /* bidir */);
		if (rc) goto xout;
		num_devs = 2;
	} else if (y == model->y_height - BOT_OUTER_ROW) {
		prefix = "BIOB";
		rc = add_switch(model, y, x,
			pf("%s_DIFFO_OUT3", prefix), 
			pf("%s_DIFFO_IN2", prefix), 0 /* bidir */);
		if (rc) goto xout;
		num_devs = 2;
	} else if (!x) {
		prefix = "LIOB";
		num_devs = 1;
	} else if (x == model->x_width - RIGHT_OUTER_O) {
		prefix = "RIOB";
		num_devs = 1;
	} else
		EXIT(1);

	for (i = 0; i < num_devs*2; i++) {
		rc = add_switch(model, y, x,
			pf("%s_IBUF%i_PINW", prefix, i),
			pf("%s_IBUF%i", prefix, i), 0 /* bidir */);
		if (rc) goto xout;
		rc = add_switch(model, y, x,
			pf("%s_O%i", prefix, i),
			pf("%s_O%i_PINW", prefix, i), 0 /* bidir */);
		if (rc) goto xout;
		rc = add_switch(model, y, x,
			pf("%s_T%i", prefix, i),
			pf("%s_T%i_PINW", prefix, i), 0 /* bidir */);
		if (rc) goto xout;
	}
	rc = add_switch(model, y, x,
		pf("%s_DIFFO_OUT0", prefix),
		pf("%s_DIFFO_IN1", prefix), 0 /* bidir */);
	if (rc) goto xout;
	for (i = 0; i <= 1; i++) {
		rc = add_switch(model, y, x,
			pf("%s_PADOUT%i", prefix, i),
			pf("%s_DIFFI_IN%i", prefix, 1-i),
			0 /* bidir */);
		if (rc) goto xout;
	}
	if (num_devs > 1) {
		for (i = 0; i <= 1; i++) {
			rc = add_switch(model, y, x,
				pf("%s_PADOUT%i", prefix, i+2),
				pf("%s_DIFFI_IN%i", prefix, 3-i),
				0 /* bidir */);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int init_io(struct fpga_model *model)
{
	int x, y, rc;

	for (x = 0; x < model->x_width; x++) {
		if (has_device(model, /*y*/ 0, x, DEV_IOB)) {
			rc = init_io_tile(model, 0, x);
			if (rc) goto xout;
		}
		if (has_device(model, model->y_height - BOT_OUTER_ROW, x,
			DEV_IOB)) {
			rc = init_io_tile(model,
				model->y_height-BOT_OUTER_ROW, x);
			if (rc) goto xout;
		}
	}
	for (y = 0; y < model->y_height; y++) {
		if (has_device(model, y, /*x*/ 0, DEV_IOB)) {
			rc = init_io_tile(model, y, 0);
			if (rc) goto xout;
		}
		if (has_device(model, y, model->x_width - RIGHT_OUTER_O,
			DEV_IOB)) {
			rc = init_io_tile(model,
				y, model->x_width - RIGHT_OUTER_O);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

const char* wire_base(enum wire_type w)
{
	switch (w) {
		case W_NL1: return "NL1";
		case W_NR1: return "NR1";
		case W_EL1: return "EL1";
		case W_ER1: return "ER1";
		case W_SL1: return "SL1";
		case W_SR1: return "SR1";
		case W_WL1: return "WL1";
		case W_WR1: return "WR1";

		case W_NN2: return "NN2";
		case W_NE2: return "NE2";
		case W_EE2: return "EE2";
		case W_SE2: return "SE2";
		case W_SS2: return "SS2";
		case W_SW2: return "SW2";
		case W_WW2: return "WW2";
		case W_NW2: return "NW2";

		case W_NN4: return "NN4";
		case W_NE4: return "NE4";
		case W_EE4: return "EE4";
		case W_SE4: return "SE4";
		case W_SS4: return "SS4";
		case W_SW4: return "SW4";
		case W_WW4: return "WW4";
		case W_NW4: return "NW4";
	}
	EXIT(1);
}

enum wire_type base2wire(const char* str)
{
	if (!strncmp(str, "NL1", 3)) return W_NL1;
	if (!strncmp(str, "NR1", 3)) return W_NR1;
	if (!strncmp(str, "EL1", 3)) return W_EL1;
	if (!strncmp(str, "ER1", 3)) return W_ER1;
	if (!strncmp(str, "SL1", 3)) return W_SL1;
	if (!strncmp(str, "SR1", 3)) return W_SR1;
	if (!strncmp(str, "WL1", 3)) return W_WL1;
	if (!strncmp(str, "WR1", 3)) return W_WR1;

	if (!strncmp(str, "NN2", 3)) return W_NN2;
	if (!strncmp(str, "NE2", 3)) return W_NE2;
	if (!strncmp(str, "EE2", 3)) return W_EE2;
	if (!strncmp(str, "SE2", 3)) return W_SE2;
	if (!strncmp(str, "SS2", 3)) return W_SS2;
	if (!strncmp(str, "SW2", 3)) return W_SW2;
	if (!strncmp(str, "WW2", 3)) return W_WW2;
	if (!strncmp(str, "NW2", 3)) return W_NW2;

	if (!strncmp(str, "NN4", 3)) return W_NN4;
	if (!strncmp(str, "NE4", 3)) return W_NE4;
	if (!strncmp(str, "EE4", 3)) return W_EE4;
	if (!strncmp(str, "SE4", 3)) return W_SE4;
	if (!strncmp(str, "SS4", 3)) return W_SS4;
	if (!strncmp(str, "SW4", 3)) return W_SW4;
	if (!strncmp(str, "WW4", 3)) return W_WW4;
	if (!strncmp(str, "NW4", 3)) return W_NW4;

	return 0;
}

static int rotate_num(int cur, int off, int first, int last)
{
	if (cur+off > last)
		return first + (cur+off-last-1) % ((last+1)-first);
	if (cur+off < first)
		return last - (first-(cur+off)-1) % ((last+1)-first);
	return cur+off;
}

enum wire_type rotate_wire(enum wire_type cur, int off)
{
	if (W_IS_LEN1(cur))
		return rotate_num(cur, off, FIRST_LEN1, LAST_LEN1);
	if (W_IS_LEN2(cur))
		return rotate_num(cur, off, FIRST_LEN2, LAST_LEN2);
	if (W_IS_LEN4(cur))
		return rotate_num(cur, off, FIRST_LEN4, LAST_LEN4);
	EXIT(1);
}

enum wire_type wire_to_len(enum wire_type w, int first_len)
{
	if (W_IS_LEN1(w))
		return w-FIRST_LEN1 + first_len;
	if (W_IS_LEN2(w))
		return w-FIRST_LEN2 + first_len;
	if (W_IS_LEN4(w))
		return w-FIRST_LEN4 + first_len;
	EXIT(1);
}

static const char* routing_wirestr(enum extra_wires wire,
	int routing_io, int gclk_brk)
{
	if (routing_io) {
		if (wire == GFAN0) return "INT_IOI_GFAN0";
		if (wire == GFAN1) return "INT_IOI_GFAN1";
		if (wire == LW + LI_A5) return "INT_IOI_LOGICIN_B4";
		if (wire == LW + LI_B4) return "INT_IOI_LOGICIN_B10";
	}
	if (gclk_brk) {
		switch (wire) {
			case GCLK0: return "GCLK0_BRK";
			case GCLK1: return "GCLK1_BRK";
			case GCLK2: return "GCLK2_BRK";
			case GCLK3: return "GCLK3_BRK";
			case GCLK4: return "GCLK4_BRK";
			case GCLK5: return "GCLK5_BRK";
			case GCLK6: return "GCLK6_BRK";
			case GCLK7: return "GCLK7_BRK";
			case GCLK8: return "GCLK8_BRK";
			case GCLK9: return "GCLK9_BRK";
			case GCLK10: return "GCLK10_BRK";
			case GCLK11: return "GCLK11_BRK";
			case GCLK12: return "GCLK12_BRK";
			case GCLK13: return "GCLK13_BRK";
			case GCLK14: return "GCLK14_BRK";
			case GCLK15: return "GCLK15_BRK";
			default: ;
		}
	}
	return fpga_wire2str(wire);
}

static int init_routing_tile(struct fpga_model *model, int y, int x)
{
	int i, routing_io, gclk_brk, from_wire, to_wire, is_bidir, rc;
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	routing_io = (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L);
	gclk_brk = (tile->type == ROUTING_BRK || tile->type == BRAM_ROUTING_BRK);

	// KEEP1
	for (i = X_A1; i <= M_WE; i++) {
		rc = add_switch(model, y, x, "KEEP1_WIRE",
			logicin_s(i, routing_io), 0 /* bidir */);
		if (rc) FAIL(rc);
	}
	rc = add_switch(model, y, x, "KEEP1_WIRE", "FAN_B", 0 /* bidir */);
	if (rc) FAIL(rc);
	for (i = 0; i <= 1; i++) {
		rc = add_switch(model, y, x, "KEEP1_WIRE",
			pf("CLK%i", i), 0 /* bidir */);
		if (rc) FAIL(rc);
		rc = add_switch(model, y, x, "KEEP1_WIRE",
			pf("SR%i", i), 0 /* bidir */);
		if (rc) FAIL(rc);
		rc = add_switch(model, y, x,
			"KEEP1_WIRE", routing_wirestr(GFAN0+i, routing_io, gclk_brk), 0 /* bidir */);
		if (rc) FAIL(rc);
	}

	for (i = 0; i < model->num_bitpos; i++) {
		from_wire = model->sw_bitpos[i].from;
		to_wire = model->sw_bitpos[i].to;
		is_bidir = model->sw_bitpos[i].bidir;
		if (routing_io) {
			if (from_wire == GFAN0 || from_wire == GFAN1) {
				from_wire = VCC_WIRE;
				is_bidir = 0;
			} else if (to_wire == GFAN0 || to_wire == GFAN1)
				is_bidir = 0;
		}
		rc = add_switch(model, y, x,
			routing_wirestr(from_wire, routing_io, gclk_brk),
			routing_wirestr(to_wire, routing_io, gclk_brk),
			is_bidir);
		if (rc) FAIL(rc);
		if (is_bidir) {
			rc = add_switch(model, y, x,
				routing_wirestr(to_wire, routing_io, gclk_brk),
				routing_wirestr(from_wire, routing_io, gclk_brk),
				/* bidir */ 1);
			if (rc) FAIL(rc);
		}
	}
	if (routing_io) {
		// These switches don't come out of the general model because
		// they are bidir there and skipped on the reverse side, but
		// fall back to regular unidir switches in the io tiles. Can
		// be cleaned up one day.
		rc = add_switch(model, y, x, "LOGICIN_B6", "INT_IOI_GFAN0", 0);
		if (rc) FAIL(rc);
		rc = add_switch(model, y, x, "LOGICIN_B35", "INT_IOI_GFAN0", 0);
		if (rc) FAIL(rc);
		rc = add_switch(model, y, x, "LOGICIN_B51", "INT_IOI_GFAN1", 0);
		if (rc) FAIL(rc);
		rc = add_switch(model, y, x, "LOGICIN_B53", "INT_IOI_GFAN1", 0);
		if (rc) FAIL(rc);
	}
	{ const int logicin_b[] = {20, 21, 28, 36, 44, 52, 60, 62};
	for (i = 0; i < sizeof(logicin_b)/sizeof(*logicin_b); i++) {
		rc = add_switch(model, y, x,
			pf("LOGICIN_B%i", logicin_b[i]),
			pf("LOGICIN%i", logicin_b[i]),
			/* bidir */ 0);
		if (rc) FAIL(rc);
	}}
	return 0;
fail:
	return rc;
}

static int init_routing(struct fpga_model *model)
{
	int x, y, rc;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
			if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y))
				continue;
			rc = init_routing_tile(model, y, x);
			if (rc) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

int replicate_routing_switches(struct fpga_model *model)
{
	struct fpga_tile* tile;
	int x, y, first_y, first_x, rc;

	first_y = -1;
	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
			if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y))
				continue;
			tile = YX_TILE(model, y, x);
			// Some tiles are different so we cannot include
			// them in the high-speed replication scheme.
			if (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L
			    || tile->type == ROUTING_BRK || tile->type == BRAM_ROUTING_BRK) {
				rc = init_routing_tile(model, y, x);
				if (rc) FAIL(rc);
				continue;
			}
			if (first_y == -1) {
				first_y = y;
				first_x = x;
				rc = init_routing_tile(model, y, x);
				if (rc) FAIL(rc);
				continue;
			}
			rc = replicate_switches_and_names(model,
				first_y, first_x, y, x);
			if (rc) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int init_center(struct fpga_model *model)
{
	int i, j, rc;

	{ const char* pairs[] =
		{ "CLKC_CKLR%i",	"CLKC_GCLK%i",
		  "CLKC_CKTB%i",	"CLKC_GCLK%i",
		  "CLKC_PLL_L%i",	"CLKC_GCLK%i",
		  "CLKC_PLL_U%i",	"CLKC_GCLK%i",
		  "CLKC_SEL%i_PLL",	"S_GCLK_SITE%i",
		  "I0_GCLK_SITE%i",	"O_GCLK_SITE%i",
		  "O_GCLK_SITE%i",	"CLKC_GCLK_MAIN%i" };
	 int i_dest[2][16] =
		{{ 0,1,2,4,3,5,6,7,8,9,10,12,11,13,14,15 },
		 { 1,0,3,5,2,4,7,6,9,8,11,13,10,12,15,14 }};

	for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
		for (j = 0; j <= 15; j++) {
			if ((rc = add_switch(model, model->center_y, model->center_x,
				pf(pairs[i*2], j), pf(pairs[i*2+1], j),
				/*bidir*/ 0))) FAIL(rc);
		}
	}
	for (j = 0; j <= 15; j++) {
		if ((rc = add_switch(model, model->center_y, model->center_x,
			pf("CLKC_GCLK%i", j), pf("I0_GCLK_SITE%i", i_dest[0][j]),
			/*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, model->center_y, model->center_x,
			pf("CLKC_GCLK%i", j), pf("I1_GCLK_SITE%i", i_dest[1][j]),
			/*bidir*/ 0))) FAIL(rc);
	}}

	{ const char *to[] = {
		"CLK_PLL_LOCK_LT0", "CLK_PLL_LOCK_LT1",
		"CLK_PLL_LOCK_RT0", "CLK_PLL_LOCK_RT1" };
	 const char *from[] = {
		"PLL_LOCK_BOT0", "PLL_LOCK_BOT1",
		"PLL_LOCK_TOP0", "PLL_LOCK_TOP1" };
	for (i = 0; i < sizeof(to)/sizeof(*to); i++) {
		for (j = 0; j < sizeof(from)/sizeof(*from); j++) {
			if ((rc = add_switch(model, model->center_y,
				model->center_x-CENTER_CMTPLL_O,
				from[j], to[i], /*bidir*/ 0))) FAIL(rc);
		}
	}}

	{ const char* pairs[] = {
		"PLL_LOCK_BOT0", "PLL_LOCK_TOP2",
		"PLL_LOCK_BOT1", "PLL_LOCK_TOP2",
		"PLL_LOCK_TOP0", "PLL_LOCK_BOT2",
		"PLL_LOCK_TOP1", "PLL_LOCK_BOT2" };

	for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
		if ((rc = add_switch(model, model->center_y,
			model->center_x-CENTER_CMTPLL_O,
			pairs[i*2], pairs[i*2+1],
			/*bidir*/ 0))) FAIL(rc);
	}}

	{ const char *to[] = {
		"REGC_CLKPLL_IO_LT0", "REGC_CLKPLL_IO_LT1",
		"REGC_CLKPLL_IO_RT0", "REGC_CLKPLL_IO_RT1",
		"REGC_PLLCLK_UP_OUT0", "REGC_PLLCLK_UP_OUT1" };
	for (i = 0; i <= 3; i++) {
		for (j = 0; j < sizeof(to)/sizeof(*to); j++) {
			if ((rc = add_switch(model, model->center_y,
				model->center_x-CENTER_CMTPLL_O,
				pf("REGC_PLLCLK_DN_IN%i", i), to[j],
				/*bidir*/ 0))) FAIL(rc);
		}
	}}
	{ const char *to[] = {
		"REGC_CLKPLL_IO_LT0", "REGC_CLKPLL_IO_LT1",
		"REGC_CLKPLL_IO_RT0", "REGC_CLKPLL_IO_RT1",
		"REGC_PLLCLK_DN_OUT0", "REGC_PLLCLK_DN_OUT1" };
	for (i = 0; i <= 3; i++) {
		for (j = 0; j < sizeof(to)/sizeof(*to); j++) {
			if ((rc = add_switch(model, model->center_y,
				model->center_x-CENTER_CMTPLL_O,
				pf("REGC_PLLCLK_UP_IN%i", i), to[j],
				/*bidir*/ 0))) FAIL(rc);
		}
	}}
	return 0;
fail:
	return rc;
}

static int init_hclk(struct fpga_model *model)
{
	int x, y, i, rc;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
			if (!is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
				continue;
			for (i = 0; i <= 15; i++) {
				if ((rc = add_switch(model, y, x,
					pf("HCLK_GCLK%i_INT", i), pf("HCLK_GCLK%i", i),
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("HCLK_GCLK%i_INT", i), pf("HCLK_GCLK_UP%i", i),
					/*bidir*/ 0))) FAIL(rc);
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int init_logicout_fw(struct fpga_model *model)
{
	int i, x, y, rc;

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_FABRIC_BRAM_VIA_COL|X_FABRIC_MACC_VIA_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_CHIP_HORIZ_REGS|Y_ROW_HORIZ_AXSYMM, model, y))
					continue;
				for (i = 0; i <= 23; i++) {
					if ((rc = add_switch(model, y, x,
						pf("INT_INTERFACE_LOGICOUT_%i", i), pf("INT_INTERFACE_LOGICOUT%i", i),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
			continue;
		}
		if (is_atx(X_CENTER_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (!is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
					continue;
				for (i = 0; i <= 23; i++) {
					if ((rc = add_switch(model, y-1, model->center_x-CENTER_LOGIC_O,
						pf("INT_INTERFACE_LOGICOUT_%i", i), pf("INT_INTERFACE_LOGICOUT%i", i),
						/*bidir*/ 0))) FAIL(rc);
					if ((rc = add_switch(model, y+1, model->center_x-CENTER_LOGIC_O,
						pf("INT_INTERFACE_LOGICOUT_%i", i), pf("INT_INTERFACE_LOGICOUT%i", i),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
			continue;
		}
		if (is_atx(X_LEFT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_CHIP_HORIZ_REGS|Y_ROW_HORIZ_AXSYMM, model, y)
				    || has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC))
					continue;
				if (has_device(model, y, LEFT_IO_DEVS, DEV_OCT_CALIBRATE)) {
					for (i = 0; i <= 23; i++) {
						if ((rc = add_switch(model, y, x,
							pf("INT_INTERFACE_LOCAL_LOGICOUT_%i", i), pf("INT_INTERFACE_LOCAL_LOGICOUT%i", i),
							/*bidir*/ 0))) FAIL(rc);
					}
					continue;
				}
				// todo: this is probably not right...
				if (y == model->center_y-1 || y == model->center_y-2
				    || y < TOP_IO_TILES + HALF_ROW
				    || y == model->y_height-BOT_INNER_IO)
					continue;
				for (i = 0; i <= 23; i++) {
					if ((rc = add_switch(model, y, x,
						pf("INT_INTERFACE_LOGICOUT_%i", i), pf("INT_INTERFACE_LOGICOUT%i", i),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
			continue;
		}
		if (is_atx(X_RIGHT_IO_DEVS_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_CHIP_HORIZ_REGS|Y_ROW_HORIZ_AXSYMM, model, y)
				    || has_device(model, y, model->x_width-RIGHT_IO_DEVS_O, DEV_ILOGIC))
					continue;
				if (has_device(model, y, model->x_width-RIGHT_IO_DEVS_O, DEV_BSCAN)
				    || has_device(model, y, model->x_width-RIGHT_IO_DEVS_O, DEV_ICAP)
				    || has_device(model, y, model->x_width-RIGHT_IO_DEVS_O, DEV_SLAVE_SPI)) {
					for (i = 0; i <= 23; i++) {
						if ((rc = add_switch(model, y, x,
							pf("INT_INTERFACE_LOCAL_LOGICOUT_%i", i), pf("INT_INTERFACE_LOCAL_LOGICOUT%i", i),
							/*bidir*/ 0))) FAIL(rc);
					}
					continue;
				}
				// todo: this is probably not right...
				if (y == model->center_y-1 || y == model->center_y-2
				    || y < TOP_IO_TILES + HALF_ROW)
					continue;
				for (i = 0; i <= 23; i++) {
					if ((rc = add_switch(model, y, x,
						pf("INT_INTERFACE_LOGICOUT_%i", i), pf("INT_INTERFACE_LOGICOUT%i", i),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
			continue;
		}
	}
	return 0;
fail:
	return rc;
}

static int init_bram(struct fpga_model *model)
{
	int i, x, y, tile0_to_3, wire_num, rc;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_FABRIC_BRAM_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (!has_device(model, y, x, DEV_BRAM16))
				continue;
			{ const char* pairs[] = {
				"BRAM_CLK%c_INT1", "RAMB16BWER_CLK%c",
				"BRAM_CLK%c_INT1", "RAMB8BWER_0_CLK%c",
				"BRAM_CLK%c_INT2", "RAMB8BWER_1_CLK%c" };
			for (i = 0; i < sizeof(pairs)/sizeof(*pairs)/2; i++) {
				if ((rc = add_switch(model, y, x,
					pf(pairs[i*2], '0'+0), pf(pairs[i*2+1], 'A'+0),
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf(pairs[i*2], '0'+1), pf(pairs[i*2+1], 'A'+1),
					/*bidir*/ 0))) FAIL(rc);
			}}
			{ const char *s[] = {
				"BRAM_SR0_INT1", "RAMB16BWER_RSTA",
				"BRAM_SR0_INT1", "RAMB8BWER_0_RSTA",
				"BRAM_SR0_INT2", "RAMB8BWER_1_RSTA",
				"BRAM_SR1_INT1", "RAMB16BWER_RSTB",
				"BRAM_SR1_INT1", "RAMB8BWER_0_RSTB",
				"BRAM_SR1_INT2", "RAMB8BWER_1_RSTB" };
			for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
				if ((rc = add_switch(model, y, x, s[i*2],
					s[i*2+1], /*bidir*/ 0))) FAIL(rc);
			}}
			for (i = BI_FIRST; i <= BI_LAST; i++) {
				fdev_bram_inbit(BW+i, &tile0_to_3, &wire_num);
				if (tile0_to_3 == -1) { HERE(); continue; }
				if ((rc = add_switch(model, y, x,
					pf("BRAM_LOGICINB%i_INT%i", wire_num, tile0_to_3),
					fpga_wire2str(BW+i),
					/*bidir*/ 0))) FAIL(rc);
				if (fdev_is_bram8_inwire(i)) {
					fdev_bram_inbit(BW+(B8_0|i), &tile0_to_3, &wire_num);
					if (tile0_to_3 == -1) { HERE(); continue; }
					if ((rc = add_switch(model, y, x,
						pf("BRAM_LOGICINB%i_INT%i", wire_num, tile0_to_3),
						fpga_wire2str(BW+(B8_0|i)),
						/*bidir*/ 0))) FAIL(rc);

					fdev_bram_inbit(BW+(B8_1|i), &tile0_to_3, &wire_num);
					if (tile0_to_3 == -1) { HERE(); continue; }
					if ((rc = add_switch(model, y, x,
						pf("BRAM_LOGICINB%i_INT%i", wire_num, tile0_to_3),
						fpga_wire2str(BW+(B8_1|i)),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
			for (i = BO_FIRST; i <= BO_LAST; i++) {
				fdev_bram_outbit(BW+i, &tile0_to_3, &wire_num);
				if (tile0_to_3 == -1) { HERE(); continue; }
				if ((rc = add_switch(model, y, x,
					fpga_wire2str(BW+i),
					pf("BRAM_LOGICOUT%i_INT%i", wire_num, tile0_to_3),
					/*bidir*/ 0))) FAIL(rc);
				if (fdev_is_bram8_outwire(i)) {
					fdev_bram_outbit(BW+(B8_0|i), &tile0_to_3, &wire_num);
					if (tile0_to_3 == -1) { HERE(); continue; }
					if ((rc = add_switch(model, y, x,
						fpga_wire2str(BW+(B8_0|i)),
						pf("BRAM_LOGICOUT%i_INT%i", wire_num, tile0_to_3),
						/*bidir*/ 0))) FAIL(rc);

					fdev_bram_outbit(BW+(B8_1|i), &tile0_to_3, &wire_num);
					if (tile0_to_3 == -1) { HERE(); continue; }
					if ((rc = add_switch(model, y, x,
						fpga_wire2str(BW+(B8_1|i)),
						pf("BRAM_LOGICOUT%i_INT%i", wire_num, tile0_to_3),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int init_macc(struct fpga_model *model)
{
	int i, x, y, tile0_to_3, wire_num, rc;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_FABRIC_MACC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (!has_device(model, y, x, DEV_MACC))
				continue;
			{ const char *s[] = {
				"MACC_SR0_INT0", "RSTD_DSP48A1_SITE",
				"MACC_SR0_INT1", "RSTP_DSP48A1_SITE",
				"MACC_SR0_INT2", "RSTC_DSP48A1_SITE",
				"MACC_SR0_INT3", "RSTA_DSP48A1_SITE",
				"MACC_SR1_INT0", "RSTCARRYIN_DSP48A1_SITE",
				"MACC_SR1_INT1", "RSTOPMODE_DSP48A1_SITE",
				"MACC_SR1_INT2", "RSTM_DSP48A1_SITE",
				"MACC_SR1_INT3", "RSTB_DSP48A1_SITE",
				"MACC_CLK0_INT2", "CLK_DSP48A1_SITE" };
			for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
				if ((rc = add_switch(model, y, x, s[i*2],
					s[i*2+1], /*bidir*/ 0))) FAIL(rc);
			}}
			for (i = MI_FIRST; i <= MI_LAST; i++) {
				fdev_macc_inbit(MW+i, &tile0_to_3, &wire_num);
				if (tile0_to_3 == -1) { HERE(); continue; }
				if ((rc = add_switch(model, y, x,
					pf("MACC_LOGICINB%i_INT%i", wire_num, tile0_to_3),
					fpga_wire2str(MW+i),
					/*bidir*/ 0))) FAIL(rc);
			}
			for (i = MO_FIRST; i <= MO_LAST; i++) {
				fdev_macc_outbit(MW+i, &tile0_to_3, &wire_num);
				if (tile0_to_3 == -1) { HERE(); continue; }
				if ((rc = add_switch(model, y, x,
					fpga_wire2str(MW+i),
					pf("MACC_LOGICOUT%i_INT%i", wire_num, tile0_to_3),
					/*bidir*/ 0))) FAIL(rc);
			}
			if (y != TOP_IO_TILES+3) { // exclude topmost macc dev 
				if ((rc = add_switch(model, y, x,
					"CARRYOUT_DSP48A1_SITE", "CARRYOUT_DSP48A1_B_SITE",
					/*bidir*/ 0))) FAIL(rc);
				for (i = 0; i <= 17; i++) {
					if ((rc = add_switch(model, y, x,
						pf("BCOUT%i_DSP48A1_SITE", i), pf("BCOUT%i_DSP48A1_B_SITE", i),
						/*bidir*/ 0))) FAIL(rc);
				}
				for (i = 0; i <= 47; i++) {
					if ((rc = add_switch(model, y, x,
						pf("PCOUT%i_DSP48A1_SITE", i), pf("PCOUT%i_DSP48A1_B_SITE", i),
						/*bidir*/ 0))) FAIL(rc);
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int init_topbot_tterm_gclk(struct fpga_model *model)
{
	int i, rc;

	for (i = 0; i <= 15; i++) {
		if ((rc = add_switch(model, TOP_INNER_ROW, model->center_x + CENTER_X_PLUS_1, pf("IOI_TTERM_GCLK%i", i),
			"BUFPLL_TOP_GCLK0", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, TOP_INNER_ROW, model->center_x + CENTER_X_PLUS_1, pf("IOI_TTERM_GCLK%i", i),
			"BUFPLL_TOP_GCLK1", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, model->y_height - BOT_INNER_ROW, model->center_x + CENTER_X_PLUS_1, pf("IOI_BTERM_GCLK%i", i),
			"BUFPLL_BOT_GCLK0", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, model->y_height - BOT_INNER_ROW, model->center_x + CENTER_X_PLUS_1, pf("IOI_BTERM_GCLK%i", i),
			"BUFPLL_BOT_GCLK1", /*bidir*/ 0))) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int init_bufio_tile(struct fpga_model *model, int y, int x, const char *s1, const char *s2)
{
	int i, j, rc;

	{ static const char *s[] = {
		"I_BUFIO2_%s_SITE%i", "O_BUFIO2_%s_SITE%i",
		"I_BUFIO2_%s_SITE%i", "ODIV_BUFIO2_%s_SITE%i",
		"IFB_BUFIO2FB_%s_SITE%i", "OFB_BUFIO2FB_%s_SITE%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s2, j),
				pf(s[i*2+1], s2, j), /*bidir*/ 0))) FAIL(rc);
		}
	}}
	{ static const char *s[] = {
		"O_BUFIO2_%s_SITE%i", "%s_IOCLKOUT%i",
		"ODIV_BUFIO2_%s_SITE%i", "%s_CKPIN_OUT%i",
		"OE_BUFIO2_%s_SITE%i", "%s_IOCEOUT%i",
		"OFB_BUFIO2FB_%s_SITE%i", "%s_CLK_FEEDBACK%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s2, j),
				pf(s[i*2+1], s1, j), /*bidir*/ 0))) FAIL(rc);
		}
	}}
	{ static const char *s[] = {
		"%s_CFB%i", "IFB_BUFIO2FB_%s_SITE%i",
		"%s_CFB1_%i", "IB_BUFIO2FB_%s_SITE%i",
		"%s_DFB%i", "IFB_BUFIO2FB_%s_SITE%i",
		"%s_DFB%i", "I_BUFIO2_%s_SITE%i",
		"%s_GTPFB%i", "IFB_BUFIO2FB_%s_SITE%i",
		"%s_CLKPIN%i", "IB_BUFIO2_%s_SITE%i",
		"%s_CLKPIN%i", "I_BUFIO2_%s_SITE%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s2, j), /*bidir*/ 0))) FAIL(rc);
		}
	}}
	for (j = 0; j <= 7; j++) {
		if ((rc = add_switch(model, y, x, pf("%s_VCC", s1),
			pf("IB_BUFIO2_%s_SITE%i", s2, j),
			/*bidir*/ 0))) FAIL(rc);
	}
	{ int n[] = { 1, 0, 3, 2, 5, 4, 7, 6 };
	  const char *s[] = {
		"%s_CLKPIN%i", "IFB_BUFIO2FB_%s_SITE%i",
		"%s_DFB%i", "IB_BUFIO2_%s_SITE%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s2, n[j]),
				/*bidir*/ 0))) FAIL(rc);
		}
	}}
	{ int n1[] = { 1, 0, 3, 2, 0, 0, 2, 2};
	  int n2[] = { 4, 4, 6, 6, 1, 1, 3, 3};
	  int n3[] = { 5, 5, 7, 7, 5, 4, 7, 6};
	  const char *s[] = {
		"%s_CLKPIN%i", "IB_BUFIO2_%s_SITE%i",
		"%s_CLKPIN%i", "I_BUFIO2_%s_SITE%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s2, n1[j]),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s2, n2[j]),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s2, n3[j]),
				/*bidir*/ 0))) FAIL(rc);
		}
	}}
	return 0;
fail:
	return rc;
}

static int init_bufio(struct fpga_model *model)
{
	int y, x, j, rc;
	const char *s1, *s2;
	const int lr_n[] = { 0, 0, 2, 2, 4, 5, 6, 7 };

	y = TOP_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	s1 = "REGT";
	s2 = "TOP";
	rc = init_bufio_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (j = 0; j <= 7; j++) {
		if ((rc = add_switch(model, y, x, pf("%s_GTPCLK%i", s1, j),
			pf("I_BUFIO2_%s_SITE%i", s2, j), /*bidir*/ 0))) FAIL(rc);
	}

	y = model->center_y;
	x = LEFT_OUTER_COL;
	s1 = "REGL";
	s2 = "LEFT";
	rc = init_bufio_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (j = 0; j <= 7; j++) {
		if ((rc = add_switch(model, y, x, pf("%s_GTPCLK%i", s1, lr_n[j]),
			pf("I_BUFIO2_%s_SITE%i", s2, j), /*bidir*/ 0))) FAIL(rc);
	}

	y = model->center_y;
	x = model->x_width-RIGHT_OUTER_O;
	s1 = "REGR";
	s2 = "RIGHT";
	rc = init_bufio_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (j = 0; j <= 7; j++) {
		if ((rc = add_switch(model, y, x, pf("%s_GTPCLK%i", s1, lr_n[j]),
			pf("I_BUFIO2_%s_SITE%i", s2, j), /*bidir*/ 0))) FAIL(rc);
	}

	y = model->y_height-BOT_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	s1 = "REGB";
	s2 = "BOT";
	rc = init_bufio_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (j = 0; j <= 7; j++) {
		if ((rc = add_switch(model, y, x, pf("%s_GTPCLK%i", s1, j),
			pf("I_BUFIO2_%s_SITE%i", s2, j), /*bidir*/ 0))) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int init_bscan(struct fpga_model *model)
{
	int y, x, num_bscan_devs, i, j, rc;
	const char *s[] = {
		"BSCAN%i_CAPTURE_PINWIRE",
		"BSCAN%i_DRCK_PINWIRE",
		"BSCAN%i_RESET_PINWIRE",
		"BSCAN%i_RUNTEST_PINWIRE",
		"BSCAN%i_SHIFT_PINWIRE",
		"BSCAN%i_TCK_PINWIRE",
		"BSCAN%i_TDI_PINWIRE",
		"BSCAN%i_TMS_PINWIRE",
		"BSCAN%i_UPDATE_PINWIRE",
		"BSCAN%i_SEL_PINWIRE" };

	x = model->x_width-RIGHT_IO_DEVS_O;
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		num_bscan_devs = has_device(model, y, x, DEV_BSCAN);
		if (!num_bscan_devs) continue;
		if (num_bscan_devs != 2) {
			HERE();
			continue;
		}
		for (i = 0; i <= 1; i++) {
			for (j = 0; j < sizeof(s)/sizeof(*s); j++) {
				if ((rc = add_switch(model, y, x, pf(s[j], i+1),
					pf("INT_INTERFACE_LOCAL_LOGICOUT_%i", i*10+j), /*bidir*/ 0))) FAIL(rc);
			}
			if ((rc = add_switch(model, y, x, pf("INT_INTERFACE_LOCAL_LOGICBIN%i", i+1), 
				pf("BSCAN%i_TDO_PINWIRE", i+1), /*bidir*/ 0))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int init_dcm(struct fpga_model *model)
{
	int y, x, i, j, k, num_devs, rc;

	x = model->center_x-CENTER_CMTPLL_O;
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		num_devs = has_device(model, y, x, DEV_DCM);
		if (!num_devs) continue;
		if (num_devs != 2) {
			HERE();
			continue;
		}
		for (i = 0; i <= 1; i++) { // 2 dcm devs
			for (j = 0; j <= 9; j++) { // 10 clk outs
				for (k = 0; k <= 15; k++) { // 16 hclk dests
					if ((rc = add_switch(model, y, x,
						pf("DCM%i_CLKOUT%i", i+1, j),
						pf("DCM_HCLK%i", k), 
						/*bidir*/ 0))) FAIL(rc);
				}
			}
		}
		for (k = 0; k <= 15; k++) { // 16 hclk dests
			if ((rc = add_switch(model, y, x,
				pf("DCM_FABRIC_CLK%i", k), pf("DCM_HCLK%i", k),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("DCM_HCLK%i", k), pf("DCM_HCLK%i_N", k),
				/*bidir*/ 0))) FAIL(rc);
			if (y > model->center_y) { // lower half
				if ((rc = add_switch(model, y, x,
					pf("DCM_HCLK%i_N", k), pf("PLL_CLK_CASC_TOP%i", k), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("PLL_CLK_CASC_BOT%i", k), pf("PLL_CLK_CASC_TOP%i", k), 
					/*bidir*/ 0))) FAIL(rc);
			} else { // upper half
				if ((rc = add_switch(model, y, x,
					pf("DCM_HCLK%i_N", k), pf("PLL_CLK_CASC_BOT%i", k), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("PLL_CLK_CASC_TOP%i", k), pf("PLL_CLK_CASC_BOT%i", k), 
					/*bidir*/ 0))) FAIL(rc);
			}
		}
		for (i = 0; i <= 7; i++) { // 8 wires
			for (j = 1; j <= 2; j++) { // dcm 1 and 2
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLK_FEEDBACK_LR_TOP%i", i), pf("DCM%i_CLKFB", j), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLK_FEEDBACK_TB_BOT%i", i), pf("DCM%i_CLKFB", j), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLK_INDIRECT_LR_TOP%i", i), pf("DCM%i_CLKIN", j), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLK_INDIRECT_TB_BOT%i", i), pf("DCM%i_CLKIN", j), 
					/*bidir*/ 0))) FAIL(rc);
			}
		}
		// logicin
		{ const char *clb1_in0_to_62[] = {
			/* 0*/ 0, 0, 0, 0, 0, "DCM2_STSADRS3", 0, 0,
			/* 8*/ 0, 0, 0, 0, "DCM2_STSADRS2", 0, 0, "DCM2_PSINCDEC",
			/*16*/ "DCM2_STSADRS0", 0, 0, 0, 0, 0, 0, 0,
			/*24*/ "DCM2_PSEN", "DCM2_SE_CLK_IN1", 0, 0, 0, 0, 0, 0,
			/*32*/ 0, 0, "DCM2_CTLOSC2", 0, "DCM2_STSADRS4", 0, 0, 0,
			/*40*/ 0, 0, "DCM2_FREEZEDFS", 0, 0, 0, 0, "DCM2_RST",
			/*48*/ 0, 0, 0, 0, 0, 0, "DCM2_CTLOSC1", 0,
			/*56*/ 0, "DCM2_SE_CLK_IN0", 0, 0, 0, 0, "DCM2_STSADRS1" };
		  const char *clb2_in0_to_62[] = {
			/* 0*/ 0, 0, "DCM_FABRIC_CLK13", "DCM_FABRIC_CLK5", 0, "DCM1_STSADRS3", 0, 0,
			/* 8*/ "DCM_FABRIC_CLK15", 0, 0, 0, "DCM1_STSADRS2", 0, 0, "DCM1_PSINCDEC",
			/*16*/ "DCM1_STSADRS0", 0, 0, "DCM_FABRIC_CLK7", 0, 0, 0, 0,
			/*24*/ "DCM1_PSEN", "DCM1_SE_CLK_IN1", "DCM_FABRIC_CLK2", 0, "DCM_FABRIC_CLK8", "DCM_FABRIC_CLK9", "DCM_FABRIC_CLK3", 0,
			/*32*/ "DCM_FABRIC_CLK10", 0, "DCM1_CTLOSC2", 0, "DCM1_STSADRS4", 0, 0, "DCM_FABRIC_CLK14",
			/*40*/ 0, "DCM_FABRIC_CLK4", "DCM1_FREEZEDFS", 0, "DCM_FABRIC_CLK0", 0, 0, "DCM1_RST",
			/*48*/ 0, 0, 0, 0, "DCM_FABRIC_CLK12", 0, "DCM1_CTLOSC1", "DCM_FABRIC_CLK1",
			/*56*/ 0, "DCM1_SE_CLK_IN0", "DCM_FABRIC_CLK6", "DCM_FABRIC_CLK11", 0, 0, "DCM1_STSADRS1" };
		for (i = 0; i < 63; i++) {
			if (clb1_in0_to_62[i])
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLB1_LOGICINB%i", i), clb1_in0_to_62[i], 
					/*bidir*/ 0))) FAIL(rc);
			if (clb2_in0_to_62[i])
				if ((rc = add_switch(model, y, x,
					pf("DCM_CLB2_LOGICINB%i", i), clb2_in0_to_62[i], 
					/*bidir*/ 0))) FAIL(rc);
		}}
		// logicout
		for (i = 0; i <= 7; i++) {
			if ((rc = add_switch(model, y, x,
				pf("DCM1_STATUS%i", i), pf("DCM_CLB2_LOGICOUT%i", 16+i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("DCM2_STATUS%i", i), pf("DCM_CLB1_LOGICOUT%i", 16+i),
				/*bidir*/ 0))) FAIL(rc);
		}
		if ((rc = add_switch(model, y, x, "DCM1_LOCKED",
			"DCM_CLB2_LOGICOUT14", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, y, x, "DCM1_PSDONE",
			"DCM_CLB2_LOGICOUT15", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, y, x, "DCM2_LOCKED",
			"DCM_CLB1_LOGICOUT14", /*bidir*/ 0))) FAIL(rc);
		if ((rc = add_switch(model, y, x, "DCM2_PSDONE",
			"DCM_CLB1_LOGICOUT15", /*bidir*/ 0))) FAIL(rc);

		{ const char *s[] = {
			"CLK0", "CLK90", "CLK180", "CLK270", "CLK2X",
			"CLK2X180", "CLKDV", "CLKFX", "CLKFX180", "CONCUR" };
		for (i = 0; i < sizeof(s)/sizeof(*s); i++) {
			for (j = 1; j <= 2; j++) { // dcm1 and dcm2
				if ((rc = add_switch(model, y, x, pf("DCM%i_%s", j, s[i]),
					pf("DCM%i_CLKOUT%i", j, i), /*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x, pf("DCM%i_%s", j, s[i]),
					pf("DCM%i_CLK_TO_PLL", j), /*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x, pf("DCM%i_%s", j, s[i]),
					pf("DCM%i_%s_TEST", j, s[i]), /*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x, pf("DCM%i_%s_TEST", j, s[i]),
					pf("DCM_%i_TESTCLK_PINWIRE", j==1?1:0), /*bidir*/ 0))) FAIL(rc);
			}
		}}

		{ const char *s[] = {
			"CMT_DCM_LOCK_UP0", "CMT_DCM_LOCK_DN0",
			"CMT_DCM_LOCK_UP1", "CMT_DCM_LOCK_DN1",
			"DCM_IOCLK_UP0", "DCM_IOCLK_DOWN0",
			"DCM_IOCLK_UP1", "DCM_IOCLK_DOWN1",
			"DCM_IOCLK_UP2", "DCM_IOCLK_DOWN2",
			"DCM_IOCLK_UP3", "DCM_IOCLK_DOWN3",
			"DCM1_CLK_TO_PLL", "DCM_1_MUXED_CLK_PINWIRE",
			"DCM2_CLK_TO_PLL", "DCM_0_MUXED_CLK_PINWIRE",
			"DCM_CLB1_CLK0", "DCM2_CLK_FROM_BUFG0",
			"DCM_CLB1_CLK1", "DCM2_CLK_FROM_BUFG1",
			"DCM_CLB2_CLK0", "DCM1_CLK_FROM_BUFG0",
			"DCM_CLB2_CLK1", "DCM1_CLK_FROM_BUFG1",
			"DCM1_GFAN1", "DCM2_PSCLK",
			"DCM2_GFAN1", "DCM1_PSCLK" };
		for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
			if ((rc = add_switch(model, y, x, s[i*2], s[i*2+1],
				/*bidir*/ 0))) FAIL(rc);
		}}
		{ const char *s[] = {
			"DCM%i_CLK_FROM_BUFG0", "DCM%i_CLKFB",
			"DCM%i_CLK_FROM_BUFG1", "DCM%i_CLKIN",
			"DCM%i_CLK_FROM_PLL", "DCM%i_CLKIN",
			"DCM%i_SE_CLK_IN0", "DCM%i_CLKFB",
			"DCM%i_SE_CLK_IN1", "DCM%i_CLKIN" };
		for (j = 1; j <= 2; j++) { // dcm1 and dcm2
			for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
				if ((rc = add_switch(model, y, x, pf(s[i*2], j), pf(s[i*2+1], j),
					/*bidir*/ 0))) FAIL(rc);
			}
		}}
		if (y > model->center_y) { // lower half
			const char *s[] = {
				"CMT_DCM_LOCK_UP2", "CMT_DCM_LOCK_DN2",
				"DCM_IOCLK_UP4", "DCM_IOCLK_DOWN4",
				"DCM_IOCLK_UP5", "DCM_IOCLK_DOWN5" };
			for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
				if ((rc = add_switch(model, y, x, s[i*2], s[i*2+1],
					/*bidir*/ 0))) FAIL(rc);
			}
		} else { // upper half
			const char *s[] = {
				"CMT_DCM_LOCK_DN2", "CMT_DCM_LOCK_UP2",
				"DCM_IOCLK_DOWN4", "DCM_IOCLK_UP4",
				"DCM_IOCLK_DOWN5", "DCM_IOCLK_UP5" };
			for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
				if ((rc = add_switch(model, y, x, s[i*2], s[i*2+1],
					/*bidir*/ 0))) FAIL(rc);
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int init_pll(struct fpga_model *model)
{
	int y, x, i, j, num_devs, rc;

	x = model->center_x-CENTER_CMTPLL_O;
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		num_devs = has_device(model, y, x, DEV_PLL);
		if (!num_devs) continue;
		if (num_devs != 1) {
			HERE();
			continue;
		}
		for (i = 0; i <= 5; i++) { // 6 clk outs
			for (j = 0; j <= 15; j++) { // 16 hclk dests
				if ((rc = add_switch(model, y, x,
					pf("CMT_PLL_CLKOUT%i", i),
					pf("CMT_PLL_HCLK%i", j), 
					/*bidir*/ 0))) FAIL(rc);
			}
		}
		for (i = 0; i <= 15; i++) {
			if ((rc = add_switch(model, y, x,
				"CMT_CLKFB",
				pf("CMT_PLL_HCLK%i", i), 
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				"CMT_SE_CLK_OUT",
				pf("CMT_PLL_HCLK%i", i), 
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CMT_FABRIC_CLK%i", i),
				pf("CMT_PLL_HCLK%i", i), 
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CMT_PLL_HCLK%i", i),
				pf("CMT_PLL_HCLK%i_E", i), 
				/*bidir*/ 0))) FAIL(rc);
			if (y < model->center_y) {
				if ((rc = add_switch(model, y, x,
					pf("CMT_PLL_HCLK%i_E", i),
					pf("CLK_PLLCASC_OUT%i", i), 
					/*bidir*/ 0))) FAIL(rc);
				if ((rc = add_switch(model, y, x,
					pf("PLL_CLK_CASC_IN%i", i),
					pf("CLK_PLLCASC_OUT%i", i), 
					/*bidir*/ 0))) FAIL(rc);
			} else {
				if ((rc = add_switch(model, y, x,
					pf("CMT_PLL_HCLK%i_E", i),
					pf("PLL_CLK_CASC_IN%i", i), 
					/*bidir*/ 0))) FAIL(rc);
			}
		}
		for (i = 0; i <= 7; i++) {
			if ((rc = add_switch(model, y, x,
				pf("CMT_PLL_CLK_FEEDBACK_LRBOT%i", i),
				"CMT_CLKMUX_CLKFB",
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("PLL_CLK_FEEDBACK_TB%i", i),
				"CMT_CLKMUX_CLKFB",
				/*bidir*/ 0))) FAIL(rc);
		}
		for (i = 0; i <= 7; i++) {
			const char *to = i < 4 ? "CMT_CLKMUX_CLKREF" : "CMT_CLKMUX_CLKIN2";
			if ((rc = add_switch(model, y, x,
				pf("CMT_PLL_CLK_INDIRECT_LRBOT%i", i), to,
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("PLL_CLK_INDIRECT_TB%i", i), to,
				/*bidir*/ 0))) FAIL(rc);
		}
		for (i = 0; i <= 5; i++) {
			if ((rc = add_switch(model, y, x,
				pf("CMT_PLL_CLKOUTDCM%i", i),
				"CMT_CLK_TO_DCM1",
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CMT_PLL_CLKOUTDCM%i", i),
				"CMT_CLK_TO_DCM2",
				/*bidir*/ 0))) FAIL(rc);
		}
		{ const char *s[] = {
			"CMT_CLKFB", "CMT_CLKMUX_CLKFB",
			"CMT_CLK_FROM_BUFG0", "CMT_CLKMUX_CLKREF",
			"CMT_CLK_FROM_BUFG1", "CMT_CLKMUX_CLKIN2",
			"CMT_CLK_FROM_BUFG2", "CMT_CLKMUX_CLKFB",
			"CMT_CLK_FROM_DCM1", "CMT_CLKMUX_CLKIN2",
			"CMT_CLK_FROM_DCM2", "CMT_CLKMUX_CLKIN2",
			"CMT_CLKMUX_CLKFB", "CMT_CLKMUX_CLKFB_TEST",
			"CMT_CLKMUX_CLKFB", "CMT_PLL_CLKFBIN",
			"CMT_CLKMUX_CLKIN2", "CMT_PLL_CLKIN2",
			"CMT_CLKMUX_CLKREF", "CMT_CLKMUX_CLKREF_TEST",
			"CMT_CLKMUX_CLKREF", "CMT_PLL_CLKIN1",
			"CMT_CLK_TO_DCM1", "CMT_PLL_SKEWCLKIN1",
			"CMT_CLK_TO_DCM2", "CMT_PLL_SKEWCLKIN2",
			"CMT_PLL_CLKFB", "CMT_CLKFB",
			"CMT_PLL_CLKFBDCM", "CMT_CLKMUX_CLKFB",
			"CMT_PLL_CLKFBDCM", "CMT_PLL_CLKFBDCM_TEST",
			"CMT_PLL_CLKOUT0", "CMT_CLKMUX_CLKFB",
			"CMT_PLL_CLKOUT0", "PLLCASC_CLKOUT0",
			"CMT_PLL_CLKOUT1", "PLLCASC_CLKOUT1",
			"CMT_PLL_LOCKED", "CMT_PLL_LOCK_DN1",
			"CMT_PLL_LOCKED", "CMT_PLL_LOCK_UP1",
			"CMT_SE_CLKIN0", "CMT_CLKMUX_CLKIN2",
			"CMT_SE_CLKIN1", "CMT_CLKMUX_CLKFB",
			"CMT_TEST_CLK", "CMT_SE_CLK_OUT",
			"PLLCASC_CLKOUT0", "PLL_IOCLK_DN2",
			"PLLCASC_CLKOUT0", "PLL_IOCLK_UP2",
			"PLLCASC_CLKOUT1", "PLL_IOCLK_DN3",
			"PLLCASC_CLKOUT1", "PLL_IOCLK_UP3",
			"PLL_LOCKED", "CMT_PLL_LOCKED",
			"PLL_LOCKED", "PLL_CLB1_LOGICOUT18",
			"PLL_VCC", "PLL_REL",
			"PLL_CLB1_CLK0", "CMT_CLK_FROM_BUFG0",
			"PLL_CLB1_CLK1", "CMT_CLK_FROM_BUFG1",
			"PLL_CLB2_CLK0", "CMT_CLK_FROM_BUFG2",
			"PLL_CLB2_GFAN1", "PLL_DCLK" };
		for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
			if ((rc = add_switch(model, y, x,
				s[i*2], s[i*2+1], /*bidir*/ 0))) FAIL(rc);
		}}
		// logicin
		{ const char *clb1_in0_to_62[] = {
			/* 0*/ 0, 0, "13", "6", "1", 0, 0, 0,
			/* 8*/ "15", 0, 0, 0, 0, 0, 0, 0,
			/*16*/ 0, 0, 0, "7", 0, 0, 0, 0,
			/*24*/ 0, 0, "3", 0, "8", "9", "4", 0,
			/*32*/ "10", 0, 0, 0, 0, 0, 0, "14",
			/*40*/ 0, "5", 0, 0, "0", 0, 0, 0,
			/*48*/ 0, 0, 0, 0, "12", 0, 0, "2",
			/*56*/ 0, 0, 0, "11", 0, 0, 0 };
		  const char *clb2_in0_to_62[] = {
			/* 0*/ 0, "PLL_DI14", 0, 0, 0, "PLL_DADDR3", 0, "PLL_DADDR1",
			/* 8*/ 0, 0, 0, 0, "PLL_DI0", 0, 0, "PLL_DADDR0",
			/*16*/ 0, "PLL_DI4", 0, "PLL_DI12", 0, 0, 0, 0,
			/*24*/ "PLL_DEN", "PLL_DI2", "PLL_DI8", "PLL_DI13", "CMT_SE_CLKIN0", 0, "PLL_DI10", 0,
			/*32*/ "PLL_DI15", 0, "PLL_DI1", 0, "PLL_DADDR4", "PLL_CLKINSEL", "PLL_DI3", 0,
			/*40*/ 0, "PLL_DI11", "PLL_DADDR2", 0, "PLL_DI5", 0, 0, 0,
			/*48*/ "PLL_DI9", 0, 0, 0, "PLL_RST", 0, 0, "PLL_DI6",
			/*56*/ 0, "PLL_DI7", "PLL_DWE", "CMT_SE_CLKIN1", 0, 0, 0 };
		for (i = 0; i < 63; i++) {
			if (clb1_in0_to_62[i])
				if ((rc = add_switch(model, y, x,
					pf("PLL_CLB1_LOGICINB%i", i),
					pf("CMT_FABRIC_CLK%s", clb1_in0_to_62[i]), 
					/*bidir*/ 0))) FAIL(rc);
			if (clb2_in0_to_62[i])
				if ((rc = add_switch(model, y, x,
					pf("PLL_CLB2_LOGICINB%i", i), clb2_in0_to_62[i], 
					/*bidir*/ 0))) FAIL(rc);
		}}
		// logicout
		for (i = 0; i <= 15; i++) {
			if ((rc = add_switch(model, y, x,
				pf("PLL_DO%i", i),
				pf("PLL_CLB1_LOGICOUT%i", i),
				/*bidir*/ 0))) FAIL(rc);
		}
		if ((rc = add_switch(model, y, x, "PLL_DRDY",
			"PLL_CLB1_LOGICOUT16", /*bidir*/ 0))) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int init_center_hclk(struct fpga_model *model)
{
	int y, x, i, rc;

	x = model->center_x;
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		if (!is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
			continue;
		for (i = 0; i <= 15; i++) {
			if ((rc = add_switch(model, y, x,
				pf("CLKV_GCLKH_L%i", i),
				pf("I_BUFH_LEFT_SITE%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CLKV_GCLKH_R%i", i),
				pf("I_BUFH_RIGHT_SITE%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CLKV_GCLKH_MAIN%i_FOLD", i),
				pf("CLKV_GCLKH_L%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CLKV_GCLKH_MAIN%i_FOLD", i),
				pf("CLKV_GCLKH_R%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("I_BUFH_LEFT_SITE%i", i),
				pf("O_BUFH_LEFT_SITE%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("I_BUFH_RIGHT_SITE%i", i),
				pf("O_BUFH_RIGHT_SITE%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("O_BUFH_LEFT_SITE%i", i),
				pf("CLKV_BUFH_LEFT_L%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("O_BUFH_RIGHT_SITE%i", i),
				pf("CLKV_BUFH_RIGHT_R%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("REGV_PLL_HCLK%i", i),
				pf("CLKV_GCLKH_L%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("REGV_PLL_HCLK%i", i),
				pf("CLKV_GCLKH_R%i", i),
				/*bidir*/ 0))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int init_center_midbuf(struct fpga_model *model)
{
	int y, x, i, rc;

	x = model->center_x;
	for (y = TOP_IO_TILES; y < model->y_height-BOT_IO_TILES; y++) {
		if (!is_atyx(YX_CENTER_MIDBUF, model, y, x))
			continue;
		if (y < model->center_y) {
			for (i = 0; i <= 7; i++) {
				if ((rc = add_switch(model, y-1, x,
					pf("CLKV_CKPIN_BUF%i", i),
					pf("CLKV_MIDBUF_TOP_CKPIN%i", i),
					/*bidir*/ 0))) FAIL(rc);
			}
		} else {
			for (i = 0; i <= 7; i++) {
				if ((rc = add_switch(model, y+1, x,
					pf("CLKV_MIDBUF_BOT_CKPIN%i", i),
					pf("CLKV_CKPIN_BOT_BUF%i", i),
					/*bidir*/ 0))) FAIL(rc);
			}
		}
		for (i = 0; i <= 15; i++) {
			if ((rc = add_switch(model, y, x,
				pf("CLKV_GCLK_MAIN%i", i),
				pf("CLKV_MIDBUF_GCLK%i", i),
				/*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x,
				pf("CLKV_MIDBUF_GCLK%i", i),
				pf("CLKV_GCLK_MAIN%i_BUF", i),
				/*bidir*/ 0))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int init_center_reg_tile(struct fpga_model *model, int y, int x, const char *s1, const char *s2)
{
	int i, j, rc;

	{ static const char *s[] = {
		"%s_CKPIN_OUT%i", "%s_CKPIN%i",
		"%s_CKPIN_OUT%i", "%s_CLK_INDIRECT%i",
		"%s_GND", "%s_IOCLKOUT%i",
		"%s_VCC", "%s_CKPIN%i",
		"%s_VCC", "%s_CLK_FEEDBACK%i",
		"%s_VCC", "%s_CLK_INDIRECT%i" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		for (j = 0; j <= 7; j++) {
			if ((rc = add_switch(model, y, x, pf(s[i*2], s1, j),
				pf(s[i*2+1], s1, j), /*bidir*/ 0))) FAIL(rc);
		}
	}}
	return 0;
fail:
	return rc;
}

static int init_center_reg_tblr(struct fpga_model *model)
{
	int y, x, i, j, rc;
	const char *s1, *s2;

	//
	// top
	//

	y = TOP_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	s1 = "REGT";
	s2 = "TOP";
	rc = init_center_reg_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (i = 0; i <= 5; i++) {
		for (j = 0; j <= 1; j++) {
			if ((rc = add_switch(model, y, x, pf("REGT_PLL_IOCLK_UP%i", i),
				pf("PLLIN_BUFPLL%i_TOP_SITE", j), /*bidir*/ 0))) FAIL(rc);
			if (i < 3) {
				if ((rc = add_switch(model, y, x, pf("REGT_LOCKIN%i", i),
					pf("LOCKED_BUFPLL%i_TOP_SITE", j), /*bidir*/ 0))) FAIL(rc);
			}
		}
	}
	{ const char *s[] = {
		"REGT_VCC", "REGT_PLLCLK0",
		"REGT_VCC", "REGT_PLLCLK1",
		"IOCLK_BUFPLL0_TOP_SITE", "REGT_PLLCLK0",
		"IOCLK_BUFPLL1_TOP_SITE", "REGT_PLLCLK1",
		"LOCK_BUFPLL0_TOP_SITE", "REGT_LOCK0",
		"LOCK_BUFPLL1_TOP_SITE", "REGT_LOCK1",
		"REGT_GCLK0", "GCLK_BUFPLL0_TOP_SITE",
		"REGT_GCLK1", "GCLK_BUFPLL1_TOP_SITE",
		"SERDESSTROBE_BUFPLL0_TOP_SITE", "REGT_CEOUT0",
		"SERDESSTROBE_BUFPLL1_TOP_SITE", "REGT_CEOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}
	// y+1
	for (i = 0; i <= 7; i++) {
		if ((rc = add_switch(model, y+1, x, pf("REGT_TTERM_CLKPIN%i", i),
			pf("REGT_TTERM_CKPIN%i", i), /*bidir*/ 0))) FAIL(rc);
	}
	{ const char *s[] = {
		"REGT_TTERM_PLL_CEOUT0_N", "REGT_TTERM_PLL_CEOUT0",
		"REGT_TTERM_PLL_CEOUT1_N", "REGT_TTERM_PLL_CEOUT1",
		"REGT_TTERM_PLL_CLKOUT0_N", "REGT_TTERM_PLL_CLKOUT0",
		"REGT_TTERM_PLL_CLKOUT1_N", "REGT_TTERM_PLL_CLKOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y+1, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}

	//
	// left
	//

	y = model->center_y;
	x = LEFT_OUTER_COL;
	s1 = "REGL";
	s2 = "LEFT";
	rc = init_center_reg_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	{ const char *s[] = {
		"REGL_VCC", "REGL_PLL_CLKOUT0_LEFT",
		"REGL_VCC", "REGL_PLL_CLKOUT1_LEFT",
		"IOCLK_BUFPLL0_LEFT_SITE", "REGL_PLL_CLKOUT0_LEFT",
		"IOCLK_BUFPLL1_LEFT_SITE", "REGL_PLL_CLKOUT1_LEFT",
		"LOCK_BUFPLL0_LEFT_SITE", "REGL_LOCK0",
		"LOCK_BUFPLL1_LEFT_SITE", "REGL_LOCK1",
		"REGL_CLKPLL0", "PLLIN_BUFPLL0_LEFT_SITE",
		"REGL_CLKPLL1", "PLLIN_BUFPLL1_LEFT_SITE",
		"REGL_GCLK0", "GCLK_BUFPLL0_LEFT_SITE",
		"REGL_GCLK1", "GCLK_BUFPLL1_LEFT_SITE",
		"REGL_GCLK2", "PLLIN_BUFPLL0_LEFT_SITE",
		"REGL_GCLK3", "PLLIN_BUFPLL1_LEFT_SITE",
		"REGL_LOCKED0", "LOCKED_BUFPLL0_LEFT_SITE",
		"REGL_LOCKED1", "LOCKED_BUFPLL1_LEFT_SITE",
		"REGL_PCI_IRDY_IOB", "REGL_PCI_IRDY_PINW",
		"REGL_PCI_TRDY_IOB", "REGL_PCI_TRDY_PINW",
		"SERDESSTROBE_BUFPLL0_LEFT_SITE", "REGL_PLL_CEOUT0_LEFT",
		"SERDESSTROBE_BUFPLL1_LEFT_SITE", "REGL_PLL_CEOUT1_LEFT" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}
	// x+1
	for (i = 0; i <= 7; i++) {
		if ((rc = add_switch(model, y, x+1, pf("REGH_LTERM_CLKPIN%i", i),
			pf("REGH_LTERM_CKPIN%i", i), /*bidir*/ 0))) FAIL(rc);
	}
	{ const char *s[] = {
		"REGH_LTERM_PLL_CEOUT0_W", "REGH_LTERM_PLL_CEOUT0",
		"REGH_LTERM_PLL_CEOUT1_W", "REGH_LTERM_PLL_CEOUT1",
		"REGH_LTERM_PLL_CLKOUT0_W", "REGH_LTERM_PLL_CLKOUT0",
		"REGH_LTERM_PLL_CLKOUT1_W", "REGH_LTERM_PLL_CLKOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x+1, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}

	//
	// right
	//

	y = model->center_y;
	x = model->x_width-RIGHT_OUTER_O;
	s1 = "REGR";
	s2 = "RIGHT";
	rc = init_center_reg_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	{ const char *s[] = {
		"REGR_VCC", "REGR_PLLCLK0",
		"REGR_VCC", "REGR_PLLCLK1",
		"IOCLK_BUFPLL0_RIGHT_SITE", "REGR_PLLCLK0",
		"IOCLK_BUFPLL1_RIGHT_SITE", "REGR_PLLCLK1",
		"LOCK_BUFPLL0_RIGHT_SITE", "REGR_LOCK0",
		"LOCK_BUFPLL1_RIGHT_SITE", "REGR_LOCK1",
		"REGR_CLKPLL0", "PLLIN_BUFPLL0_RIGHT_SITE",
		"REGR_CLKPLL1", "PLLIN_BUFPLL1_RIGHT_SITE",
		"REGR_GCLK0", "GCLK_BUFPLL0_RIGHT_SITE",
		"REGR_GCLK1", "GCLK_BUFPLL1_RIGHT_SITE",
		"REGR_GCLK2", "PLLIN_BUFPLL0_RIGHT_SITE",
		"REGR_GCLK3", "PLLIN_BUFPLL1_RIGHT_SITE",
		"REGR_LOCKED0", "LOCKED_BUFPLL0_RIGHT_SITE",
		"REGR_LOCKED1", "LOCKED_BUFPLL1_RIGHT_SITE",
		"REGR_PCI_IRDY_IOB", "REGR_PCI_IRDY_PINW",
		"REGR_PCI_TRDY_IOB", "REGR_PCI_TRDY_PINW",
		"SERDESSTROBE_BUFPLL0_RIGHT_SITE", "REGR_CEOUT0",
		"SERDESSTROBE_BUFPLL1_RIGHT_SITE", "REGR_CEOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}
	// x-1
	for (i = 0; i <= 7; i++) {
		if ((rc = add_switch(model, y, x-1, pf("REGH_RTERM_CLKPIN%i", i),
			pf("REGH_RTERM_CKPIN%i", i), /*bidir*/ 0))) FAIL(rc);
	}
	{ const char *s[] = {
		"REGH_RTERM_PLL_CEOUT0_E", "REGH_RTERM_PLL_CEOUT0",
		"REGH_RTERM_PLL_CEOUT1_E", "REGH_RTERM_PLL_CEOUT1",
		"REGH_RTERM_PLL_CLKOUT0_E", "REGH_RTERM_PLL_CLKOUT0",
		"REGH_RTERM_PLL_CLKOUT1_E", "REGH_RTERM_PLL_CLKOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x-1, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}

	//
	// bottom
	//

	y = model->y_height-BOT_OUTER_ROW;
	x = model->center_x-CENTER_CMTPLL_O;
	s1 = "REGB";
	s2 = "BOT";
	rc = init_center_reg_tile(model, y, x, s1, s2);
	if (rc) FAIL(rc);
	for (i = 0; i <= 5; i++) {
		for (j = 0; j <= 1; j++) {
			if ((rc = add_switch(model, y, x, pf("REGB_PLL_IOCLK_DOWN%i", i),
				pf("PLLIN_BUFPLL%i_BOT_SITE", j), /*bidir*/ 0))) FAIL(rc);
			if (i < 3) {
				if ((rc = add_switch(model, y, x, pf("REGB_LOCKIN%i", i),
					pf("LOCKED_BUFPLL%i_BOT_SITE", j), /*bidir*/ 0))) FAIL(rc);
			}
		}
	}
	{ const char *s[] = {
		"REGB_VCC", "REGB_PLLCLK0",
		"REGB_VCC", "REGB_PLLCLK1",
		"IOCLK_BUFPLL0_BOT_SITE", "REGB_PLLCLK0",
		"IOCLK_BUFPLL1_BOT_SITE", "REGB_PLLCLK1",
		"LOCK_BUFPLL0_BOT_SITE", "REGB_LOCK0",
		"LOCK_BUFPLL1_BOT_SITE", "REGB_LOCK1",
		"REGB_GCLK0", "GCLK_BUFPLL0_BOT_SITE",
		"REGB_GCLK1", "GCLK_BUFPLL1_BOT_SITE",
		"SERDESSTROBE_BUFPLL0_BOT_SITE", "REGB_CEOUT0",
		"SERDESSTROBE_BUFPLL1_BOT_SITE", "REGB_CEOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}
	// y-1
	for (i = 0; i <= 7; i++) {
		if ((rc = add_switch(model, y-1, x, pf("REGB_BTERM_CLKPIN%i", i),
			pf("REGB_BTERM_CKPIN%i", i), /*bidir*/ 0))) FAIL(rc);
	}
	{ const char *s[] = {
		"REGB_BTERM_PLL_CEOUT0_S", "REGB_BTERM_PLL_CEOUT0",
		"REGB_BTERM_PLL_CEOUT1_S", "REGB_BTERM_PLL_CEOUT1",
		"REGB_BTERM_PLL_CLKOUT0_S", "REGB_BTERM_PLL_CLKOUT0",
		"REGB_BTERM_PLL_CLKOUT1_S", "REGB_BTERM_PLL_CLKOUT1" };
	for (i = 0; i < sizeof(s)/sizeof(*s)/2; i++) {
		if ((rc = add_switch(model, y-1, x, pf(s[i*2]),
			pf(s[i*2+1]), /*bidir*/ 0))) FAIL(rc);
	}}
	return 0;
fail:
	return rc;
}

static int init_center_topbot_cfb_dfb(struct fpga_model *model)
{
	const int x_enum[] = { model->center_x-CENTER_LOGIC_O,
		model->center_x+CENTER_X_PLUS_2 };
	int y, i, x_i, rc;

	y = TOP_INNER_ROW;
	for (x_i = 0; x_i < sizeof(x_enum)/sizeof(*x_enum); x_i++) {
		for (i = 0; i <= 1; i++) {
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_CFB1_M%i_S", i+1),
				pf("IOI_REGT_CFB1_M%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_CFB1_S%i_S", i+1),
				pf("IOI_REGT_CFB1_S%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_CFB_M%i_S", i+1),
				pf("IOI_REGT_CFB_M%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_CFB_S%i_S", i+1),
				pf("IOI_REGT_CFB_S%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_DFB_M%i_S", i+1),
				pf("IOI_REGT_DFB_M%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_DFB_S%i_S", i+1),
				pf("IOI_REGT_DFB_S%i", i+1), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_DQSN%i_S", i),
				pf("IOI_REGT_DQSN%i", i), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("IOI_REGT_DQSP%i_S", i),
				pf("IOI_REGT_DQSP%i", i), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("TTERM_IOIBOT_IBUF%i", i),
				pf("IOI_REGT_CLKPIN%i", i+2), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("TTERM_IOIUP_IBUF%i", i),
				pf("IOI_REGT_CLKPIN%i", i), /*bidir*/ 0))) FAIL(rc);
		}
	}

	y = model->y_height-BOT_INNER_ROW;
	for (x_i = 0; x_i < sizeof(x_enum)/sizeof(*x_enum); x_i++) {
		for (i = 0; i <= 3; i++) {
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_CLB_CFB1_%i_N", i+4),
				pf("BTERM_CLB_CFB1_%i", i+4), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_CLB_CFB%i_N", i+4),
				pf("BTERM_CLB_CFB%i", i+4), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_CLB_DFB%i_N", i+4),
				pf("BTERM_CLB_DFB%i", i+4), /*bidir*/ 0))) FAIL(rc);
		}
		for (i = 0; i <= 1; i++) {
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_CLB_DQSN%i_N", i+2),
				pf("BTERM_CLB_DQSN%i", i+2), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_CLB_DQSP%i_N", i+2),
				pf("BTERM_CLB_DQSP%i", i+2), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_IOIBOT_IBUF%i", i),
				pf("BTERM_CLB_CLKPIN%i", i+4), /*bidir*/ 0))) FAIL(rc);
			if ((rc = add_switch(model, y, x_enum[x_i], pf("BTERM_IOIUP_IBUF%i", i),
				pf("BTERM_CLB_CLKPIN%i", i+6), /*bidir*/ 0))) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}
