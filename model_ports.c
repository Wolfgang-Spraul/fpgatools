//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

enum which_side
{
	TOP_S, BOTTOM_S, RIGHT_S, LEFT_S
};

static int init_iologic_ports(struct fpga_model* model, int y, int x,
	enum which_side side, int dup_warn)
{
	static const char* prefix, *suffix1, *suffix2;
	int rc, i;

	switch (side) {
		case TOP_S: prefix = "TIOI"; break;
		case BOTTOM_S: prefix = "BIOI"; break;
		case LEFT_S: prefix = "LIOI"; break;
		case RIGHT_S: prefix = "RIOI"; break;
		default: EXIT(1);
	}
	if (side == LEFT_S || side == RIGHT_S) {
		suffix1 = "_M";
		suffix2 = "_S";
	} else {
		suffix1 = "_STUB";
		suffix2 = "_S_STUB";
	}

	for (i = X_A /* 0 */; i <= M_DQ /* 23 */; i++) {
		rc = add_connpt_name(model, y, x, pf("IOI_INTER_LOGICOUT%i", i),
			dup_warn, /*name_i*/ 0, /*connpt_o*/ 0);
		if (rc) goto xout;
	}
	rc = add_connpt_name(model, y, x, pf("%s_GND_TIEOFF", prefix),
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, pf("%s_VCC_TIEOFF", prefix),
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, pf("%s_KEEP1_STUB", prefix),
		dup_warn, 0, 0);
	if (rc) goto xout;
	for (i = 0; i <= 4; i++) {
		rc = add_connpt_2(model, y, x, pf("AUXADDR%i_IODELAY", i),
			suffix1, suffix2, dup_warn);
		if (rc) goto xout;
	}
	rc = add_connpt_2(model, y, x, "AUXSDOIN_IODELAY", suffix1, suffix2,
		dup_warn);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "AUXSDO_IODELAY", suffix1, suffix2,
		dup_warn);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "MEMUPDATE_IODELAY", suffix1, suffix2,
		dup_warn);
	if (rc) goto xout;

	rc = add_connpt_name(model, y, x, "OUTN_IODELAY_SITE",
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "STUB_OUTN_IODELAY_S",
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "OUTP_IODELAY_SITE",
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "STUB_OUTP_IODELAY_S",
		dup_warn, 0, 0);
	if (rc) goto xout;

	for (i = 1; i <= 4; i++) {
		rc = add_connpt_2(model, y, x, pf("Q%i_ILOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("D%i_OLOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("T%i_OLOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("SHIFTIN%i_OLOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("SHIFTOUT%i_OLOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
	}
	for (i = 0; i <= 1; i++) {
		rc = add_connpt_2(model, y, x, pf("CFB%i_ILOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("CLK%i_ILOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
		rc = add_connpt_2(model, y, x, pf("CLK%i_OLOGIC_SITE", i), "", "_S",
			dup_warn);
		if (rc) goto xout;
	}
	{
		static const char* mcb_2[] = {
			"BITSLIP_ILOGIC_SITE", "BUSY_IODELAY_SITE",
			"CAL_IODELAY_SITE", "CE0_ILOGIC_SITE",
			"CE_IODELAY_SITE", "CIN_IODELAY_SITE",
			"CLKDIV_ILOGIC_SITE", "CLKDIV_OLOGIC_SITE",
			"CLK_IODELAY_SITE", "DATAOUT_IODELAY_SITE",
			"DDLY2_ILOGIC_SITE", "DDLY_ILOGIC_SITE",
			"DFB_ILOGIC_SITE", "D_ILOGIC_IDATAIN_IODELAY",
			"D_ILOGIC_SITE", "DOUT_IODELAY_SITE",
			"FABRICOUT_ILOGIC_SITE", "IDATAIN_IODELAY_SITE",
			"INCDEC_ILOGIC_SITE", "INC_IODELAY_SITE",
			"IOCE_ILOGIC_SITE", "IOCE_OLOGIC_SITE",
			"IOCLK1_IODELAY_SITE", "IOCLK_IODELAY_SITE",
			"LOAD_IODELAY_SITE", "OCE_OLOGIC_SITE",
			"ODATAIN_IODELAY_SITE", "OFB_ILOGIC_SITE",
			"OQ_OLOGIC_SITE", "RCLK_IODELAY_SITE",
			"READEN_IODELAY_UNUSED_SITE", "REV_ILOGIC_SITE",
			"REV_OLOGIC_SITE", "RST_IODELAY_SITE",
			"SHIFTIN_ILOGIC_SITE", "SHIFTOUT_ILOGIC_SITE",
			"SR_ILOGIC_SITE", "SR_OLOGIC_SITE",
			"TCE_OLOGIC_SITE", "TFB_ILOGIC_SITE",
			"T_IODELAY_SITE", "TOUT_IODELAY_SITE",
			"TQ_OLOGIC_SITE", "TRAIN_OLOGIC_SITE",
			"VALID_ILOGIC_SITE", "" };

		for (i = 0; mcb_2[i][0]; i++) {
			rc = add_connpt_2(model, y, x, mcb_2[i], "", "_S",
				dup_warn);
		}
	}
	rc = add_connpt_name(model, y, x, "DATAOUT2_IODELAY_SITE",
		dup_warn, 0, 0);
	if (rc) goto xout;
	rc = add_connpt_name(model, y, x, "DATAOUT2_IODELAY2_SITE_S",
		dup_warn, 0, 0);
	if (rc) goto xout;

	for (i = 0; i <= 2; i++) {
		rc = add_connpt_2(model, y, x, pf("IOI_CLK%iINTER", i),
			"_M", "_S", dup_warn);
		if (rc) goto xout;
	}
	for (i = 0; i <= 1; i++) {
		rc = add_connpt_2(model, y, x, pf("IOI_CLKDIST_IOCE%i", i),
			"_M", "_S", dup_warn);
		if (rc) goto xout;
	}
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK0_ILOGIC", "_M", "_S",
		dup_warn);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK0_OLOGIC", "_M", "_S",
		dup_warn);
	if (rc) goto xout;
	rc = add_connpt_2(model, y, x, "IOI_CLKDIST_CLK1", "_M", "_S",
		dup_warn);
	if (rc) goto xout;

	if (side == TOP_S || side == BOTTOM_S) {
		static const char* mcb_2[] = {
			"IOI_MCB_DQIEN", "IOI_MCB_INBYP",
			"IOI_MCB_IN", "IOI_MCB_OUTN",
			"IOI_MCB_OUTP", "" };
		static const char* mcb_1[] = {
			"IOI_MCB_DRPADD", "IOI_MCB_DRPBROADCAST",
			"IOI_MCB_DRPCLK", "IOI_MCB_DRPCS",
			"IOI_MCB_DRPSDI", "IOI_MCB_DRPSDO",
			"IOI_MCB_DRPTRAIN", "" };

		for (i = 0; mcb_2[i][0]; i++) {
			rc = add_connpt_2(model, y, x, mcb_2[i], "_M", "_S",
				dup_warn);
			if (rc) goto xout;
		}
		for (i = 0; mcb_1[i][0]; i++) {
			rc = add_connpt_name(model, y, x, mcb_1[i],
				dup_warn, 0, 0);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

int init_ports(struct fpga_model* model, int dup_warn)
{
	int x, y, i, j, k, row_num, row_pos, rc;

	// inner and outer IO tiles (ILOGIC/ILOGIC/IODELAY)
	for (x = LEFT_SIDE_WIDTH; x < model->x_width - RIGHT_SIDE_WIDTH; x++) {
		if (has_device(model, TOP_OUTER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, TOP_OUTER_IO, x, TOP_S, dup_warn);
			if (rc) goto xout;
		}
		if (has_device(model, TOP_INNER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, TOP_INNER_IO, x, TOP_S, dup_warn);
			if (rc) goto xout;
		}
		if (has_device(model, model->y_height - BOT_INNER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, model->y_height - BOT_INNER_IO, x, BOTTOM_S, dup_warn);
			if (rc) goto xout;
		}
		if (has_device(model, model->y_height - BOT_OUTER_IO, x, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, model->y_height - BOT_OUTER_IO, x, BOTTOM_S, dup_warn);
			if (rc) goto xout;
		}
	}
	for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
		if (has_device(model, y, LEFT_IO_DEVS, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, y, LEFT_IO_DEVS, LEFT_S, dup_warn);
			if (rc) goto xout;
		}
		if (has_device(model, y, model->x_width - RIGHT_IO_DEVS_O, DEV_ILOGIC)) {
			rc = init_iologic_ports(model, y, model->x_width - RIGHT_IO_DEVS_O, RIGHT_S, dup_warn);
			if (rc) goto xout;
		}
	}

	for (x = 0; x < model->x_width; x++) {
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				rc = add_connpt_name(model, y, x, "VCC_WIRE",
					dup_warn, 0, 0);
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "GND_WIRE",
					dup_warn, 0, 0);
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "KEEP1_WIRE",
					dup_warn, 0, 0);
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN",
					dup_warn, 0, 0);
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN_B",
					dup_warn, 0, 0);
				if (rc) goto xout;

				if (!is_atyx(YX_IO_ROUTING, model, y, x)) {
					for (i = 0; i <= 1; i++) {
						rc = add_connpt_name(model, y, x, pf("GFAN%i", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
				} else {
					if (!is_atx(X_CENTER_ROUTING_COL, model, x)
					    || is_aty(Y_TOPBOT_IO_RANGE, model, y)) {
						// In the center those 2 wires are connected
						// to the PLL, but elsewhere? Not clear what they
						// connect to...
						rc = add_connpt_name(model, y, x,
							logicin_s(X_A5, 1 /* routing_io */),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x,
							logicin_s(X_B4, 1 /* routing_io */),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
				}
			}
		}
		if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_BRAM_DEV) {

					static const char* pass_str[3] = {"RAMB16BWER", "RAMB8BWER_0", "RAMB8BWER_1"};
					// pass 0 is ramb16, pass 1 and 2 are for ramb8
					for (i = 0; i <= 2; i++) {
						for (j = 'A'; j <= 'B'; j++) {
							rc = add_connpt_name(model, y, x, pf("%s_CLK%c", pass_str[i], j),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_EN%c", pass_str[i], j),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_REGCE%c", pass_str[i], j),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_RST%c", pass_str[i], j),
								dup_warn, 0, 0);
							if (rc) goto xout;
							for (k = 0; k <= (!i ? 3 : 1); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DIP%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DOP%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_WE%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 13 : 12); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_ADDR%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 31 : 15); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DI%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DO%c%i", pass_str[i], j, k),
									dup_warn, 0, 0);
								if (rc) goto xout;
							}
						}
					}
				}
			}
		}
		if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_MACC_DEV) {
					static const char* pref[] = {"CE", "RST", ""};
					static const char* seq[] = {"A", "B", "C", "D", "M", "P", "OPMODE", ""};

					is_in_row(model, y, &row_num, &row_pos);
					if (!row_num && row_pos == LAST_POS_IN_ROW) {
						rc = add_connpt_name(model, y, x, "CARRYIN_DSP48A1_SITE",
							dup_warn, 0, 0);
						if (rc) goto xout;
						for (i = 0; i <= 47; i++) {
							rc = add_connpt_name(model, y, x, pf("PCIN%i_DSP48A1_SITE", i),
								dup_warn, 0, 0);
							if (rc) goto xout;
						}
					}

					rc = add_connpt_name(model, y, x, "CLK_DSP48A1_SITE",
						dup_warn, 0, 0);
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUT_DSP48A1_SITE",
						dup_warn, 0, 0);
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUTF_DSP48A1_SITE",
						dup_warn, 0, 0);
					if (rc) goto xout;

					for (i = 0; pref[i][0]; i++) {
						rc = add_connpt_name(model, y, x, pf("%sCARRYIN_DSP48A1_SITE", pref[i]),
							dup_warn, 0, 0);
						if (rc) goto xout;
						for (j = 0; seq[j][0]; j++) {
							rc = add_connpt_name(model, y, x, pf("%s%s_DSP48A1_SITE", pref[i], seq[j]),
								dup_warn, 0, 0);
							if (rc) goto xout;
						}
					}
						
					for (i = 0; i <= 17; i++) {
						rc = add_connpt_name(model, y, x, pf("A%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("B%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("D%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("BCOUT%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
					for (i = 0; i <= 47; i++) {
						rc = add_connpt_name(model, y, x, pf("C%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("P%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("PCOUT%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
					for (i = 0; i <= 35; i++) {
						rc = add_connpt_name(model, y, x, pf("M%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
					for (i = 0; i <= 7; i++) {
						rc = add_connpt_name(model, y, x, pf("OPMODE%i_DSP48A1_SITE", i),
							dup_warn, 0, 0);
						if (rc) goto xout;
					}
				}
			}
		}
		if (is_atx(X_LOGIC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & (TF_LOGIC_XM_DEV|TF_LOGIC_XL_DEV)) {
					const char* pref[2];

					if (YX_TILE(model, y, x)->flags & TF_LOGIC_XM_DEV) {
						// The first SLICEM on the bottom has a given C_IN port.
						if (is_aty(Y_INNER_BOTTOM, model, y+3)) {
							rc = add_connpt_name(model, y, x, "M_CIN",
								dup_warn, 0, 0);
							if (rc) goto xout;
						}
						rc = add_connpt_name(model, y, x, "M_COUT",
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, "M_WE",
							dup_warn, 0, 0);
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							rc = add_connpt_name(model, y, x, pf("M_%cI", i),
								dup_warn, 0, 0);
							if (rc) goto xout;
						}
						pref[0] = "M";
						pref[1] = "X";
					} else { // LOGIC_XL
						rc = add_connpt_name(model, y, x, "XL_COUT",
							dup_warn, 0, 0);
						if (rc) goto xout;
						pref[0] = "L";
						pref[1] = "XX";
					}
					for (k = 0; k <= 1; k++) {
						rc = add_connpt_name(model, y, x, pf("%s_CE", pref[k]),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_SR", pref[k]),
							dup_warn, 0, 0);
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_CLK", pref[k]),
							dup_warn, 0, 0);
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							for (j = 1; j <= 6; j++) {
								rc = add_connpt_name(model, y, x, pf("%s_%c%i", pref[k], i, j),
									dup_warn, 0, 0);
								if (rc) goto xout;
							}
							rc = add_connpt_name(model, y, x, pf("%s_%c", pref[k], i),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cMUX", pref[k], i),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cQ", pref[k], i),
								dup_warn, 0, 0);
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cX", pref[k], i),
								dup_warn, 0, 0);
							if (rc) goto xout;
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
