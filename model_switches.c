//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

static int init_ce_clk_switches(struct fpga_model* model);
static int init_io_switches(struct fpga_model* model);
static int init_routing_switches(struct fpga_model* model);
static int init_north_south_dirwire_term(struct fpga_model* model);
static int init_iologic_switches(struct fpga_model* model);
static int init_logic_switches(struct fpga_model* model);

int init_switches(struct fpga_model* model)
{
	int rc;

	rc = init_logic_switches(model);
	if (rc) goto xout;

   	rc = init_iologic_switches(model);
	if (rc) goto xout;

   	rc = init_north_south_dirwire_term(model);
	if (rc) goto xout;

   	rc = init_ce_clk_switches(model);
	if (rc) goto xout;

	rc = init_io_switches(model);
	if (rc) goto xout;

#if 0
	rc = init_routing_switches(model);
	if (rc) goto xout;
#endif
	return 0;
xout:
	return rc;
}

static int init_logic_tile(struct fpga_model* model, int y, int x)
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

static int init_logic_switches(struct fpga_model* model)
{
	int x, y, rc;

	for (x = LEFT_SIDE_WIDTH; x < model->x_width-RIGHT_SIDE_WIDTH; x++) {
		if (!is_atx(X_LOGIC_COL, model, x))
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

static int init_iologic_tile(struct fpga_model* model, int y, int x)
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

static int init_iologic_switches(struct fpga_model* model)
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

static int init_north_south_dirwire_term(struct fpga_model* model)
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
		if (is_atx(X_ROUTING_TO_BRAM_COL, model, x))
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

static int init_ce_clk_switches(struct fpga_model* model)
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
		if ((is_atx(X_FABRIC_LOGIC_IO_COL|X_CENTER_LOGIC_COL, model, x))) {
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

static int init_io_tile(struct fpga_model* model, int y, int x)
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

static int init_io_switches(struct fpga_model* model)
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

// The wires are ordered clockwise. Order is important for
// wire_to_NESW4().
enum wire_type
{
	FIRST_LEN1 = 1,
	W_NL1 = FIRST_LEN1,
	W_NR1,
	W_EL1,
	W_ER1,
	W_SL1,
	W_SR1,
	W_WL1,
	W_WR1,
	LAST_LEN1 = W_WR1,

	FIRST_LEN2,
	W_NN2 = FIRST_LEN2,
	W_NE2,
	W_EE2,
	W_SE2,
	W_SS2,
	W_SW2,
	W_WW2,
	W_NW2,
	LAST_LEN2 = W_NW2,

	FIRST_LEN4,
	W_NN4 = FIRST_LEN4,
	W_NE4,
	W_EE4,
	W_SE4,
	W_SS4,
	W_SW4,
	W_WW4,
	W_NW4,
	LAST_LEN4 = W_NW4
};

#define W_CLOCKWISE(w)			rotate_wire((w), 1)
#define W_CLOCKWISE_2(w)		rotate_wire((w), 2)
#define W_COUNTER_CLOCKWISE(w)		rotate_wire((w), -1)
#define W_COUNTER_CLOCKWISE_2(w)	rotate_wire((w), -2)

#define W_IS_LEN1(w)			((w) >= FIRST_LEN1 && (w) <= LAST_LEN1)
#define W_IS_LEN2(w)			((w) >= FIRST_LEN2 && (w) <= LAST_LEN2)
#define W_IS_LEN4(w)			((w) >= FIRST_LEN4 && (w) <= LAST_LEN4)

#define W_TO_LEN1(w)			wire_to_len(w, FIRST_LEN1)
#define W_TO_LEN2(w)			wire_to_len(w, FIRST_LEN2)
#define W_TO_LEN4(w)			wire_to_len(w, FIRST_LEN4)

static const char* wire_base(enum wire_type w);
static enum wire_type rotate_wire(enum wire_type cur, int off);
static enum wire_type wire_to_len(enum wire_type w, int first_len);

static const char* wire_base(enum wire_type w)
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

static enum wire_type wire_to_NESW4(enum wire_type w)
{
	// normalizes any of the 8 directions to just N/E/S/W
	// by going back to an even number.
	w = W_TO_LEN4(w);
	return w - (w-FIRST_LEN4)%2;
}
	
// longest should be something like "WW2E_N3"?
typedef char WIRE_NAME[8];

struct one_switch
{
	WIRE_NAME from;
	WIRE_NAME to;
};

struct set_of_switches
{
	int num_s;
	struct one_switch s[64];
};

static void add_switch_range(struct set_of_switches* dest,
	enum wire_type end_wire, int end_from, int end_to,
	enum wire_type beg_wire, int beg_from)
{
	int i;
	for (i = end_from; i <= end_to; i++) {
		sprintf(dest->s[dest->num_s].from, "%sE%i", wire_base(end_wire), i);
		sprintf(dest->s[dest->num_s].to, "%sB%i", wire_base(beg_wire), beg_from + (i-end_from));
		dest->num_s++;
	}
}

static void add_switch_E3toB0(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s = wire_base(end_wire);
	const char* beg_wire_s = wire_base(beg_wire);

	sprintf(dest->s[dest->num_s].from, "%sE3", end_wire_s);
	sprintf(dest->s[dest->num_s].to, "%sB0", beg_wire_s);
	dest->num_s++;
	add_switch_range(dest, end_wire, 0, 2, beg_wire, 1);
}

static void add_switch_E0toB3(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s = wire_base(end_wire);
	const char* beg_wire_s = wire_base(beg_wire);

	sprintf(dest->s[dest->num_s].from, "%sE0", end_wire_s);
	sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
	dest->num_s++;
	add_switch_range(dest, end_wire, 1, 3, beg_wire, 0);
}

static int add_switches(struct set_of_switches* dest,
	enum wire_type end_wire, enum wire_type beg_wire)
{
	const char* end_wire_s, *beg_wire_s;
	int i;

	end_wire_s = wire_base(end_wire);
	beg_wire_s = wire_base(beg_wire);

	//
	// First the directional routing at the end of len-1 wires.
	//

	if (W_IS_LEN1(end_wire)) {
		if (end_wire == W_WL1) {
			if (beg_wire == W_NL1) {
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					0, 2, beg_wire, 0);
				return 0;
			}
			if (beg_wire == W_WR1) {
				add_switch_range(dest, end_wire,
					0, 1, beg_wire, 2);
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB1",
					beg_wire_s);
				dest->num_s++;
				sprintf(dest->s[dest->num_s].from, "%sE2",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_NN2 || beg_wire == W_NW2) {
				sprintf(dest->s[dest->num_s].from, "%sE_N3",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					0, 2, beg_wire, 1);
				return 0;
			}
			if (beg_wire == W_SR1) {
				add_switch_E3toB0(dest, end_wire, beg_wire);
				return 0;
			}
			if (beg_wire == W_WL1) {
				add_switch_E0toB3(dest, end_wire, beg_wire);
				return 0;
			}
			add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
			return 0;
		}
		if (end_wire == W_WR1) {
			if (beg_wire == W_SW2 || beg_wire == W_WW2) {
				add_switch_range(dest, end_wire,
					1, 3, beg_wire, 0);
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_SR1) {
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB0",
					beg_wire_s);
				dest->num_s++;
				add_switch_range(dest, end_wire,
					1, 3, beg_wire, 1);
				return 0;
			}
			if (beg_wire == W_WL1) {
				add_switch_range(dest, end_wire,
					2, 3, beg_wire, 0);
				sprintf(dest->s[dest->num_s].from, "%sE_S0",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB2",
					beg_wire_s);
				dest->num_s++;
				sprintf(dest->s[dest->num_s].from, "%sE1",
					end_wire_s);
				sprintf(dest->s[dest->num_s].to, "%sB3",
					beg_wire_s);
				dest->num_s++;
				return 0;
			}
			if (beg_wire == W_WR1) {
				add_switch_E3toB0(dest, end_wire, beg_wire);
				return 0;
			}
			if (beg_wire == W_NL1) {
				add_switch_E0toB3(dest, end_wire, beg_wire);
				return 0;
			}
			add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
			return 0;
		}
		if (beg_wire == W_WR1 || beg_wire == W_ER1
		    || beg_wire == W_SR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_NL1 || beg_wire == W_EL1
		    || beg_wire == W_WL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	//
	// The rest of the function is for directional routing
	// at the end of len-2 and len-4 wires.
	//

	if (end_wire == W_WW2) {
		if (beg_wire == W_NL1) {
			add_switch_range(dest, end_wire, 0, 2, beg_wire, 0);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		if (beg_wire == W_WR1) {
			add_switch_range(dest, end_wire, 0, 1, beg_wire, 2);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB1",
				beg_wire_s);
			dest->num_s++;
			sprintf(dest->s[dest->num_s].from, "%sE2",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		if (beg_wire == W_WL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_SR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_WW4 || beg_wire == W_NN2
		    || beg_wire == W_NW2 || beg_wire == W_NE4
		    || beg_wire == W_NN4 || beg_wire == W_NW4) {
			add_switch_range(dest, end_wire, 0, 2, beg_wire, 1);
			sprintf(dest->s[dest->num_s].from, "%sE_N3",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	if (end_wire == W_NW2 || end_wire == W_NW4
	    || end_wire == W_WW4) {
		if (beg_wire == W_NL1) {
			add_switch_E0toB3(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_WR1) {
			add_switch_E3toB0(dest, end_wire, beg_wire);
			return 0;
		}
		if (beg_wire == W_SR1) {
			sprintf(dest->s[dest->num_s].from, "%sE_S0",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB0",
				beg_wire_s);
			dest->num_s++;
			add_switch_range(dest, end_wire, 1, 3, beg_wire, 1);
			return 0;
		}
		if (beg_wire == W_WL1) {
			sprintf(dest->s[dest->num_s].from, "%sE_S0",
				end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB2",
				beg_wire_s);
			dest->num_s++;
			sprintf(dest->s[dest->num_s].from, "%sE1", end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
			dest->num_s++;
			add_switch_range(dest, end_wire, 2, 3, beg_wire, 0);
			return 0;
		}
		if (beg_wire == W_SS4 || beg_wire == W_SW2
		    || beg_wire == W_SW4 || beg_wire == W_WW2) {
			add_switch_range(dest, end_wire, 1, 3, beg_wire, 0);
			sprintf(dest->s[dest->num_s].from, "%sE_S0", end_wire_s);
			sprintf(dest->s[dest->num_s].to, "%sB3", beg_wire_s);
			dest->num_s++;
			return 0;
		}
		add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
		return 0;
	}

	if ((end_wire == W_SS2 || end_wire == W_SS4
	     || end_wire == W_SW2 || end_wire == W_SW4)
	    && (beg_wire == W_NW4 || beg_wire == W_WW4)) {
		for (i = 0; i <= 2; i++) {
			sprintf(dest->s[dest->num_s].from, "%sE%i",
				end_wire_s, i);
			sprintf(dest->s[dest->num_s].to, "%sB%i",
				beg_wire_s, i+1);
			dest->num_s++;
		}
		sprintf(dest->s[dest->num_s].from, "%sE_N3", end_wire_s);
		sprintf(dest->s[dest->num_s].to, "%sB0", beg_wire_s);
		dest->num_s++;
		return 0;
	}

	if (beg_wire == W_WR1 || beg_wire == W_ER1 || beg_wire == W_SR1) {
		add_switch_E3toB0(dest, end_wire, beg_wire);
		return 0;
	}

	if (beg_wire == W_WL1 || beg_wire == W_EL1 || beg_wire == W_NL1) {
		add_switch_E0toB3(dest, end_wire, beg_wire);
		return 0;
	}

	add_switch_range(dest, end_wire, 0, 3, beg_wire, 0);
	return 0;
}

static int build_dirwire_switches(struct set_of_switches* dest,
	enum wire_type src_wire)
{
	enum wire_type cur;
	int i, rc;

	dest->num_s = 0;
	if (W_IS_LEN2(src_wire) || W_IS_LEN4(src_wire)) {
		cur = W_COUNTER_CLOCKWISE_2(wire_to_NESW4(src_wire));
		for (i = 0; i < 6; i++) {
			rc = add_switches(dest, src_wire, cur);
			if (rc) goto xout;
			cur = W_CLOCKWISE(cur);
		}
	}
	cur = W_COUNTER_CLOCKWISE(W_TO_LEN2(wire_to_NESW4(src_wire)));
	for (i = 0; i < 4; i++) {
		rc = add_switches(dest, src_wire, cur);
		if (rc) goto xout;
		cur = W_CLOCKWISE(cur);
	}
	cur = W_COUNTER_CLOCKWISE(W_TO_LEN1(wire_to_NESW4(src_wire)));
	for (i = 0; i < 4; i++) {
		rc = add_switches(dest, src_wire, cur);
		if (rc) goto xout;
		cur = W_CLOCKWISE(cur);
	}
	return 0;
xout:
	return rc;
}

static int add_logicio_extra(struct fpga_model* model,
	int y, int x, int routing_io)
{
	// 16 groups of 4. The order inside the group does not matter,
	// but the order of the groups must match the order in src_w.
	static int dest_w[] = {
		/* group  0 */ M_D1, X_A1, X_CE, X_BX  | LWF_BIDIR,
		/* group  1 */ M_B2, M_WE, X_C2, M_AX  | LWF_BIDIR,
		/* group  2 */ M_C1, M_AI, X_B1, X_AX  | LWF_BIDIR,
		/* group  3 */ M_A2, M_BI, X_D2, M_BX  | LWF_BIDIR,
		/* group  4 */ M_C2, M_DX, X_B2, FAN_B | LWF_BIDIR,
		/* group  5 */ M_A1, X_CX, X_D1, M_CE  | LWF_BIDIR,
		/* group  6 */ M_CX, M_D2, X_A2, M_CI  | LWF_BIDIR,
		/* group  7 */ M_B1, X_C1, X_DX, M_DI  | LWF_BIDIR,
		/* group  8 */ M_A5, M_C4, X_B5, X_D4,
		/* group  9 */ M_A6, M_C3, X_B6, X_D3,
		/* group 10 */ M_B5, M_D4, X_A5, X_C4,
		/* group 11 */ M_B6, M_D3, X_A6, X_C3,
		/* group 12 */ M_B4, M_D5, X_A4, X_C5,
		/* group 13 */ M_B3, M_D6, X_A3, X_C6,
		/* group 14 */ M_A3, M_C6, X_B3, X_D6,
		/* group 15 */ M_A4, M_C5, X_B4, X_D5,
	};
	// 16 groups of 5. Order of groups in sync with in_w.
	// Each dest_w group can only have 1 bidir wire, which is
	// flagged there. The flag in src_w signals whether that one
	// bidir line in dest_w is to be driven as bidir or not.
	static int src_w[] = {
		/* group  0 */ GFAN0,   	  M_AX,			M_CI | LWF_BIDIR,  M_DI | LWF_BIDIR, LOGICIN_N28,
		/* group  1 */ GFAN0 | LWF_BIDIR, LOGICIN20,   		M_CI | LWF_BIDIR,  LOGICIN_N52,	     LOGICIN_N28,
		/* group  2 */ GFAN0 | LWF_BIDIR, M_CE | LWF_BIDIR,	LOGICIN_N21,	   LOGICIN44,        LOGICIN_N60,
		/* group  3 */ GFAN0,  	          FAN_B | LWF_BIDIR,	X_AX,   	   M_CE | LWF_BIDIR, LOGICIN_N60,
		/* group  4 */ GFAN1,     	  M_BX | LWF_BIDIR,	LOGICIN21,	   LOGICIN_S44,      LOGICIN_S36,
		/* group  5 */ GFAN1 | LWF_BIDIR, FAN_B,		M_BX | LWF_BIDIR,  X_AX | LWF_BIDIR, LOGICIN_S36,
		/* group  6 */ GFAN1 | LWF_BIDIR, M_AX | LWF_BIDIR,	X_BX | LWF_BIDIR,  M_DI,             LOGICIN_S62,
		/* group  7 */ GFAN1,             LOGICIN52,		X_BX | LWF_BIDIR,  LOGICIN_S20,	     LOGICIN_S62,

		/* group  8 */ M_AX,      M_CI,        M_DI,        LOGICIN_N28, UNDEF,
		/* group  9 */ LOGICIN20, M_CI,        LOGICIN_N52, LOGICIN_N28, UNDEF,
		/* group 10 */ FAN_B,     X_AX,        M_CE,        LOGICIN_N60, UNDEF,
		/* group 11 */ M_CE,      LOGICIN_N21, LOGICIN44,   LOGICIN_N60, UNDEF,
		/* group 12 */ FAN_B,     M_BX,        X_AX,        LOGICIN_S36, UNDEF,
		/* group 13 */ M_BX,      LOGICIN21,   LOGICIN_S44, LOGICIN_S36, UNDEF,
		/* group 14 */ LOGICIN52, X_BX,        LOGICIN_S20, LOGICIN_S62, UNDEF,
		/* group 15 */ M_AX,      X_BX,        M_DI,        LOGICIN_S62, UNDEF
	};
	char from_str[32], to_str[32];
	int i, j, cur_dest_w, is_bidir, rc;

	for (i = 0; i < sizeof(src_w)/sizeof(src_w[0]); i++) {
		for (j = 0; j < 4; j++) {

			cur_dest_w = dest_w[(i/5)*4 + j];
			is_bidir = (cur_dest_w & LWF_BIDIR) && (src_w[i] & LWF_BIDIR);
			if ((cur_dest_w & LWF_WIRE_MASK) == FAN_B)
				strcpy(to_str, "FAN_B");
			else
				strcpy(to_str, logicin_s(cur_dest_w, routing_io));

			switch (src_w[i] & LWF_WIRE_MASK) {
				case UNDEF: continue;
				default:
					snprintf(from_str, sizeof(from_str), "LOGICIN_B%i",
						src_w[i] & LWF_WIRE_MASK);
					break;
				case GFAN0:
				case GFAN1:
					if (routing_io) {
						is_bidir = 0;
						strcpy(from_str, "VCC_WIRE");
					} else {
						strcpy(from_str, (src_w[i] & LWF_WIRE_MASK)
							== GFAN0 ? "GFAN0" : "GFAN1");
					}
					break;
				case FAN_B:		strcpy(from_str, "FAN_B"); break;
				case LOGICIN20:		strcpy(from_str, "LOGICIN20"); break;
				case LOGICIN21:		strcpy(from_str, "LOGICIN21"); break;
				case LOGICIN44:		strcpy(from_str, "LOGICIN44"); break;
				case LOGICIN52:		strcpy(from_str, "LOGICIN52"); break;
				case LOGICIN_N21:	strcpy(from_str, "LOGICIN_N21"); break;
				case LOGICIN_N28:	strcpy(from_str, "LOGICIN_N28"); break;
				case LOGICIN_N52:	strcpy(from_str, "LOGICIN_N52"); break;
				case LOGICIN_N60:	strcpy(from_str, "LOGICIN_N60"); break;
				case LOGICIN_S20:	strcpy(from_str, "LOGICIN_S20"); break;
				case LOGICIN_S36:	strcpy(from_str, "LOGICIN_S36"); break;
				case LOGICIN_S44:	strcpy(from_str, "LOGICIN_S44"); break;
				case LOGICIN_S62:	strcpy(from_str, "LOGICIN_S62"); break;
			}
			rc = add_switch(model, y, x, from_str, to_str, is_bidir);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int add_logicout_switches(struct fpga_model* model,
	int y, int x, int routing_io)
{
	// 8 groups of 3. The order inside the group does not matter,
	// but the order of the groups does.
	static int out_wires[] = {
		/* group 0 */ M_A,    M_CMUX, X_AQ,
		/* group 1 */ M_AQ,   X_A,    X_CMUX,
		/* group 2 */ M_BQ,   X_B,    X_DMUX,
		/* group 3 */ M_B,    M_DMUX, X_BQ,
		/* group 4 */ M_AMUX, M_C,    X_CQ,
		/* group 5 */ M_CQ,   X_AMUX, X_C,
		/* group 6 */ M_BMUX, M_D,    X_DQ,
		/* group 7 */ M_DQ,   X_BMUX, X_D
	};
	// Those are the logicout wires going back into logicin, for
	// each group of out wires. Again the order inside the groups
	// does not matter, but the group order must match the out wire
	// group order.
	static int logicin_wires[] = {
		/* group 0 */ M_AI, M_B6, M_C1, M_D3, X_A6, X_AX, X_B1, X_C3,
		/* group 1 */ M_A6, M_AX, M_B2, M_C3, M_WE, X_B6, X_C2, X_D3,
		/* group 2 */ M_A2, M_B5, M_BI, M_BX, M_D4, X_A5, X_C4, X_D2,
		/* group 3 */ M_A5, M_C4, M_D1, X_A1, X_B5, X_BX, X_CE, X_D4,
		/* group 4 */ M_A1, M_B4, M_CE, M_D5, X_A4, X_C5, X_CX, X_D1,
		/* group 5 */ M_A4, M_C5, M_CI, M_CX, M_D2, X_A2, X_B4, X_D5,
		/* group 6 */ M_A3, M_B1, M_C6, M_DI, X_B3, X_C1, X_D6, X_DX,
		/* group 7 */ M_B3, M_C2, M_D6, M_DX, X_A3, X_B2, X_C6, FAN_B
	};
	enum wire_type wire;
	char from_str[32], to_str[32];
	int i, j, rc;

	for (i = 0; i < sizeof(out_wires)/sizeof(out_wires[0]); i++) {
		// out to dirwires
		snprintf(from_str, sizeof(from_str), "LOGICOUT%i",
			out_wires[i]);
		wire = W_NN2;
		do {
			// len 2
			snprintf(to_str, sizeof(to_str), "%sB%i",
				wire_base(wire), i/(2*3));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;

			// len 4
			snprintf(to_str, sizeof(to_str), "%sB%i",
				wire_base(W_TO_LEN4(wire)), i/(2*3));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;

			wire = W_CLOCKWISE(wire);
		} while (wire != W_NN2); // one full turn

		// NR1, SL1
		snprintf(to_str, sizeof(to_str), "NR1B%i", i/(2*3));
		rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
		if (rc) goto xout;
		snprintf(to_str, sizeof(to_str), "SL1B%i", i/(2*3));
		rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
		if (rc) goto xout;

		// ER1, SR1, WR1 (+1)
		// NL1, EL1, WL1 (+3)
		{
			static const char* plus1[] = {"ER1B%i", "SR1B%i", "WR1B%i"};
			static const char* plus3[] = {"NL1B%i", "EL1B%i", "WL1B%i"};
			for (j = 0; j < 3; j++) {
				snprintf(to_str, sizeof(to_str), plus1[j], (i/(2*3)+1)%4);
				rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
				if (rc) goto xout;

				snprintf(to_str, sizeof(to_str), plus3[j], (i/(2*3)+3)%4);
				rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
				if (rc) goto xout;
			}
		}
			
		// back to logicin
		for (j = 0; j < 8; j++) {
			if (logicin_wires[(i/3)*8 + j] == FAN_B)
				strcpy(to_str, "FAN_B");
			else
				strcpy(to_str, logicin_s(logicin_wires[(i/3)*8 + j], routing_io));
			rc = add_switch(model, y, x, from_str, to_str,
				0 /* bidir */);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static int add_logicin_switch(struct fpga_model* model, int y, int x,
	enum wire_type dirwire, int dirwire_num,
	int logicin_num)
{
	char from_str[32], to_str[32];
	int rc;

	if (dirwire_num == 0 && logicin_num & LWF_SOUTH0)
		snprintf(from_str, sizeof(from_str), "%sE_S0",
			wire_base(dirwire));
	else if (dirwire_num == 3 && logicin_num & LWF_NORTH3)
		snprintf(from_str, sizeof(from_str), "%sE_N3",
			wire_base(dirwire));
	else
		snprintf(from_str, sizeof(from_str), "%sE%i",
			wire_base(dirwire), dirwire_num);
	if ((logicin_num & LWF_WIRE_MASK) == FAN_B)
		strcpy(to_str, "FAN_B");
	else {
		struct fpga_tile* tile = YX_TILE(model, y, x);
		int routing_io = (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L);
		strcpy(to_str, logicin_s(logicin_num, routing_io));
	}
	rc = add_switch(model, y, x, from_str, to_str, 0 /* bidir */);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

// This function adds the switches for all dirwires in the
// quarter belonging to dirwire. So dirwire should only be
// one of W_NN2, W_EE2, W_SS2 or W_WW2 - the rest is handled
// inside the function.
static int add_logicin_switch_quart(struct fpga_model* model, int y, int x,
	enum wire_type dirwire, int dirwire_num,
	int logicin_num)
{
	enum wire_type len1;
	int rc;

	rc = add_logicin_switch(model, y, x, dirwire, dirwire_num,
		logicin_num);
	if (rc) goto xout;
	len1 = W_COUNTER_CLOCKWISE(W_TO_LEN1(dirwire));
	rc = add_logicin_switch(model, y, x, len1,
		dirwire_num, logicin_num);
	if (rc) goto xout;

	if (dirwire == W_WW2) {
		int nw_num = dirwire_num+1;
		if (nw_num > 3)
			nw_num = 0;
		rc = add_logicin_switch(model, y, x, W_NW2, nw_num,
			logicin_num);
		if (rc) goto xout;
		rc = add_logicin_switch(model, y, x, W_NL1, nw_num,
			logicin_num);
		if (rc) goto xout;
	} else {
		rc = add_logicin_switch(model, y, x, W_CLOCKWISE(dirwire),
			dirwire_num, logicin_num);
		if (rc) goto xout;
		len1 = rotate_wire(len1, 3);
		rc = add_logicin_switch(model, y, x, len1,
			dirwire_num, logicin_num);
		if (rc) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int loop_and_rotate_over_wires(struct fpga_model* model, int y, int x,
	int* wires, int num_wires, int early_decrement)
{
	int i, rc;

	//
	// We loop over the wires times 4 because each wire will
	// be processed at NN, EE, SS and WW.
	//
	// i/4        position in the wire array
	// 3-(i/4)%4  num of wire 0:3 for current element in the wire array
	// i%4        NN (0) - EE (1) - SS (2) - WW (3)
	//

	for (i = 0; i < num_wires*4; i++) {
		rc = add_logicin_switch_quart(model, y, x, FIRST_LEN2+(i%4)*2,
			3-((i+early_decrement)/4)%4, wires[i/4]);
		if (rc) goto xout;
	}
	return 0;
xout:
	return rc;
}

static int add_logicin_switches(struct fpga_model* model, int y, int x)
{
	int rc;
	{ static int decrement_at_NN[] =
		{ M_DI | LWF_SOUTH0, M_CI, X_CE, M_WE,
		  M_B1 | LWF_SOUTH0, X_A2, X_A1, M_B2,
		  M_C6 | LWF_SOUTH0, M_C5, M_C4, M_C3,
		  X_D6 | LWF_SOUTH0, X_D5, X_D4, X_D3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_NN,
		sizeof(decrement_at_NN)/sizeof(decrement_at_NN[0]),
		0 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_EE[] =
		{ M_CX, X_BX, M_AX, X_DX | LWF_SOUTH0,
		  M_D2, M_D1, X_C2, X_C1 | LWF_SOUTH0,
		  M_A4, M_A5, M_A6, M_A3 | LWF_SOUTH0,
		  X_B4, X_B5, X_B6, X_B3 | LWF_SOUTH0 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_EE,
		sizeof(decrement_at_EE)/sizeof(decrement_at_EE[0]),
		3 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_SS[] =
		{ FAN_B, M_CE, M_BI, M_AI | LWF_NORTH3,
		   X_B2, M_A1, M_A2, X_B1 | LWF_NORTH3,
		   X_C6, X_C5, X_C4, X_C3 | LWF_NORTH3,
		   M_D6, M_D5, M_D4, M_D3 | LWF_NORTH3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_SS,
		sizeof(decrement_at_SS)/sizeof(decrement_at_SS[0]),
		2 /* early_decrement */);
	if (rc) goto xout; }

	{ static int decrement_at_WW[] =
		{ M_DX, X_CX, M_BX, X_AX | LWF_NORTH3,
		  M_C2, X_D1, X_D2, M_C1 | LWF_NORTH3,
		  X_A3, X_A4, X_A5, X_A6 | LWF_NORTH3,
		  M_B3, M_B4, M_B5, M_B6 | LWF_NORTH3 };

	rc = loop_and_rotate_over_wires(model, y, x, decrement_at_WW,
		sizeof(decrement_at_WW)/sizeof(decrement_at_WW[0]),
		1 /* early_decrement */);
	if (rc) goto xout; }
	return 0;
xout:
	return rc;
}

static int init_routing_switches(struct fpga_model* model)
{
	int x, y, i, j, routing_io, rc;
	struct set_of_switches dir_EB_switches;
	enum wire_type wire;
	struct fpga_tile* tile;
	const char* gfan_s, *gclk_s;

	for (x = 0; x < model->x_width; x++) {
		if (!is_atx(X_ROUTING_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y))
				continue;
			tile = YX_TILE(model, y, x);
			routing_io = (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L);
			gfan_s = routing_io ? "INT_IOI_GFAN%i" : "GFAN%i";

			// GND
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, x, "GND_WIRE",
					pf(gfan_s, i), 0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, y, x, "GND_WIRE", "SR1",
				0 /* bidir */);
			if (rc) goto xout;

			// VCC
		 	{ int vcc_dest[] = {
				X_A3, X_A4, X_A5, X_A6, X_B3, X_B4, X_B5, X_B6,
				X_C3, X_C4, X_C5, X_C6, X_D3, X_D4, X_D5, X_D6,
				M_A3, M_A4, M_A5, M_A6, M_B3, M_B4, M_B5, M_B6,
				M_C3, M_C4, M_C5, M_C6, M_D3, M_D4, M_D5, M_D6 };

			for (i = 0; i < sizeof(vcc_dest)/sizeof(vcc_dest[0]); i++) {
				rc = add_switch(model, y, x, "VCC_WIRE",
					logicin_s(vcc_dest[i], routing_io),
					0 /* bidir */);
				if (rc) goto xout;
			}}

			// KEEP1
			for (i = X_A1; i <= M_WE; i++) {
				rc = add_switch(model, y, x, "KEEP1_WIRE",
					logicin_s(i, routing_io),
					0 /* bidir */);
				if (rc) goto xout;
			}
			rc = add_switch(model, y, x, "KEEP1_WIRE", "FAN_B",
				0 /* bidir */);
			if (rc) goto xout;

			// VCC and KEEP1 to CLK0:1, SR0:1, GFAN0:1
			{ static const char* src[] = {"VCC_WIRE", "KEEP1_WIRE"};
			for (i = 0; i <= 1; i++)
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, src[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, src[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x,
						src[i],
						pf(gfan_s, j),
						0 /* bidir */);
					if (rc) goto xout;
				}
			}

			// GCLK0:15 -> CLK0:1, GFAN0:1/SR0:1
			if (tile->type == ROUTING_BRK
			    || tile->type == BRAM_ROUTING_BRK)
				gclk_s = "GCLK%i_BRK";
			else
				gclk_s = "GCLK%i";
			for (i = 0; i <= 15; i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf(gclk_s, i),
						pf("CLK%i", j),
						0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x,
						pf(gclk_s, i),
						(i < 8)
						  ? pf(gfan_s, j)
						  : pf("SR%i", j),
						0 /* bidir */);
					if (rc) goto xout;
				}
			}

			// FAN_B to SR0:1
			for (i = 0; i <= 1; i++) {
				rc = add_switch(model, y, x, "FAN_B",
					pf("SR%i", i), 0 /* bidir */);
				if (rc) goto xout;
			}

			// some logicin wires are singled out
			{ int logic_singles[] = {X_CE, X_CX, X_DX,
				M_AI, M_BI, M_CX, M_DX, M_WE};
			for (i = 0; i < sizeof(logic_singles)/sizeof(logic_singles[0]); i++) {
				rc = add_switch(model, y, x, pf("LOGICIN_B%i", logic_singles[i]),
					pf("LOGICIN%i", logic_singles[i]), 0 /* bidir */);
				if (rc) goto xout;
			}}

			// connecting directional wires endpoints to logicin
			rc = add_logicin_switches(model, y, x);
			if (rc) goto xout;

			// connecting logicout back to directional wires
			// beginning points (and some back to logicin)
			rc = add_logicout_switches(model, y, x, routing_io);
			if (rc) goto xout;

			// there are extra wires to send signals to logicin, or
			// to share/multiply logicin signals
			rc = add_logicio_extra(model, y, x, routing_io);
			if (rc) goto xout;

			// extra wires going to SR, CLK and GFAN
			{ int to_sr[] = {X_BX, M_BX, M_DI};
			for (i = 0; i < sizeof(to_sr)/sizeof(to_sr[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_sr[i]),
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ int to_clk[] = {M_BX, M_CI};
			for (i = 0; i < sizeof(to_clk)/sizeof(to_clk[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_clk[i]),
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ int to_gf[] = {M_AX, X_AX, M_CE, M_CI};
			for (i = 0; i < sizeof(to_gf)/sizeof(to_gf[0]); i++) {
				for (j = 0; j <= 1; j++) {
					int bidir = !routing_io
					  && ((!j && i < 2) || (j && i >= 2));
					rc = add_switch(model, y, x,
						pf("LOGICIN_B%i", to_gf[i]),
						pf(gfan_s, j), bidir);
					if (rc) goto xout;
				}
			}}

			// connecting the directional wires from one's end
			// to another one's beginning
			wire = W_NN2;
			do {
				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN1(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN2(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				rc = build_dirwire_switches(&dir_EB_switches, W_TO_LEN4(wire));
				if (rc) goto xout;
				for (i = 0; i < dir_EB_switches.num_s; i++) {
					rc = add_switch(model, y, x,
						dir_EB_switches.s[i].from,
						dir_EB_switches.s[i].to, 0 /* bidir */);
					if (rc) goto xout;
				}

				wire = W_CLOCKWISE(wire);
			} while (wire != W_NN2); // one full turn

			// and finally, some end wires go to CLK, SR and GFAN
			{ static const char* from[] = {"NR1E2", "WR1E2"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, from[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"ER1E1", "SR1E1"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("CLK%i", j), 0 /* bidir */);
					if (rc) goto xout;
					rc = add_switch(model, y, x, from[i],
						pf(gfan_s, j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"NR1E1", "WR1E1"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf(gfan_s, j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
			{ static const char* from[] = {"ER1E2", "SR1E2"};
			for (i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
				for (j = 0; j <= 1; j++) {
					rc = add_switch(model, y, x, from[i],
						pf("SR%i", j), 0 /* bidir */);
					if (rc) goto xout;
				}
			}}
		}
	}
	return 0;
xout:
	return rc;
}
