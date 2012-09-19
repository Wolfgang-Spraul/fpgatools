//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"
#include "parts.h"

static int init_ce_clk_switches(struct fpga_model* model);
static int init_io_switches(struct fpga_model* model);
static int init_routing_switches(struct fpga_model* model);
static int init_north_south_dirwire_term(struct fpga_model* model);
static int init_iologic_switches(struct fpga_model* model);
static int init_logic_switches(struct fpga_model* model);

int init_switches(struct fpga_model* model, int routing_sw)
{
	int rc;

	if (routing_sw) {
		rc = init_routing_switches(model);
		if (rc) goto xout;
	}

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

static int init_routing_tile(struct fpga_model* model, int y, int x)
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

static int init_routing_switches(struct fpga_model* model)
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

int replicate_routing_switches(struct fpga_model* model)
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
