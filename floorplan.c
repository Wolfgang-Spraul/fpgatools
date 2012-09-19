//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>

#include "model.h"
#include "control.h"
#include "floorplan.h"

void printf_version(FILE* f)
{
	fprintf(f, "fpga_floorplan_format 1\n");
}

#define PRINT_FLAG(fp, flag)	if ((tf) & (flag)) \
				  { fprintf (fp, " %s", #flag); tf &= ~(flag); }

int printf_tiles(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y;

	for (x = 0; x < model->x_width; x++) {
		fprintf(f, "\n");
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			if (tile->type != NA)
				fprintf(f, "tile y%02i x%02i name %s\n", y, x,
					fpga_tiletype_str(tile->type));
			if (tile->flags) {
				int tf = tile->flags;
				fprintf(f, "tile y%02i x%02i flags", y, x);

				PRINT_FLAG(f, TF_FABRIC_ROUTING_COL);
				PRINT_FLAG(f, TF_FABRIC_LOGIC_XM_COL);
				PRINT_FLAG(f, TF_FABRIC_LOGIC_XL_COL);
				PRINT_FLAG(f, TF_FABRIC_BRAM_VIA_COL);
				PRINT_FLAG(f, TF_FABRIC_MACC_VIA_COL);
				PRINT_FLAG(f, TF_FABRIC_BRAM_COL);
				PRINT_FLAG(f, TF_FABRIC_MACC_COL);
				PRINT_FLAG(f, TF_ROUTING_NO_IO);
				PRINT_FLAG(f, TF_BRAM_DEV);
				PRINT_FLAG(f, TF_MACC_DEV);
				PRINT_FLAG(f, TF_LOGIC_XL_DEV);
				PRINT_FLAG(f, TF_LOGIC_XM_DEV);
				PRINT_FLAG(f, TF_IOLOGIC_DELAY_DEV);
				PRINT_FLAG(f, TF_DCM_DEV);
				PRINT_FLAG(f, TF_PLL_DEV);
				PRINT_FLAG(f, TF_WIRED);

				if (tf) fprintf(f, " 0x%x", tf);
				fprintf(f, "\n");
			}
		}
	}
	return 0;
}

static int printf_IOB(FILE* f, struct fpga_model* model,
	int y, int x, int config_only)
{
	struct fpga_tile* tile;
	char pref[256];
	int type_count, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type != DEV_IOB)
			continue;
		if (config_only && !(tile->devs[i].instantiated)) {
			type_count++;
			continue;
		}
		snprintf(pref, sizeof(pref), "dev y%02i x%02i IOB %i",
			y, x, type_count);
		type_count++;

		if (!config_only) {
			fprintf(f, "%s type %s\n", pref,
				tile->devs[i].subtype == IOBM ? "M" : "S");
		}
		if (tile->devs[i].u.iob.istandard[0])
			fprintf(f, "%s istd %s\n", pref, 
				tile->devs[i].u.iob.istandard);
		if (tile->devs[i].u.iob.ostandard[0])
			fprintf(f, "%s ostd %s\n", pref, 
				tile->devs[i].u.iob.ostandard);
		switch (tile->devs[i].u.iob.bypass_mux) {
			case BYPASS_MUX_I:
				fprintf(f, "%s bypass_mux I\n", pref);
				break;
			case BYPASS_MUX_O:
				fprintf(f, "%s bypass_mux O\n", pref);
				break;
			case BYPASS_MUX_T:
				fprintf(f, "%s bypass_mux T\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].u.iob.I_mux) {
			case IMUX_I_B:
				fprintf(f, "%s imux I_B\n", pref);
				break;
			case IMUX_I: 
				fprintf(f, "%s imux I\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
		if (tile->devs[i].u.iob.drive_strength)
			fprintf(f, "%s strength %i\n", pref,
				tile->devs[i].u.iob.drive_strength);
		switch (tile->devs[i].u.iob.slew) {
			case SLEW_SLOW:
				fprintf(f, "%s slew SLOW\n", pref);
				break;
			case SLEW_FAST:
				fprintf(f, "%s slew FAST\n", pref);
				break;
			case SLEW_QUIETIO:
				fprintf(f, "%s slew QUIETIO\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
		if (tile->devs[i].u.iob.O_used)
			fprintf(f, "%s O_used\n", pref);
		switch (tile->devs[i].u.iob.suspend) {
			case SUSP_LAST_VAL:
				fprintf(f, "%s suspend DRIVE_LAST_VALUE\n", pref);
				break;
			case SUSP_3STATE:
				fprintf(f, "%s suspend 3STATE\n", pref);
				break;
			case SUSP_3STATE_PULLUP:
				fprintf(f, "%s suspend 3STATE_PULLUP\n", pref);
				break;
			case SUSP_3STATE_PULLDOWN:
				fprintf(f, "%s suspend 3STATE_PULLDOWN\n", pref);
				break;
			case SUSP_3STATE_KEEPER:
				fprintf(f, "%s suspend 3STATE_KEEPER\n", pref);
				break;
			case SUSP_3STATE_OCT_ON:
				fprintf(f, "%s suspend 3STATE_OCT_ON\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].u.iob.in_term) {
			case ITERM_NONE: 
				fprintf(f, "%s in_term NONE\n", pref);
				break;
			case ITERM_UNTUNED_25:
				fprintf(f, "%s in_term UNTUNED_SPLIT_25\n", pref);
				break;
			case ITERM_UNTUNED_50:
				fprintf(f, "%s in_term UNTUNED_SPLIT_50\n", pref);
				break;
			case ITERM_UNTUNED_75:
				fprintf(f, "%s in_term UNTUNED_SPLIT_75\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].u.iob.out_term) {
			case OTERM_NONE: 
				fprintf(f, "%s out_term NONE\n", pref);
				break;
			case OTERM_UNTUNED_25:
				fprintf(f, "%s out_term UNTUNED_25\n", pref);
				break;
			case OTERM_UNTUNED_50:
				fprintf(f, "%s out_term UNTUNED_50\n", pref);
				break;
			case OTERM_UNTUNED_75:
				fprintf(f, "%s out_term UNTUNED_75\n", pref);
				break;
			case 0: break; default: EXIT(1);
		}
	}
	return 0;
}

static int read_IOB_attr(struct fpga_model* model, struct fpga_device* dev,
	const char* w1, int w1_len, const char* w2, int w2_len)
{
	// First the one-word attributes.
	if (!str_cmp(w1, w1_len, "O_used", ZTERM)) {
		dev->u.iob.O_used = 1;
		goto inst_1;
	}
	// The remaining attributes all require 2 words.
	if (w2_len < 1) return 0;
	if (!str_cmp(w1, w1_len, "type", ZTERM))
		return 2; // no reason for instantiation
	if (!str_cmp(w1, w1_len, "istd", ZTERM)) {
		memcpy(dev->u.iob.istandard, w2, w2_len);
		dev->u.iob.istandard[w2_len] = 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "ostd", ZTERM)) {
		memcpy(dev->u.iob.ostandard, w2, w2_len);
		dev->u.iob.ostandard[w2_len] = 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "bypass_mux", ZTERM)) {
		if (!str_cmp(w2, w2_len, "I", ZTERM))
			dev->u.iob.bypass_mux = BYPASS_MUX_I;
		else if (!str_cmp(w2, w2_len, "O", ZTERM))
			dev->u.iob.bypass_mux = BYPASS_MUX_O;
		else if (!str_cmp(w2, w2_len, "T", ZTERM))
			dev->u.iob.bypass_mux = BYPASS_MUX_T;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "imux", ZTERM)) {
		if (!str_cmp(w2, w2_len, "I_B", ZTERM))
			dev->u.iob.I_mux = IMUX_I_B;
		else if (!str_cmp(w2, w2_len, "I", ZTERM))
			dev->u.iob.I_mux = IMUX_I;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "strength", ZTERM)) {
		dev->u.iob.drive_strength = to_i(w2, w2_len);
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "slew", ZTERM)) {
		if (!str_cmp(w2, w2_len, "SLOW", ZTERM))
			dev->u.iob.slew = SLEW_SLOW;
		else if (!str_cmp(w2, w2_len, "FAST", ZTERM))
			dev->u.iob.slew = SLEW_FAST;
		else if (!str_cmp(w2, w2_len, "QUIETIO", ZTERM))
			dev->u.iob.slew = SLEW_QUIETIO;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "suspend", 7)) {
		if (!str_cmp(w2, w2_len, "DRIVE_LAST_VALUE", ZTERM))
			dev->u.iob.suspend = SUSP_LAST_VAL;
		else if (!str_cmp(w2, w2_len, "3STATE", ZTERM))
			dev->u.iob.suspend = SUSP_3STATE;
		else if (!str_cmp(w2, w2_len, "3STATE_PULLUP", ZTERM))
			dev->u.iob.suspend = SUSP_3STATE_PULLUP;
		else if (!str_cmp(w2, w2_len, "3STATE_PULLDOWN", ZTERM))
			dev->u.iob.suspend = SUSP_3STATE_PULLDOWN;
		else if (!str_cmp(w2, w2_len, "3STATE_KEEPER", ZTERM))
			dev->u.iob.suspend = SUSP_3STATE_KEEPER;
		else if (!str_cmp(w2, w2_len, "3STATE_OCT_ON", ZTERM))
			dev->u.iob.suspend = SUSP_3STATE_OCT_ON;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "in_term", ZTERM)) {
		if (!str_cmp(w2, w2_len, "NONE", ZTERM))
			dev->u.iob.in_term = ITERM_NONE;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_25", ZTERM))
			dev->u.iob.in_term = ITERM_UNTUNED_25;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_50", ZTERM))
			dev->u.iob.in_term = ITERM_UNTUNED_50;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_75", ZTERM))
			dev->u.iob.in_term = ITERM_UNTUNED_75;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "out_term", ZTERM)) {
		if (!str_cmp(w2, w2_len, "NONE", ZTERM))
			dev->u.iob.out_term = OTERM_NONE;
		else if (!str_cmp(w2, w2_len, "UNTUNED_25", ZTERM))
			dev->u.iob.out_term = OTERM_UNTUNED_25;
		else if (!str_cmp(w2, w2_len, "UNTUNED_50", ZTERM))
			dev->u.iob.out_term = OTERM_UNTUNED_50;
		else if (!str_cmp(w2, w2_len, "UNTUNED_75", ZTERM))
			dev->u.iob.out_term = OTERM_UNTUNED_75;
		else return 0;
		goto inst_2;
	}
	return 0;
inst_1:
	dev->instantiated = 1;
	return 1;
inst_2:
	dev->instantiated = 2;
	return 2;
}

static int printf_LOGIC(FILE* f, struct fpga_model* model,
	int y, int x, int config_only)
{
	struct fpga_tile* tile;
	struct fpgadev_logic* cfg;
	char pref[256];
	int type_count, i, j, rc;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type != DEV_LOGIC)
			continue;
		if (config_only && !(tile->devs[i].instantiated)) {
			type_count++;
			continue;
		}
		snprintf(pref, sizeof(pref), "dev y%02i x%02i LOGIC %i",
			y, x, type_count);
		type_count++;

		if (!config_only) {
			switch (tile->devs[i].subtype) {
				case LOGIC_X:
					fprintf(f, "%s type X\n", pref);
					break;
				case LOGIC_L:
					fprintf(f, "%s type L\n", pref);
					break;
				case LOGIC_M:
					fprintf(f, "%s type M\n", pref);
					break;
				default: EXIT(1);
			}
		}
		cfg = &tile->devs[i].u.logic;
		for (j = LUT_A; j <= LUT_D; j++) {
			if (cfg->a2d[j].used)
				fprintf(f, "%s %c_used\n", pref, 'A'+j);
			if (cfg->a2d[j].lut6 && cfg->a2d[j].lut6[0])
				fprintf(f, "%s %c6_lut %s\n", pref, 'A'+j,
					cfg->a2d[j].lut6);
			if (cfg->a2d[j].lut5 && cfg->a2d[j].lut5[0])
				fprintf(f, "%s %c5_lut %s\n", pref, 'A'+j,
					cfg->a2d[j].lut5);

			switch (cfg->a2d[j].ff_mux) {
				case MUX_O6:
					fprintf(f, "%s %c_ffmux O6\n", pref, 'A'+j);
					break;
				case MUX_O5:
					fprintf(f, "%s %c_ffmux O5\n", pref, 'A'+j);
					break;
				case MUX_X:
					fprintf(f, "%s %c_ffmux X\n", pref, 'A'+j);
					break;
				case MUX_F7:
					fprintf(f, "%s %c_ffmux F7\n", pref, 'A'+j);
					break;
				case MUX_CY:
					fprintf(f, "%s %c_ffmux CY\n", pref, 'A'+j);
					break;
				case MUX_XOR:
					fprintf(f, "%s %c_ffmux XOR\n", pref, 'A'+j);
					break;
				case 0: break; default: FAIL(EINVAL);
			}
			switch (cfg->a2d[j].ff_srinit) {
				case FF_SRINIT0:
					fprintf(f, "%s %c_ffsrinit 0\n", pref, 'A'+j);
					break;
				case FF_SRINIT1:
					fprintf(f, "%s %c_ffsrinit 1\n", pref, 'A'+j);
					break;
				case 0: break; default: FAIL(EINVAL);
			}
			switch (cfg->a2d[j].out_mux) {
				case MUX_O6:
					fprintf(f, "%s %c_outmux O6\n", pref, 'A'+j);
					break;
				case MUX_O5:
					fprintf(f, "%s %c_outmux O5\n", pref, 'A'+j);
					break;
				case MUX_5Q:
					fprintf(f, "%s %c_outmux 5Q\n", pref, 'A'+j);
					break;
				case MUX_F7:
					fprintf(f, "%s %c_outmux F7\n", pref, 'A'+j);
					break;
				case MUX_CY:
					fprintf(f, "%s %c_outmux CY\n", pref, 'A'+j);
					break;
				case MUX_XOR:
					fprintf(f, "%s %c_outmux XOR\n", pref, 'A'+j);
					break;
				case 0: break; default: FAIL(EINVAL);
			}
			switch (cfg->a2d[j].ff) {
				case FF_OR2L:
					fprintf(f, "%s %c_ff OR2L\n", pref, 'A'+j);
					break;
				case FF_AND2L:
					fprintf(f, "%s %c_ff AND2L\n", pref, 'A'+j);
					break;
				case FF_LATCH:
					fprintf(f, "%s %c_ff LATCH\n", pref, 'A'+j);
					break;
				case FF_FF:
					fprintf(f, "%s %c_ff FF\n", pref, 'A'+j);
					break;
				case 0: break; default: FAIL(EINVAL);
			}
		}
		switch (cfg->clk_inv) {
			case CLKINV_B:
				fprintf(f, "%s clk CLK_B\n", pref);
				break;
			case CLKINV_CLK:
				fprintf(f, "%s clk CLK\n", pref);
				break;
			case 0: break; default: FAIL(EINVAL);
		}
		switch (cfg->sync_attr) {
			case SYNCATTR_SYNC:
				fprintf(f, "%s sync SYNC\n", pref);
				break;
			case SYNCATTR_ASYNC:
				fprintf(f, "%s sync ASYNC\n", pref);
				break;
			case 0: break; default: FAIL(EINVAL);
		}
		if (cfg->ce_used)
			fprintf(f, "%s ce_used\n", pref);
		if (cfg->sr_used)
			fprintf(f, "%s sr_used\n", pref);
		switch (cfg->we_mux) {
			case WEMUX_WE:
				fprintf(f, "%s wemux WE\n", pref);
				break;
			case WEMUX_CE:
				fprintf(f, "%s wemux CE\n", pref);
				break;
			case 0: break; default: FAIL(EINVAL);
		}
	}
	return 0;
fail:
	return rc;
}

static int read_LOGIC_attr(struct fpga_model* model, int y, int x, int type_idx,
	const char* w1, int w1_len, const char* w2, int w2_len)
{
	struct fpga_device* dev;
	char cmp_str[128];
	int i, rc;

	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) { HERE(); return 0; }

	// First the one-word attributes.
	for (i = LUT_A; i <= LUT_D; i++) {
		snprintf(cmp_str, sizeof(cmp_str), "%c_used", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			dev->u.logic.a2d[i].used = 1;
			goto inst_1;
		}
	}
	if (!str_cmp(w1, w1_len, "ce_used", ZTERM)) {
		dev->u.logic.ce_used = 1;
		goto inst_1;
	}
	if (!str_cmp(w1, w1_len, "sr_used", ZTERM)) {
		dev->u.logic.sr_used = 1;
		goto inst_1;
	}

	// The remaining attributes all require 2 words.
	if (w2_len < 1) return 0;
	if (!str_cmp(w1, w1_len, "type", ZTERM))
		return 2; // no reason for instantiation

	for (i = LUT_A; i <= LUT_D; i++) {
		snprintf(cmp_str, sizeof(cmp_str), "%c6_lut", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			rc = fdev_logic_set_lut(model, y, x, type_idx, i, 6, w2, w2_len);
			if (rc) return 0;
			goto inst_2;
		}
		snprintf(cmp_str, sizeof(cmp_str), "%c5_lut", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			rc = fdev_logic_set_lut(model, y, x, type_idx, i, 5, w2, w2_len);
			if (rc) return 0;
			goto inst_2;
		}
		snprintf(cmp_str, sizeof(cmp_str), "%c_ffmux", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			if (!str_cmp(w2, w2_len, "O6", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_O6;
			else if (!str_cmp(w2, w2_len, "O5", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_O5;
			else if (!str_cmp(w2, w2_len, "X", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_X;
			else if (!str_cmp(w2, w2_len, "F7", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_F7;
			else if (!str_cmp(w2, w2_len, "CY", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_CY;
			else if (!str_cmp(w2, w2_len, "XOR", ZTERM))
				dev->u.logic.a2d[i].ff_mux = MUX_XOR;
			else return 0;
			goto inst_2;
		}
		snprintf(cmp_str, sizeof(cmp_str), "%c_ffsrinit", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			if (!str_cmp(w2, w2_len, "0", ZTERM))
				dev->u.logic.a2d[i].ff_srinit = FF_SRINIT0;
			else if (!str_cmp(w2, w2_len, "1", ZTERM))
				dev->u.logic.a2d[i].ff_srinit = FF_SRINIT1;
			else return 0;
			goto inst_2;
		}
		snprintf(cmp_str, sizeof(cmp_str), "%c_outmux", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			if (!str_cmp(w2, w2_len, "O6", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_O6;
			if (!str_cmp(w2, w2_len, "O5", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_O5;
			if (!str_cmp(w2, w2_len, "5Q", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_5Q;
			if (!str_cmp(w2, w2_len, "F7", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_F7;
			if (!str_cmp(w2, w2_len, "CY", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_CY;
			if (!str_cmp(w2, w2_len, "XOR", ZTERM))
				dev->u.logic.a2d[i].out_mux = MUX_XOR;
			else return 0;
			goto inst_2;
		}
		snprintf(cmp_str, sizeof(cmp_str), "%c_ff", 'A'+i);
		if (!str_cmp(w1, w1_len, cmp_str, ZTERM)) {
			if (!str_cmp(w2, w2_len, "OR2L", ZTERM))
				dev->u.logic.a2d[i].ff = FF_OR2L;
			if (!str_cmp(w2, w2_len, "AND2L", ZTERM))
				dev->u.logic.a2d[i].ff = FF_AND2L;
			if (!str_cmp(w2, w2_len, "LATCH", ZTERM))
				dev->u.logic.a2d[i].ff = FF_LATCH;
			if (!str_cmp(w2, w2_len, "FF", ZTERM))
				dev->u.logic.a2d[i].ff = FF_FF;
			else return 0;
			goto inst_2;
		}
	}
	if (!str_cmp(w1, w1_len, "clk", ZTERM)) {
		if (!str_cmp(w2, w2_len, "CLK_B", ZTERM))
			dev->u.logic.clk_inv = CLKINV_B;
		else if (!str_cmp(w2, w2_len, "CLK", ZTERM))
			dev->u.logic.clk_inv = CLKINV_CLK;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "sync", ZTERM)) {
		if (!str_cmp(w2, w2_len, "SYNC", ZTERM))
			dev->u.logic.sync_attr = SYNCATTR_SYNC;
		else if (!str_cmp(w2, w2_len, "ASYNC", ZTERM))
			dev->u.logic.sync_attr = SYNCATTR_ASYNC;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "wemux", ZTERM)) {
		if (!str_cmp(w2, w2_len, "WE", ZTERM))
			dev->u.logic.we_mux = WEMUX_WE;
		else if (!str_cmp(w2, w2_len, "CE", ZTERM))
			dev->u.logic.we_mux = WEMUX_CE;
		else return 0;
		goto inst_2;
	}
	return 0;
inst_1:
	dev->instantiated = 1;
	return 1;
inst_2:
	dev->instantiated = 1;
	return 2;
}

int printf_devices(FILE* f, struct fpga_model* model, int config_only)
{
	int x, y, i, rc;
	struct fpga_tile* tile;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {

			rc = printf_IOB(f, model, y, x, config_only);
			if (rc) goto fail;
			rc = printf_LOGIC(f, model, y, x, config_only);
			if (rc) goto fail;

			tile = YX_TILE(model, y, x);
			for (i = 0; i < tile->num_devs; i++) {
				if (config_only && !(tile->devs[i].instantiated))
					continue;
				if (tile->devs[i].type == DEV_LOGIC
				    || tile->devs[i].type == DEV_IOB)
					continue; // handled above
				fprintf(f, "dev y%02i x%02i %s\n", y, x,
					fdev_type2str(tile->devs[i].type));
			}
		}
	}
	return 0;
fail:
	return rc;
}

int printf_ports(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	const char* conn_point_name_src;
	int x, y, i, conn_point_dests_o, num_dests_for_this_conn_point;
	int first_port_printed;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			first_port_printed = 0;
			for (i = 0; i < tile->num_conn_point_names; i++) {
				conn_point_dests_o = tile->conn_point_names[i*2];
				if (i < tile->num_conn_point_names-1)
					num_dests_for_this_conn_point = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
				else
					num_dests_for_this_conn_point = tile->num_conn_point_dests - conn_point_dests_o;
				if (num_dests_for_this_conn_point)
					// ports is only for connection-less endpoints
					continue;
				conn_point_name_src = strarray_lookup(&model->str, tile->conn_point_names[i*2+1]);
				if (!conn_point_name_src) {
					fprintf(stderr, "Cannot lookup src conn point name index %i, x%i y%i i%i\n",
						tile->conn_point_names[i*2+1], x, y, i);
					continue;
				}
				if (!first_port_printed) {
					first_port_printed = 1;
					fprintf(f, "\n");
				}
				fprintf(f, "port y%02i x%02i %s\n",
					y, x, conn_point_name_src);
			}
		}
	}
	return 0;
}

int printf_conns(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	char tmp_line[512];
	const char* conn_point_name_src, *other_tile_connpt_str;
	uint16_t other_tile_connpt_str_i;
	int x, y, i, j, k, conn_point_dests_o, num_dests_for_this_conn_point;
	int other_tile_x, other_tile_y, first_conn_printed;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			first_conn_printed = 0;
			for (i = 0; i < tile->num_conn_point_names; i++) {
				conn_point_dests_o = tile->conn_point_names[i*2];
				if (i < tile->num_conn_point_names-1)
					num_dests_for_this_conn_point = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
				else
					num_dests_for_this_conn_point = tile->num_conn_point_dests - conn_point_dests_o;
				if (!num_dests_for_this_conn_point)
					continue;
				conn_point_name_src = strarray_lookup(&model->str, tile->conn_point_names[i*2+1]);
				if (!conn_point_name_src) {
					fprintf(stderr, "Cannot lookup src conn point name index %i, x%i y%i i%i\n",
						tile->conn_point_names[i*2+1], x, y, i);
					continue;
				}
				for (j = 0; j < num_dests_for_this_conn_point; j++) {
					other_tile_x = tile->conn_point_dests[(conn_point_dests_o+j)*3];
					other_tile_y = tile->conn_point_dests[(conn_point_dests_o+j)*3+1];
					other_tile_connpt_str_i = tile->conn_point_dests[(conn_point_dests_o+j)*3+2];

					other_tile_connpt_str = strarray_lookup(&model->str, other_tile_connpt_str_i);
					if (!other_tile_connpt_str) {
						fprintf(stderr, "Lookup err line %i, dest pt %i, dest x%i y%i, from x%i y%i j%i num_dests %i src_pt %s\n",
							__LINE__, other_tile_connpt_str_i, other_tile_x, other_tile_y, x, y, j, num_dests_for_this_conn_point, conn_point_name_src);
						continue;
					}

					if (!first_conn_printed) {
						first_conn_printed = 1;
						fprintf(f, "\n");
					}
					sprintf(tmp_line, "conn y%02i x%02i %s ",
						y, x, conn_point_name_src);
					k = strlen(tmp_line);
					while (k < 45)
						tmp_line[k++] = ' ';
					sprintf(&tmp_line[k], "y%02i x%02i %s\n",
						other_tile_y, other_tile_x, other_tile_connpt_str);
					fprintf(f, tmp_line);
				}
			}
		}
	}
	return 0;
}

int printf_switches(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y, i, first_switch_printed;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);
			first_switch_printed = 0;
			for (i = 0; i < tile->num_switches; i++) {
				if (!first_switch_printed) {
					first_switch_printed = 1;	
					fprintf(f, "\n");
				}
				fprintf(f, "sw y%02i x%02i %s\n",
					y, x, fpga_switch_print(model, y, x, i));
			}
		}
	}
	return 0;
}

int printf_nets(FILE* f, struct fpga_model* model)
{
	net_idx_t net_i;
	int rc;

	net_i = NO_NET;
	while (!(rc = fnet_enum(model, net_i, &net_i)) && net_i != NO_NET)
		fnet_printf(f, model, net_i);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

static enum fpgadev_type to_type(const char* s, int len)
{
	if (!str_cmp(s, len, "IOB", 3)) return DEV_IOB;
	if (!str_cmp(s, len, "LOGIC", 5)) return DEV_LOGIC;
	return DEV_NONE;
}

static int coord(const char* s, int start, int* end, int* y, int* x)
{
	int y_beg, y_end, x_beg, x_end, rc;

	next_word(s, start, &y_beg, &y_end);
	next_word(s, y_end, &x_beg, &x_end);
	if (y_end < y_beg+2 || x_end < x_beg+2
	    || s[y_beg] != 'y' || s[x_beg] != 'x'
	    || !all_digits(&s[y_beg+1], y_end-y_beg-1)
	    || !all_digits(&s[x_beg+1], x_end-x_beg-1)) {
		FAIL(EINVAL);
	}
	*y = to_i(&s[y_beg+1], y_end-y_beg-1);
	*x = to_i(&s[x_beg+1], x_end-x_beg-1);
	*end = x_end;
	return 0;
fail:
	return rc;
}

static void read_net_line(struct fpga_model* model, const char* line, int start)
{
	int coord_end, y_coord, x_coord;
	int from_beg, from_end, from_str_i;
	int direction_beg, direction_end, is_bidir;
	int to_beg, to_end, to_str_i;
	int net_idx_beg, net_idx_end, el_type_beg, el_type_end;
	int dev_str_beg, dev_str_end, dev_type_idx_str_beg, dev_type_idx_str_end;
	int pin_str_beg, pin_str_end, pin_name_beg, pin_name_end;
	enum fpgadev_type dev_type;
	char buf[1024];
	net_idx_t net_idx;
	pinw_idx_t pinw_idx;
	int sw_is_bidir;

	// net lines will be one of the following three types:
	// in-port:  net 1 in y68 x13 LOGIC 1 pin D3
	// out-port: net 1 out y72 x12 IOB 0 pin I
	// switch:   net 1 sw y72 x12 BIOB_IBUF0_PINW -> BIOB_IBUF0

	next_word(line, start, &net_idx_beg, &net_idx_end);
	if (net_idx_end == net_idx_beg
	    || !all_digits(&line[net_idx_beg], net_idx_end-net_idx_beg))
		{ HERE(); return; }
	net_idx = to_i(&line[net_idx_beg], net_idx_end-net_idx_beg);
	if (net_idx < 1)
		{ HERE(); return; }

	next_word(line, net_idx_end, &el_type_beg, &el_type_end);
	if (!str_cmp(&line[el_type_beg], el_type_end-el_type_beg, "sw", 2)) {
		struct sw_set sw;

		if (coord(line, el_type_end, &coord_end, &y_coord, &x_coord))
			return;

		next_word(line, coord_end, &from_beg, &from_end);
		next_word(line, from_end, &direction_beg, &direction_end);
		next_word(line, direction_end, &to_beg, &to_end);

		if (from_end <= from_beg || direction_end <= direction_beg
		    || to_end <= to_beg) {
			HERE();
			return;
		}
		memcpy(buf, &line[from_beg], from_end-from_beg);
		buf[from_end-from_beg] = 0;
		from_str_i = strarray_find(&model->str, buf);
		if (from_str_i == STRIDX_NO_ENTRY) {
			HERE();
			return;
		}
		if (!str_cmp(&line[direction_beg], direction_end-direction_beg,
			"->", 2))
			is_bidir = 0;
		else if (!str_cmp(&line[direction_beg], direction_end-direction_beg,
			"<->", 3))
			is_bidir = 1;
		else {
			HERE();
			return;
		}
		memcpy(buf, &line[to_beg], to_end-to_beg);
		buf[to_end-to_beg] = 0;
		to_str_i = strarray_find(&model->str, buf);
		if (to_str_i == STRIDX_NO_ENTRY) {
			HERE();
			return;
		}

		sw.sw[0] = fpga_switch_lookup(model, y_coord, x_coord, from_str_i, to_str_i);
		if (sw.sw[0] == NO_SWITCH) {
			HERE();
			return;
		}
		sw_is_bidir = fpga_switch_is_bidir(model, y_coord, x_coord, sw.sw[0]);
		if ((is_bidir && !sw_is_bidir)
		    || (!is_bidir && sw_is_bidir)) {
			HERE();
			return;
		}
		if (fpga_switch_is_used(model, y_coord, x_coord, sw.sw[0]))
			HERE();
		sw.len = 1;
		if (fnet_add_sw(model, net_idx, y_coord, x_coord, sw.sw, sw.len))
			HERE();
		return;
	}

	if (str_cmp(&line[el_type_beg], el_type_end-el_type_beg, "in", 2)
	    && str_cmp(&line[el_type_beg], el_type_end-el_type_beg, "out", 3))
		{ HERE(); return; }

	if (coord(line, el_type_end, &coord_end, &y_coord, &x_coord))
		return;

	next_word(line, coord_end, &dev_str_beg, &dev_str_end);
	next_word(line, dev_str_end, &dev_type_idx_str_beg, &dev_type_idx_str_end);
	next_word(line, dev_type_idx_str_end, &pin_str_beg, &pin_str_end);
	next_word(line, pin_str_end, &pin_name_beg, &pin_name_end);
	if (dev_str_end <= dev_str_beg
	    || dev_type_idx_str_end <= dev_type_idx_str_beg
	    || pin_str_end <= pin_str_beg
	    || pin_name_end <= pin_name_beg
	    || !all_digits(&line[dev_type_idx_str_beg], dev_type_idx_str_end-dev_type_idx_str_beg)
	    || str_cmp(&line[pin_str_beg], pin_str_end-pin_str_beg, "pin", 3))
		{ HERE(); return; }
	dev_type = fdev_str2type(&line[dev_str_beg], dev_str_end-dev_str_beg);
	if (dev_type == DEV_NONE) { HERE(); return; }
	pinw_idx = fdev_pinw_str2idx(dev_type, &line[pin_name_beg],
		pin_name_end-pin_name_beg);
	if (pinw_idx == PINW_NO_IDX) { HERE(); return; }
	if (fnet_add_port(model, net_idx, y_coord, x_coord, dev_type,
		to_i(&line[dev_type_idx_str_beg], dev_type_idx_str_end
			-dev_type_idx_str_beg), pinw_idx))
		HERE();
}

static void read_dev_line(struct fpga_model* model, const char* line, int start)
{
	int coord_end, y_coord, x_coord;
	int type_beg, type_end, idx_beg, idx_end;
	enum fpgadev_type dev_type;
	int dev_type_idx, dev_idx, words_consumed;
	struct fpga_device* dev_ptr;
	int next_beg, next_end, second_beg, second_end;

	if (coord(line, start, &coord_end, &y_coord, &x_coord))
		return;

	next_word(line, coord_end, &type_beg, &type_end);
	next_word(line, type_end, &idx_beg, &idx_end);

	if (type_end == type_beg || idx_end == idx_beg
	    || !all_digits(&line[idx_beg], idx_end-idx_beg)) {
		fprintf(stderr, "error %i: %s", __LINE__, line);
		return;
	}
	dev_type = to_type(&line[type_beg], type_end-type_beg);
	dev_type_idx = to_i(&line[idx_beg], idx_end-idx_beg);
	dev_idx = fpga_dev_idx(model, y_coord, x_coord, dev_type, dev_type_idx);
	if (dev_idx == NO_DEV) {
		fprintf(stderr, "error %i: %s", __LINE__, line);
		return;
	}
	dev_ptr = FPGA_DEV(model, y_coord, x_coord, dev_idx);

	next_end = idx_end;
	while (next_word(line, next_end, &next_beg, &next_end),
		next_end > next_beg) {
		next_word(line, next_end, &second_beg, &second_end);
		switch (dev_type) {
			case DEV_IOB:
				words_consumed = read_IOB_attr(model, dev_ptr,
					&line[next_beg], next_end-next_beg,
					&line[second_beg],
					second_end-second_beg);
				break;
			case DEV_LOGIC:
				words_consumed = read_LOGIC_attr(model, y_coord,
					x_coord, dev_type_idx,
					&line[next_beg], next_end-next_beg,
					&line[second_beg],
					second_end-second_beg);
				break;
			default:
				fprintf(stderr, "error %i: %s", __LINE__, line);
				return;
		}
		if (!words_consumed)
			fprintf(stderr, "error %i w1 %.*s w2 %.*s: %s",
				__LINE__, next_end-next_beg, &line[next_beg],
				second_end-second_beg, &line[second_beg], line);
		else if (words_consumed == 2)
			next_end = second_end;
	}
}

int read_floorplan(struct fpga_model* model, FILE* f)
{
	char line[1024];
	int beg, end;

	while (fgets(line, sizeof(line), f)) {
		next_word(line, 0, &beg, &end);
		if (end == beg) continue;

		if (end-beg == 3
		    && !str_cmp(&line[beg], 3, "net", 3)) {
			read_net_line(model, line, end);
		}
		if (end-beg == 3
		    && !str_cmp(&line[beg], 3, "dev", 3)) {
			read_dev_line(model, line, end);
		}
	}
	return 0;
}

int write_floorplan(FILE* f, struct fpga_model* model, int flags)
{
	int rc;

	if (!(flags & FP_NO_HEADER))
		printf_version(f);

	rc = printf_devices(f, model, /*config_only*/ 1);
	if (rc) FAIL(rc);

	rc = printf_nets(f, model);
	if (rc) FAIL(rc);

	return 0;
fail:
	return rc;
}
