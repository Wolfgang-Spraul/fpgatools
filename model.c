//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

static int init_tiles(struct fpga_model* model);
static int init_wires(struct fpga_model* model);
static int init_ports(struct fpga_model* model);
static int init_devices(struct fpga_model* model);
static int init_switches(struct fpga_model* model);

static int run_gclk(struct fpga_model* model);
static int run_gclk_horiz_regs(struct fpga_model* model);
static int run_gclk_vert_regs(struct fpga_model* model);
static int run_logic_inout(struct fpga_model* model);
static int run_direction_wires(struct fpga_model* model);

static const char* pf(const char* fmt, ...);
static const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name);
static char next_non_whitespace(const char* s);
static char last_major(const char* str, int cur_o);
static int add_connpt_name(struct fpga_model* model, int y, int x, const char* connpt_name);

typedef int (*add_conn_f)(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
#define NOPREF_BI_F	add_conn_bi
#define PREF_BI_F	add_conn_bi_pref
#define NOPREF_UNI_F	add_conn_uni
#define PREF_UNI_F	add_conn_uni_pref

static int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
int add_conn_uni_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_bi_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2);
static int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func, int y1, int x1, const char* name1, int start1, int last1, int y2, int x2, const char* name2, int start2);

struct w_point // wire point
{
	const char* name;
	int start_count; // if there is a %i in the name, this is the start number
	int y, x;
};

struct w_net
{
	// if !last_inc, no incrementing will happen
	// if last_inc > 0, incrementing will happen to
	// the %i in the name from 0:last_inc, for a total
	// of last_inc+1 wires.
	int last_inc;
	struct w_point pts[40];
};

static int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net);

struct seed_data
{
	int x_flags;
	const char* str;
};

static void seed_strx(struct fpga_model* model, struct seed_data* data);

int fpga_build_model(struct fpga_model* model, int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring)
{
	int rc;

	memset(model, 0, sizeof(*model));
	model->cfg_rows = fpga_rows;
	strncpy(model->cfg_columns, columns, sizeof(model->cfg_columns)-1);
	strncpy(model->cfg_left_wiring, left_wiring,
		sizeof(model->cfg_left_wiring)-1);
	strncpy(model->cfg_right_wiring, right_wiring,
		sizeof(model->cfg_right_wiring)-1);
	strarray_init(&model->str, STRIDX_64K);

	rc = init_tiles(model);
	if (rc) return rc;

	rc = init_wires(model);
	if (rc) return rc;

	rc = init_ports(model);
	if (rc) return rc;

	rc = init_devices(model);
	if (rc) return rc;

	rc = init_switches(model);
	if (rc) return rc;

	return 0;
}

void fpga_free_model(struct fpga_model* model)
{
	if (!model) return;
	free(model->tmp_str);
	strarray_free(&model->str);
	free(model->tiles);
	memset(model, 0, sizeof(*model));
}

static int init_switches(struct fpga_model* model)
{
	return 0;
}

static int init_devices(struct fpga_model* model)
{
	return 0;
}

static int init_ports(struct fpga_model* model)
{
	int x, y, i, j, k, rc;

	for (x = 0; x < model->tile_x_range; x++) {
		if (is_atx(X_ROUTING_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->tile_y_range - BOTTOM_IO_TILES; y++) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
						model, y))
					continue;
				rc = add_connpt_name(model, y, x, "VCC_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "GND_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "KEEP1_WIRE");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN");
				if (rc) goto xout;
				rc = add_connpt_name(model, y, x, "FAN_B");
				if (rc) goto xout;
			}
		}
		if (is_atx(X_FABRIC_BRAM_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->tile_y_range - BOTTOM_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_BRAM_DEV) {

					static const char* pass_str[3] = {"RAMB16BWER", "RAMB8BWER_0", "RAMB8BWER_1"};
					// pass 0 is ramb16, pass 1 and 2 are for ramb8
					for (i = 0; i <= 2; i++) {
						for (j = 'A'; j <= 'B'; j++) {
							rc = add_connpt_name(model, y, x, pf("%s_CLK%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_EN%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_REGCE%c", pass_str[i], j));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_RST%c", pass_str[i], j));
							if (rc) goto xout;
							for (k = 0; k <= (!i ? 3 : 1); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DIP%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DOP%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_WE%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 13 : 12); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_ADDR%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
							for (k = 0; k <= (!i ? 31 : 15); k++) {
								rc = add_connpt_name(model, y, x, pf("%s_DI%c%i", pass_str[i], j, k));
								if (rc) goto xout;
								rc = add_connpt_name(model, y, x, pf("%s_DO%c%i", pass_str[i], j, k));
								if (rc) goto xout;
							}
						}
					}
				}
			}
		}
		if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->tile_y_range - BOTTOM_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & TF_MACC_DEV) {
					static const char* pref[] = {"CE", "RST", ""};
					static const char* seq[] = {"A", "B", "C", "D", "M", "P", "OPMODE", ""};

					rc = add_connpt_name(model, y, x, "CLK_DSP48A1_SITE");
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUT_DSP48A1_SITE");
					if (rc) goto xout;
					rc = add_connpt_name(model, y, x, "CARRYOUTF_DSP48A1_SITE");
					if (rc) goto xout;

					for (i = 0; pref[i][0]; i++) {
						rc = add_connpt_name(model, y, x, pf("%sCARRYIN_DSP48A1_SITE", pref[i]));
						if (rc) goto xout;
						for (j = 0; seq[j][0]; j++) {
							rc = add_connpt_name(model, y, x, pf("%s%s_DSP48A1_SITE", pref[i], seq[j]));
							if (rc) goto xout;
						}
					}
						
					for (i = 0; i <= 17; i++) {
						rc = add_connpt_name(model, y, x, pf("A%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("B%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("D%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("BCOUT%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 47; i++) {
						rc = add_connpt_name(model, y, x, pf("C%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("P%i_DSP48A1_SITE", i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("PCOUT%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 35; i++) {
						rc = add_connpt_name(model, y, x, pf("M%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
					for (i = 0; i <= 7; i++) {
						rc = add_connpt_name(model, y, x, pf("OPMODE%i_DSP48A1_SITE", i));
						if (rc) goto xout;
					}
				}
			}
		}
		if (is_atx(X_LOGIC_COL, model, x)) {
			for (y = TOP_IO_TILES; y < model->tile_y_range - BOTTOM_IO_TILES; y++) {
				if (YX_TILE(model, y, x)->flags & (TF_LOGIC_XM_DEV|TF_LOGIC_XL_DEV)) {
					const char* pref[2];

					if (YX_TILE(model, y, x)->flags & TF_LOGIC_XM_DEV) {
						pref[0] = "M";
						pref[1] = "X";
						rc = add_connpt_name(model, y, x, "M_COUT");
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, "M_WE");
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							rc = add_connpt_name(model, y, x, pf("M_%cI", i));
							if (rc) goto xout;
						}
					} else { // LOGIC_XL
						pref[0] = "L";
						pref[1] = "XX";
						rc = add_connpt_name(model, y, x, "XL_COUT");
						if (rc) goto xout;
					}
					for (k = 0; k <= 1; k++) {
						rc = add_connpt_name(model, y, x, pf("%s_CE", pref[k], i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_SR", pref[k], i));
						if (rc) goto xout;
						rc = add_connpt_name(model, y, x, pf("%s_CLK", pref[k], i));
						if (rc) goto xout;
						for (i = 'A'; i <= 'D'; i++) {
							for (j = 1; j <= 6; j++) {
								rc = add_connpt_name(model, y, x, pf("%s_%c%i", pref[k], i, j));
								if (rc) goto xout;
							}
							rc = add_connpt_name(model, y, x, pf("%s_%c", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cMUX", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cQ", pref[k], i));
							if (rc) goto xout;
							rc = add_connpt_name(model, y, x, pf("%s_%cX", pref[k], i));
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

static int init_wires(struct fpga_model* model)
{
	int rc;

	rc = run_gclk(model);
	if (rc) goto xout;

	rc = run_logic_inout(model);
	if (rc) goto xout;

	rc = run_direction_wires(model);
	if (rc) goto xout;

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
			} else if (is_atx(X_LOGIC_COL|X_LEFT_IO_DEVS_COL, model, x)) {
				gclk_net.pts[next_net_o++].name = "HCLK_GCLK%i_CLB";
			} else if (is_atx(X_FABRIC_BRAM_MACC_ROUTING_COL, model, x)) {
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
			if (x >= model->tile_x_range) {
				fprintf(stderr, "Internal error in line %i\n", __LINE__);
				goto xout;
			}
		}
	}
	for (x = 0; x < model->tile_x_range; x++) {
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
					else if (is_atx(X_ROUTING_TO_BRAM_COL|X_ROUTING_TO_MACC_COL, model, x))
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
	char* gclk_sep_str;

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
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
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

		for (x = 0; x < model->tile_x_range; x++) {
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
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
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

		for (x = 0; x < model->tile_x_range; x++) {
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
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
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
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
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
			for (x = 0; x < model->tile_x_range; x++) {
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
			  | X_FABRIC_BRAM_MACC_ROUTING_COL
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
	
		left_half = 1;
		seed_strx(model, ckpin_seeds);
		for (x = 0; x < model->tile_x_range; x++) {
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
		{{ "REGH_RIOI_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_IO_DEVS_O },
		 { "MCB_REGH_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_MCB_O },
		 { "REGR_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_OUTER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{
		const char* str[5] = {"REGH_IOI_INT_GCLK%i", "REGH_RIOI_GCLK%i", "MCB_REGH_GCLK%i", "REGR_RTERM_GCLK%i", "REGR_GCLK%i"};
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->center_y-2, model->tile_x_range-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
			model->center_y-1, model->tile_x_range-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2))) goto xout;
		for (x = model->tile_x_range-RIGHT_IO_ROUTING_O; x <= model->tile_x_range-RIGHT_OUTER_O; x++) {
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-1, model->tile_x_range-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i_EXT", 2, 3,
				model->center_y, x, str[x-(model->tile_x_range-RIGHT_IO_ROUTING_O)], 2))) goto xout;
			if ((rc = add_conn_range(model, NOPREF_BI_F,
				model->center_y-2, model->tile_x_range-RIGHT_IO_ROUTING_O, "INT_BUFPLL_GCLK%i", 2, 3,
				model->center_y, x, str[x-(model->tile_x_range-RIGHT_IO_ROUTING_O)], 2))) goto xout;
		}
	}
	{ struct w_net net = {
		1,
		{{ "INT_BUFPLL_GCLK%i", 0, model->center_y-1, model->tile_x_range-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_INT_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_IO_DEVS_O },
		 { "REGH_RTERM_GCLK%i", 0, model->center_y, model->tile_x_range-RIGHT_INNER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	{ struct w_net net = {
		1,
		{{ "REGH_IOI_INT_GCLK%i", 2, model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O },
		 { "REGH_RIOI_GCLK%i", 2, model->center_y, model->tile_x_range-RIGHT_IO_DEVS_O },
		 { "REGR_RTERM_GCLK%i", 2, model->center_y, model->tile_x_range-RIGHT_INNER_O },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

	// the naming is a little messed up here, and the networks are
	// actually simpler than represented here (with full 0:3 connections).
	// But until we have better representation of wire networks, this has
	// to suffice.
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y-1, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_OUTER_O,
			"REGR_GCLK%i", 0, 1,
		model->center_y, model->tile_x_range-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_OUTER_O,
			"REGR_GCLK%i", 2, 3,
		model->center_y, model->tile_x_range-RIGHT_INNER_O,
			"REGR_RTERM_GCLK%i", 2))) goto xout;
	// same from MCB_REGH_GCLK...
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y-1, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"INT_BUFPLL_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"REGH_RIOI_INT_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->tile_x_range-RIGHT_IO_ROUTING_O,
			"REGH_IOI_INT_GCLK%i", 2))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 0, 1,
		model->center_y, model->tile_x_range-RIGHT_INNER_O,
			"REGH_RTERM_GCLK%i", 0))) goto xout;
	if ((rc = add_conn_range(model, NOPREF_BI_F,
		model->center_y, model->tile_x_range-RIGHT_MCB_O,
			"MCB_REGH_GCLK%i", 2, 3,
		model->center_y, model->tile_x_range-RIGHT_INNER_O,
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
	{ struct w_net net = {
		1,
		{{ "REGT_GCLK%i",	0, TOP_OUTER_ROW, model->center_x-1 },
		 { "REGT_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x-1 },
		 { "REGV_TTERM_GCLK%i", 0, TOP_INNER_ROW, model->center_x },
		 { "BUFPLL_TOP_GCLK%i", 0, TOP_INNER_ROW, model->center_x+1 },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }
	{ struct w_net net = {
		1,
		{{ "REGB_GCLK%i",	0, model->tile_y_range-1, model->center_x-1 },
		 { "REGB_BTERM_GCLK%i", 0, model->tile_y_range-2, model->center_x-1 },
		 { "REGV_BTERM_GCLK%i", 0, model->tile_y_range-2, model->center_x },
		 { "BUFPLL_BOT_GCLK%i", 0, model->tile_y_range-2, model->center_x+1 },
		 { "" }}};
	if ((rc = add_conn_net(model, NOPREF_BI_F, &net))) goto xout; }

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
	for (i = model->tile_y_range-2-1; i >= model->tile_y_range-2-HALF_ROW-1; i--) {
		if ((rc = add_conn_range(model, NOPREF_BI_F,
			model->tile_y_range-2, model->center_x+1,
				"IOI_BTERM_GCLK%i",  0, 15,
			i, model->center_x+1,
				(i == model->tile_y_range-2-HALF_ROW-1) ?
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

	for (y = 0; y < model->tile_y_range; y++) {
		for (x = 0; x < model->tile_x_range; x++) {
			tile = &model->tiles[y * model->tile_x_range + x];

			// LOGICOUT
			if (tile[1].flags & TF_LOGIC_XM_DEV) {
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXM_LOGICOUT%i", 0))) goto xout;
			}
			if (tile[1].flags & TF_LOGIC_XL_DEV) {
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "CLEXL_LOGICOUT%i", 0))) goto xout;
			}
			if (tile[1].flags & TF_IOLOGIC_DELAY_DEV) {
				if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICOUT%i", 0, 23, y, x+1, "IOI_LOGICOUT%i", 0))) goto xout;
			}

			// LOGICIN
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
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
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
						if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x))
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
				if (is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
					if ((rc = add_conn_range(model, NOPREF_BI_F, y, x, "LOGICIN_B%i", 0, 62, y, x+1, "INT_INTERFACE_LOGICBIN%i", 0))) goto xout;
					if (tile[2].flags & TF_BRAM_DEV) {
						for (i = 0; i < 4; i++) {
							sprintf(buf, "BRAM_LOGICINB%%i_INT%i", 3-i);
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x, "LOGICIN_B%i", 0, 62, y, x+2, buf, 0))) goto xout;
							if ((rc = add_conn_range(model, NOPREF_BI_F, y-(3-i), x+1, "INT_INTERFACE_LOGICBIN%i", 0, 62, y, x+2, buf, 0))) goto xout;
						}
					}
				}
				if (is_atx(X_ROUTING_TO_MACC_COL, model, x)) {
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

static int run_direction_wires(struct fpga_model* model)
{
	int x, y, rc;

	for (y = 0; y < model->tile_y_range; y++) {
		for (x = 0; x < model->tile_x_range; x++) {
			// NR1
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "NR1E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else {
					{ struct w_net net = {
						3,
						{{ "NR1B%i", 0,   y, x },
						 { "NR1E%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if (is_aty(Y_INNER_BOTTOM, model, y+1) && !is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						{ struct w_net net = {
							3,
							{{ "NR1E%i", 0,   y, x },
							 { "NR1E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					}
				}
			}

			// NN2
			if (is_atyx(YX_ROUTING_TILE, model, y, x)) {
				if (is_aty(Y_INNER_TOP, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2B%i", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					{ struct w_net net = {
						0,
						{{ "NN2E_S0", 0,   y, x },
						 { "NN2E_S0", 0, y-1, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_INNER_TOP, model, y-2)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-1)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2M%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model, y-1, x, "NN2M0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y-2)) {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "NN2E%i", 0, y-3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model,   y, x, "NN2B0", y-2, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E0",   y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2M0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-2, x, "NN2E_S0", y-3, x, "NN2E0"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y-3, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
				} else {
					{ struct w_net net = {
						3,
						{{ "NN2B%i", 0,   y, x },
						 { "NN2M%i", 0, y-1, x },
						 { "NN2E%i", 0, y-2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi(model,   y, x, "NN2B0", y-1, x, "NN2E_S0"))) goto xout;
					if ((rc = add_conn_bi(model, y-2, x, "NN2E0", y-1, x, "NN2E_S0"))) goto xout;
					if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
						if ((rc = add_conn_bi(model, y, x, "NN2E_S0", y-1, x, "NN2E0"))) goto xout;
						if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
							{ struct w_net net = {
								3,
								{{ "NN2E%i", 0, y-1, x },
								 { "NN2M%i", 0,   y, x },
								 { "NN2M%i", 0, y+1, x },
								 { "" }}};
							if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
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
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+1, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2E_N3"))) goto xout;

					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;

					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+2, x, "SS2M3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+1, x, "SS2M3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2B3",   y+3, x, "SS2E_N3"))) goto xout;
				} else if (is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS, model, y+1)) {
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2B%i", 0, y+1, x },
						 { "SS2M%i", 0, y+2, x },
						 { "SS2E%i", 0, y+3, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					if ((rc = add_conn_bi_pref(model,   y, x, "SS2B3",   y+2, x, "SS2E_N3"))) goto xout;
					if ((rc = add_conn_bi_pref(model, y+2, x, "SS2E_N3", y+3, x, "SS2E3"))) goto xout;
				} else if (is_aty(Y_INNER_BOTTOM, model, y+2)) {
					if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						{ struct w_net net = {
							3,
							{{ "SS2B%i", 0,   y, x },
							 { "SS2M%i", 0, y+1, x },
							 { "SS2M%i", 0, y+2, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
					}
				} else if (is_aty(Y_INNER_BOTTOM, model, y+1)) {
					if (!is_atx(X_ROUTING_TO_BRAM_COL, model, x)) {
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2B%i", 0, 3, y+1, x, "SS2B%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E_N3"))) goto xout;
					}
				} else {
					if (is_aty(Y_INNER_TOP, model, y-1)) {
						{ struct w_net net = {
							3,
							{{ "SS2M%i", 0, y-1, x },
							 { "SS2M%i", 0,   y, x },
							 { "SS2E%i", 0, y+1, x },
							 { "" }}};
						if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
						if ((rc = add_conn_range(model, PREF_BI_F,   y, x, "SS2E%i", 0, 3, y-1, x, "SS2E%i", 0))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E3",   y-1, x, "SS2E_N3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y-1, x, "SS2M3"))) goto xout;
						if ((rc = add_conn_bi_pref(model,   y, x, "SS2E_N3", y+1, x, "SS2E3"))) goto xout;
					}
					{ struct w_net net = {
						3,
						{{ "SS2B%i", 0,   y, x },
						 { "SS2M%i", 0, y+1, x },
						 { "SS2E%i", 0, y+2, x },
						 { "" }}};
					if ((rc = add_conn_net(model, PREF_BI_F, &net))) goto xout; }
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

static int init_tiles(struct fpga_model* model)
{
	int tile_rows, tile_columns, i, j, k, l, row_top_y, left_side;
	int start, end, no_io;
	char cur_cfgcol, last_col;
	struct fpga_tile* tile_i0;

	tile_rows = 1 /* middle */ + (8+1+8)*model->cfg_rows + 2+2 /* two extra tiles at top and bottom */;
	tile_columns = 5 /* left */ + 5 /* right */;
	for (i = 0; model->cfg_columns[i] != 0; i++) {
		if (model->cfg_columns[i] == 'L' || model->cfg_columns[i] == 'M')
			tile_columns += 2; // 2 for logic blocks L/M
		else if (model->cfg_columns[i] == 'B' || model->cfg_columns[i] == 'D')
			tile_columns += 3; // 3 for bram or macc
		else if (model->cfg_columns[i] == 'R')
			tile_columns += 2+2; // 2+2 for middle IO+logic+PLL/DCM
	}
	model->tmp_str = malloc((tile_columns > tile_rows ? tile_columns : tile_rows) * sizeof(*model->tmp_str));
	if (!model->tmp_str) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return -1;
	}
	model->tile_x_range = tile_columns;
	model->tile_y_range = tile_rows;
	model->center_x = -1;
	model->tiles = calloc(tile_columns * tile_rows, sizeof(struct fpga_tile));
	if (!model->tiles) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return -1;
	}
	for (i = 0; i < tile_rows * tile_columns; i++)
		model->tiles[i].type = NA;
	if (!(tile_rows % 2))
		fprintf(stderr, "Unexpected even number of tile rows (%i).\n", tile_rows);
	model->center_y = 2 /* top IO files */ + (model->cfg_rows/2)*(8+1/*middle of row clock*/+8);

	//
	// top, bottom, center:
	// go through columns from left to right, rows from top to bottom
	//

	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
	i = 5; // skip left IO columns
	for (j = 0; model->cfg_columns[j]; j++) {
		cur_cfgcol = model->cfg_columns[j];
		switch (cur_cfgcol) {
			case 'L':
			case 'l':
			case 'M':
			case 'm':
				no_io = (next_non_whitespace(&model->cfg_columns[j+1]) == 'n');
				last_col = last_major(model->cfg_columns, j);

				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i+1].flags |= TF_FABRIC_LOGIC_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles (center row)
					start = ((k == model->cfg_rows-1 && !no_io) ? 2 : 0);
					end = ((k == 0 && !no_io) ? 14 : 16);
					for (l = start; l < end; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15 || (!k && no_io))
							tile_i0->type = ROUTING;
						else
							tile_i0->type = ROUTING_BRK;
						if (cur_cfgcol == 'L') {
							tile_i0[1].flags |= TF_LOGIC_XL_DEV;
							tile_i0[1].type = LOGIC_XL;
						} else {
							tile_i0[1].flags |= TF_LOGIC_XM_DEV;
							tile_i0[1].type = LOGIC_XM;
						}
					}
					if (cur_cfgcol == 'L') {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					} else {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XM;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XM;
					}
				}

				if (last_col == 'R') {
					model->tiles[tile_columns + i].type = IO_BUFPLL_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i].type = IO_BUFPLL_TERM_B;
				} else {
					model->tiles[tile_columns + i].type = IO_TERM_T;
					if (!no_io)
						model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i].type = LOGIC_ROUTING_TERM_B;
				}
				if (!no_io) {
					model->tiles[i].type = IO_T;
					model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
					model->tiles[2*tile_columns + i].type = IO_ROUTING;
					model->tiles[3*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;
				}

				if (last_col == 'R') {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				} else {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_TERM_T;
					if (!no_io)
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = LOGIC_NOIO_TERM_B;
				}
				if (!no_io) {
					model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
					model->tiles[2*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
					model->tiles[3*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				}

				if (cur_cfgcol == 'L') {
					model->tiles[model->center_y*tile_columns + i].type = REGH_ROUTING_XL;
					model->tiles[model->center_y*tile_columns + i + 1].type = REGH_LOGIC_XL;
				} else {
					model->tiles[model->center_y*tile_columns + i].type = REGH_ROUTING_XM;
					model->tiles[model->center_y*tile_columns + i + 1].type = REGH_LOGIC_XM;
				}
				i += 2;
				break;
			case 'B':
				if (next_non_whitespace(&model->cfg_columns[j+1]) == 'g') {
					if (left_side)
						model->left_gclk_sep_x = i+2;
					else
						model->right_gclk_sep_x = i+2;
				}
				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i+1].flags |= TF_FABRIC_BRAM_MACC_ROUTING_COL;
				model->tiles[i+2].flags |= TF_FABRIC_BRAM_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15)
							tile_i0->type = BRAM_ROUTING;
						else
							tile_i0->type = BRAM_ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4)) {
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = BRAM;
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].flags |= TF_BRAM_DEV;
						}
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_BRAM_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_BRAM_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_BRAM;
				}

				model->tiles[tile_columns + i].type = BRAM_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = BRAM_ROUTING_TERM_B;
				model->tiles[tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = BRAM_ROUTING_VIA_TERM_B;
				model->tiles[tile_columns + i + 2].type = left_side ? BRAM_TERM_LT : BRAM_TERM_RT;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? BRAM_TERM_LB : BRAM_TERM_RB;

				model->tiles[model->center_y*tile_columns + i].type = REGH_BRAM_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGH_BRAM_ROUTING_VIA;
				model->tiles[model->center_y*tile_columns + i + 2].type = left_side ? REGH_BRAM_L : REGH_BRAM_R;
				i += 3;
				break;
			case 'D':
				model->tiles[i].flags |= TF_FABRIC_ROUTING_COL;
				model->tiles[i+1].flags |= TF_FABRIC_BRAM_MACC_ROUTING_COL;
				model->tiles[i+2].flags |= TF_FABRIC_MACC_COL;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15)
							tile_i0->type = ROUTING;
						else
							tile_i0->type = ROUTING_BRK;
						tile_i0[1].type = ROUTING_VIA;
						if (!(l%4)) {
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = MACC;
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].flags |= TF_MACC_DEV;
						}
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_MACC_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_MACC_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_MACC;
				}

				model->tiles[tile_columns + i].type = MACC_ROUTING_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = MACC_ROUTING_TERM_B;
				model->tiles[tile_columns + i + 1].type = MACC_VIA_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
				model->tiles[tile_columns + i + 2].type = left_side ? MACC_TERM_TL : MACC_TERM_TR;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = left_side ? MACC_TERM_BL : MACC_TERM_BR;

				model->tiles[model->center_y*tile_columns + i].type = REGH_MACC_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGH_MACC_ROUTING_VIA;
				model->tiles[model->center_y*tile_columns + i + 2].type = REGH_MACC_L;
				i += 3;
				break;
			case 'R':
				if (next_non_whitespace(&model->cfg_columns[j+1]) != 'M') {
					// We expect a LOGIC_XM column to follow the center for
					// the top and bottom bufpll and reg routing.
					fprintf(stderr, "Expecting LOGIC_XM after center but found '%c'\n", model->cfg_columns[j+1]);
				}
				model->center_x = i+3;

				left_side = 0;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];

						if ((k < model->cfg_rows-1 || l >= 2) && (k || l<14)) {
							if (l < 15)
								tile_i0->type = ROUTING;
							else
								tile_i0->type = ROUTING_BRK;
							if (l == 7)
								tile_i0[1].type = ROUTING_VIA_IO;
							else if (l == 8)
								tile_i0[1].type = (k%2) ? ROUTING_VIA_CARRY : ROUTING_VIA_IO_DCM;
							else if (l == 15 && k==model->cfg_rows/2)
								tile_i0[1].type = ROUTING_VIA_REGC;
							else {
								tile_i0[1].type = LOGIC_XL;
								tile_i0[1].flags |= TF_LOGIC_XL_DEV;
							}
						}
						if (l == 7
						    || (l == 8 && !(k%2))) { // even row, together with DCM
							tile_i0->type = IO_ROUTING;
						}

						if (l == 7) {
							if (k%2) { // odd
								model->tiles[(row_top_y+l)*tile_columns + i + 2].flags |= TF_PLL_DEV;
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? PLL_B : PLL_T;
							} else { // even
								model->tiles[(row_top_y+l)*tile_columns + i + 2].flags |= TF_DCM_DEV;
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? DCM_B : DCM_T;
							}
						}
						// four midbuf tiles, in the middle of the top and bottom halves
						if (l == 15) {
							if (k == model->cfg_rows*3/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_MIDBUF_T;
							else if (k == model->cfg_rows/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_HCLKBUF_B;
							else
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REGV_BRK;
						} else if (l == 0 && k == model->cfg_rows*3/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_HCLKBUF_T;
						} else if (l == 0 && k == model->cfg_rows/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REGV_MIDBUF_B;
						} else if (l == 8) {
							model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = (k<model->cfg_rows/2) ? REGV_B : REGV_T;
						} else
							model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 3].type = REGV;
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 3].type = HCLK_REGV;
				}
				model->tiles[i].type = IO_T;
				model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
				model->tiles[tile_columns + i].type = IO_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
				model->tiles[2*tile_columns + i].type = IO_ROUTING;
				model->tiles[3*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;

				model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
				model->tiles[2*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
				model->tiles[3*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].flags |= TF_IOLOGIC_DELAY_DEV;

				model->tiles[i + 2].type = REG_T;
				model->tiles[tile_columns + i + 2].type = REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = REG_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = REG_B;
				model->tiles[tile_columns + i + 3].type = REGV_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 3].type = REGV_TERM_B;

				model->tiles[model->center_y*tile_columns + i].type = REGC_ROUTING;
				model->tiles[model->center_y*tile_columns + i + 1].type = REGC_LOGIC;
				model->tiles[model->center_y*tile_columns + i + 2].type = REGC_CMT;
				model->tiles[model->center_y*tile_columns + i + 3].type = CENTER;

				i += 4;
				break;
			case ' ': // space used to make string more readable only
			case 'g': // global clock separator
			case 'n': // noio for logic blocks
				break;
			default:
				fprintf(stderr, "Ignoring unexpected column identifier '%c'\n", cur_cfgcol);
				break;
		}
	}

	//
	// left IO
	//

	for (k = model->cfg_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// +0
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns].type = IO_L;
			//
			// +1
			//
			if ((k == model->cfg_rows-1 && !l) || (!k && l==15))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = CORNER_TERM_L;
			else if (k == model->cfg_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_TOP;
			else if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + 1].type = IO_TERM_L_UPPER_BOT;
			else if (k == (model->cfg_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_TOP;
			else if (k == (model->cfg_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + 1].type = IO_TERM_L_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 1].type = IO_TERM_L;
			//
			// +2
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				if (l == 15 && k && k != model->cfg_rows/2)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_IO_L_BRK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING_IO_L;
			} else { // unwired
				if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_BRK;
				else if (k == model->cfg_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + 2].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING;
			}
			//
			// +3
			//
			if (model->cfg_left_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_IO_VIA_L;
			} else { // unwired
				if (k == model->cfg_rows-1 && !l) {
					model->tiles[(row_top_y+l)*tile_columns + 3].type = CORNER_TL;
				} else if (!k && l == 15) {
					model->tiles[(row_top_y+l+1)*tile_columns + 3].type = CORNER_BL;
				} else {
					if (k && k != model->cfg_rows/2 && l == 15)
						model->tiles[(row_top_y+l+1)*tile_columns + 3].type = ROUTING_VIA_CARRY;
					else
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 3].type = ROUTING_VIA;
				}
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + 1].type = HCLK_TERM_L;
		model->tiles[(row_top_y+8)*tile_columns + 2].type = HCLK_ROUTING_IO_L;
		if (k >= model->cfg_rows/2) { // top half
			if (k > (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_UP_L;
			else if (k == (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_TOP_DN_L;
		} else { // bottom half
			if (k < model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_DN_L;
			else if (k == model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_SPLIT_L;
			else
				model->tiles[(row_top_y+8)*tile_columns + 3].type = HCLK_IO_BOT_UP_L;
		}
		model->tiles[(row_top_y+8)*tile_columns + 4].type = HCLK_MCB;
	}

	model->tiles[(model->center_y-3)*tile_columns].type = IO_PCI_L;
	model->tiles[(model->center_y-2)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[(model->center_y-1)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[model->center_y*tile_columns].type = REG_L;
	model->tiles[(model->center_y+1)*tile_columns].type = IO_RDY_L;

	model->tiles[model->center_y*tile_columns + 1].type = REGH_IO_TERM_L;

	model->tiles[tile_columns + 2].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + 2].type = CORNER_TERM_B;
	model->tiles[model->center_y*tile_columns + 2].type = REGH_ROUTING_IO_L;

	model->tiles[tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[(tile_rows-2)*tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[model->center_y*tile_columns + 3].type = REGH_IO_L;
	model->tiles[model->center_y*tile_columns + 4].type = REGH_MCB;

	//
	// right IO
	//

	for (k = model->cfg_rows-1; k >= 0; k--) {
		row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
		if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

		for (l = 0; l < 16; l++) {
			//
			// -1
			//
			if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_RDY_R;
			else if (k == model->cfg_rows/2 && l == 14)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == model->cfg_rows/2 && l == 15)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 1].type = IO_PCI_CONN_R;
			else if (k == model->cfg_rows/2-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 1].type = IO_PCI_R;
			else {
				if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 1].type = IO_R;
			}
			//
			// -2
			//
			if ((k == model->cfg_rows-1 && (!l || l == 1)) || (!k && (l==15 || l==14)))
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = CORNER_TERM_R;
			else if (k == model->cfg_rows/2 && l == 12)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_TOP;
			else if (k == model->cfg_rows/2 && l == 13)
				model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 2].type = IO_TERM_R_UPPER_BOT;
			else if (k == (model->cfg_rows/2)-1 && !l)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_TOP;
			else if (k == (model->cfg_rows/2)-1 && l == 1)
				model->tiles[(row_top_y+l)*tile_columns + tile_columns - 2].type = IO_TERM_R_LOWER_BOT;
			else 
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 2].type = IO_TERM_R;
			//
			// -3
			//
			//
			// -4
			//
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_IO_VIA_R;
			else {
				if (k == model->cfg_rows-1 && l == 0)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_UPPER;
				else if (k == model->cfg_rows-1 && l == 1)
					model->tiles[(row_top_y+l)*tile_columns + tile_columns - 4].type = CORNER_TR_LOWER;
				else if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = ROUTING_VIA_CARRY;
				else if (!k && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = CORNER_BR_UPPER;
				else if (!k && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 4].type = CORNER_BR_LOWER;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 4].type = ROUTING_VIA;
			}
			//
			// -5
			//
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W')
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = IO_ROUTING;
			else {
				if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_BRK;
				else if (k == model->cfg_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_GCLK;
				else
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = ROUTING;
			}
		}
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 2].type = HCLK_TERM_R;
		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 3].type = HCLK_MCB;

		model->tiles[(row_top_y+8)*tile_columns + tile_columns - 5].type = HCLK_ROUTING_IO_R;
		if (k >= model->cfg_rows/2) { // top half
			if (k > (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_UP_R;
			else if (k == (model->cfg_rows*3)/4)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_TOP_DN_R;
		} else { // bottom half
			if (k < model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_DN_R;
			else if (k == model->cfg_rows/4 - 1)
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_SPLIT_R;
			else
				model->tiles[(row_top_y+8)*tile_columns + tile_columns - 4].type = HCLK_IO_BOT_UP_R;
		}
	}
	model->tiles[tile_columns + tile_columns - 5].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 5].type = CORNER_TERM_B;
	model->tiles[tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[(tile_rows-2)*tile_columns + tile_columns - 4].type = ROUTING_IO_PCI_CE_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 1].type = REG_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 2].type = REGH_IO_TERM_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 3].type = REGH_MCB;
	model->tiles[model->center_y*tile_columns + tile_columns - 4].type = REGH_IO_R;
	model->tiles[model->center_y*tile_columns + tile_columns - 5].type = REGH_ROUTING_IO_R;
	return 0;
}

//
// helper funcs
//

static const char* pf(const char* fmt, ...)
{
	// safe to call it 8 times in 1 expression (such as function params)
	static char pf_buf[8][128];
	static int last_buf = 0;
	va_list list;
	last_buf = (last_buf+1)%8;
	pf_buf[last_buf][0] = 0;
	va_start(list, fmt);
	vsnprintf(pf_buf[last_buf], sizeof(pf_buf[0]), fmt, list);
	va_end(list);
	return pf_buf[last_buf];
}

static const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name)
{
	static char buf[8][128];
	static int last_buf = 0;
	char* prefix;

	if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
		prefix = is_atx(X_CENTER_REGS_COL, model, x+3)
			? "REGC_INT_" : "REGH_";
	} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
		prefix = "HCLK_";
	else if (is_aty(Y_INNER_TOP, model, y))
		prefix = "IOI_TTERM_";
	else if (is_aty(Y_INNER_BOTTOM, model, y))
		prefix = "IOI_BTERM_";
	else
		prefix = "";

	last_buf = (last_buf+1)%8;
	buf[last_buf][0] = 0;
	strcpy(buf[last_buf], prefix);
	strcat(buf[last_buf], wire_name);
	return buf[last_buf];
}

static int _add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o);

static int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name)
{
	return _add_connpt_name(model, y, x, connpt_name,
		1 /* warn_if_duplicate */,
		0 /* name_i */, 0 /* conn_point_o */);
}

#define CONN_NAMES_INCREMENT	128

static int _add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o)
{
	struct fpga_tile* tile;
	uint16_t _name_i;
	int rc, i;

	tile = &model->tiles[y * model->tile_x_range + x];
	rc = strarray_add(&model->str, connpt_name, &i);
	if (rc) return rc;
	if (i > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	_name_i = i;
	if (name_i) *name_i = i;

	// Search for an existing connection point under name.
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == _name_i)
			break;
	}
	if (conn_point_o) *conn_point_o = i;
	if (i < tile->num_conn_point_names) {
		if (warn_if_duplicate)
			fprintf(stderr,
				"Duplicate connection point name y%02i x%02u %s\n",
				y, x, connpt_name);
		return 0;
	}
	// This is the first connection under name, add name.
	if (!(tile->num_conn_point_names % CONN_NAMES_INCREMENT)) {
		uint16_t* new_ptr = realloc(tile->conn_point_names,
			(tile->num_conn_point_names+CONN_NAMES_INCREMENT)*2*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_names = new_ptr;
	}
	tile->conn_point_names[tile->num_conn_point_names*2] = tile->num_conn_point_dests;
	tile->conn_point_names[tile->num_conn_point_names*2+1] = _name_i;
	tile->num_conn_point_names++;
	return 0;
}

#define CONNS_INCREMENT		128
#undef DBG_ADD_CONN_UNI

static int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	struct fpga_tile* tile;
	uint16_t name1_i, name2_i;
	uint16_t* new_ptr;
	int conn_start, num_conn_point_dests_for_this_wire, rc, j, conn_point_o;

	rc = _add_connpt_name(model, y1, x1, name1, 0 /* warn_if_duplicate */,
		&name1_i, &conn_point_o);
	if (rc) goto xout;

	rc = strarray_add(&model->str, name2, &j);
	if (rc) return rc;
	if (j > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	name2_i = j;
	tile = &model->tiles[y1 * model->tile_x_range + x1];
	conn_start = tile->conn_point_names[conn_point_o*2];
	if (conn_point_o+1 >= tile->num_conn_point_names)
		num_conn_point_dests_for_this_wire = tile->num_conn_point_dests - conn_start;
	else
		num_conn_point_dests_for_this_wire = tile->conn_point_names[(conn_point_o+1)*2] - conn_start;

	// Is the connection made a second time?
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
		if (tile->conn_point_dests[j*3] == x2
		    && tile->conn_point_dests[j*3+1] == y2
		    && tile->conn_point_dests[j*3+2] == name2_i) {
			fprintf(stderr, "Duplicate conn (num_conn_point_dests %i): y%02i x%02i %s - y%02i x%02i %s.\n",
				num_conn_point_dests_for_this_wire, y1, x1, name1, y2, x2, name2);
			for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
				fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j,
					y1, x1, name1,
					tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
					strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
			}
			return 0;
		}
	}

	if (!(tile->num_conn_point_dests % CONNS_INCREMENT)) {
		new_ptr = realloc(tile->conn_point_dests, (tile->num_conn_point_dests+CONNS_INCREMENT)*3*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_dests = new_ptr;
	}
	if (tile->num_conn_point_dests > j)
		memmove(&tile->conn_point_dests[(j+1)*3], &tile->conn_point_dests[j*3], (tile->num_conn_point_dests-j)*3*sizeof(uint16_t));
	tile->conn_point_dests[j*3] = x2;
	tile->conn_point_dests[j*3+1] = y2;
	tile->conn_point_dests[j*3+2] = name2_i;
	tile->num_conn_point_dests++;
	while (conn_point_o+1 < tile->num_conn_point_names) {
		tile->conn_point_names[(conn_point_o+1)*2]++;
		conn_point_o++;
	}
#if DBG_ADD_CONN_UNI
	printf("conn_point_dests for y%02i x%02i %s now:\n", y1, x1, name1);
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire+1; j++) {
		fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j, y1, x1, name1,
			tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
			strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
	}
#endif
	return 0;
xout:
	return rc;
}

int add_conn_uni_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_uni(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

static int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	int rc = add_conn_uni(model, y1, x1, name1, y2, x2, name2);
	if (rc) return rc;
	return add_conn_uni(model, y2, x2, name2, y1, x1, name1);
}

static int add_conn_bi_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_bi(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

static int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func, int y1, int x1, const char* name1, int start1, int last1, int y2, int x2, const char* name2, int start2)
{
	char buf1[128], buf2[128];
	int rc, i;

	if (last1 <= start1)
		return (*add_conn_func)(model, y1, x1, name1, y2, x2, name2);
	for (i = start1; i <= last1; i++) {
		snprintf(buf1, sizeof(buf1), name1, i);
		snprintf(buf2, sizeof(buf2), name2, start2+(i-start1));
		rc = (*add_conn_func)(model, y1, x1, buf1, y2, x2, buf2);
		if (rc) return rc;
	}
	return 0;
}

static int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net)
{
	int i, j, rc;

	for (i = 0; net->pts[i].name[0] && i < sizeof(net->pts)/sizeof(net->pts[0]); i++) {
		for (j = i+1; net->pts[j].name[0] && j < sizeof(net->pts)/sizeof(net->pts[0]); j++) {
			rc = add_conn_range(model, add_conn_func,
				net->pts[i].y, net->pts[i].x,
				net->pts[i].name,
				net->pts[i].start_count,
				net->pts[i].start_count + net->last_inc,
				net->pts[j].y, net->pts[j].x,
				net->pts[j].name,
				net->pts[j].start_count);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
}

static void seed_strx(struct fpga_model* model, struct seed_data* data)
{
	int x, i;
	for (x = 0; x < model->tile_x_range; x++) {
		model->tmp_str[x] = 0;
		for (i = 0; data[i].x_flags; i++) {
			if (is_atx(data[i].x_flags, model, x))
				model->tmp_str[x] = data[i].str;
		}
	}
}

static char next_non_whitespace(const char* s)
{
	int i;
	for (i = 0; s[i] == ' '; i++);
	return s[i];
}

static char last_major(const char* str, int cur_o)
{
	for (; cur_o; cur_o--) {
		if (str[cur_o-1] >= 'A' && str[cur_o-1] <= 'Z')
			return str[cur_o-1];
	}
	return 0;
}

static int is_in_row(struct fpga_model* model, int y, int* row_pos)
{
	int dist_to_center;

	if (y < 2) return 0;
	// normalize y to beginning of rows
	y -= 2;

	// calculate distance to center and check
	// that y is not pointing to the center
	dist_to_center = (model->cfg_rows/2)*(8+1/*middle of row*/+8);
	if (y == dist_to_center) return 0;
	if (y > dist_to_center) y--;

	// check that y is not pointing past the last row
	if (y >= model->cfg_rows*(8+1+8)) return 0;

	if (row_pos) *row_pos = y%(8+1+8);
	return 1;
}

int is_aty(int check, struct fpga_model* model, int y)
{
	if (y < 0) return 0;
	if (check & Y_INNER_TOP && y == 1) return 1;
	if (check & Y_INNER_BOTTOM && y == model->tile_y_range-2) return 1;
	if (check & Y_CHIP_HORIZ_REGS && y == model->center_y) return 1;
	if (check & (Y_ROW_HORIZ_AXSYMM|Y_BOTTOM_OF_ROW)) {
		int row_pos;
		if (is_in_row(model, y, &row_pos)) {
			if (check & Y_ROW_HORIZ_AXSYMM && row_pos == 8) return 1;
			if (check & Y_BOTTOM_OF_ROW && row_pos == 16) return 1;
		}
	}
	return 0;
}

int is_atx(int check, struct fpga_model* model, int x)
{
	if (x < 0) return 0;
	if (check & X_OUTER_LEFT && !x) return 1;
	if (check & X_INNER_LEFT && x == 1) return 1;
	if (check & X_INNER_RIGHT && x == model->tile_x_range-2) return 1;
	if (check & X_OUTER_RIGHT && x == model->tile_x_range-1) return 1;
	if (check & X_ROUTING_COL
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->tile_x_range-5
		|| x == model->center_x-3)) return 1;
	if (model->tiles[x].flags & TF_FABRIC_ROUTING_COL) {
		if (check & X_ROUTING_TO_BRAM_COL
		    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
		    && model->tiles[x+2].flags & TF_FABRIC_BRAM_COL) return 1;
		if (check & X_ROUTING_TO_MACC_COL
		    && model->tiles[x+1].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL
		    && model->tiles[x+2].flags & TF_FABRIC_MACC_COL) return 1;
	}
	if (check & X_LOGIC_COL
	    && (model->tiles[x].flags & TF_FABRIC_LOGIC_COL
	        || x == model->center_x-2)) return 1;
	if (check & X_FABRIC_ROUTING_COL && model->tiles[x].flags & TF_FABRIC_ROUTING_COL) return 1;
	if (check & X_FABRIC_LOGIC_COL && model->tiles[x].flags & TF_FABRIC_LOGIC_COL) return 1;
	if (check & X_FABRIC_BRAM_MACC_ROUTING_COL && model->tiles[x].flags & TF_FABRIC_BRAM_MACC_ROUTING_COL) return 1;
	if (check & X_FABRIC_BRAM_COL && model->tiles[x].flags & TF_FABRIC_BRAM_COL) return 1;
	if (check & X_FABRIC_MACC_COL && model->tiles[x].flags & TF_FABRIC_MACC_COL) return 1;
	if (check & X_CENTER_ROUTING_COL && x == model->center_x-3) return 1;
	if (check & X_CENTER_LOGIC_COL && x == model->center_x-2) return 1;
	if (check & X_CENTER_CMTPLL_COL && x == model->center_x-1) return 1;
	if (check & X_CENTER_REGS_COL && x == model->center_x) return 1;
	if (check & X_LEFT_IO_ROUTING_COL && x == LEFT_IO_ROUTING) return 1;
	if (check & X_LEFT_IO_DEVS_COL && x == LEFT_IO_DEVS) return 1;
	if (check & X_RIGHT_IO_ROUTING_COL && x == model->tile_x_range-5) return 1;
	if (check & X_RIGHT_IO_DEVS_COL && x == model->tile_x_range-4) return 1;
	if (check & X_LEFT_SIDE && x < model->center_x) return 1;
	if (check & X_LEFT_MCB && x == LEFT_MCB_COL) return 1;
	if (check & X_RIGHT_MCB && x == model->tile_x_range-3) return 1;
	return 0;
}

int is_atyx(int check, struct fpga_model* model, int y, int x)
{
	if (y < 0 || x < 0) return 0;
	if (check & YX_ROUTING_TILE
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->tile_x_range-5
		|| x == model->center_x-3)) {
		int row_pos;
		if (is_in_row(model, y, &row_pos) && row_pos != 8) return 1;
	}
	return 0;
}

static const char* fpga_ttstr[] = // tile type strings
{
	[NA] = "NA",
	[ROUTING] = "ROUTING",
	[ROUTING_BRK] = "ROUTING_BRK",
	[ROUTING_VIA] = "ROUTING_VIA",
	[HCLK_ROUTING_XM] = "HCLK_ROUTING_XM",
	[HCLK_ROUTING_XL] = "HCLK_ROUTING_XL",
	[HCLK_LOGIC_XM] = "HCLK_LOGIC_XM",
	[HCLK_LOGIC_XL] = "HCLK_LOGIC_XL",
	[LOGIC_XM] = "LOGIC_XM",
	[LOGIC_XL] = "LOGIC_XL",
	[REGH_ROUTING_XM] = "REGH_ROUTING_XM",
	[REGH_ROUTING_XL] = "REGH_ROUTING_XL",
	[REGH_LOGIC_XM] = "REGH_LOGIC_XM",
	[REGH_LOGIC_XL] = "REGH_LOGIC_XL",
	[BRAM_ROUTING] = "BRAM_ROUTING",
	[BRAM_ROUTING_BRK] = "BRAM_ROUTING_BRK",
	[BRAM] = "BRAM",
	[BRAM_ROUTING_TERM_T] = "BRAM_ROUTING_TERM_T",
	[BRAM_ROUTING_TERM_B] = "BRAM_ROUTING_TERM_B",
	[BRAM_ROUTING_VIA_TERM_T] = "BRAM_ROUTING_VIA_TERM_T",
	[BRAM_ROUTING_VIA_TERM_B] = "BRAM_ROUTING_VIA_TERM_B",
	[BRAM_TERM_LT] = "BRAM_TERM_LT",
	[BRAM_TERM_RT] = "BRAM_TERM_RT",
	[BRAM_TERM_LB] = "BRAM_TERM_LB",
	[BRAM_TERM_RB] = "BRAM_TERM_RB",
	[HCLK_BRAM_ROUTING] = "HCLK_BRAM_ROUTING",
	[HCLK_BRAM_ROUTING_VIA] = "HCLK_BRAM_ROUTING_VIA",
	[HCLK_BRAM] = "HCLK_BRAM",
	[REGH_BRAM_ROUTING] = "REGH_BRAM_ROUTING",
	[REGH_BRAM_ROUTING_VIA] = "REGH_BRAM_ROUTING_VIA",
	[REGH_BRAM_L] = "REGH_BRAM_L",
	[REGH_BRAM_R] = "REGH_BRAM_R",
	[MACC] = "MACC",
	[HCLK_MACC_ROUTING] = "HCLK_MACC_ROUTING",
	[HCLK_MACC_ROUTING_VIA] = "HCLK_MACC_ROUTING_VIA",
	[HCLK_MACC] = "HCLK_MACC",
	[REGH_MACC_ROUTING] = "REGH_MACC_ROUTING",
	[REGH_MACC_ROUTING_VIA] = "REGH_MACC_ROUTING_VIA",
	[REGH_MACC_L] = "REGH_MACC_L",
	[PLL_T] = "PLL_T",
	[DCM_T] = "DCM_T",
	[PLL_B] = "PLL_B",
	[DCM_B] = "DCM_B",
	[REG_T] = "REG_T",
	[REG_TERM_T] = "REG_TERM_T",
	[REG_TERM_B] = "REG_TERM_B",
	[REG_B] = "REG_B",
	[REGV_TERM_T] = "REGV_TERM_T",
	[REGV_TERM_B] = "REGV_TERM_B",
	[HCLK_REGV] = "HCLK_REGV",
	[REGV] = "REGV",
	[REGV_BRK] = "REGV_BRK",
	[REGV_T] = "REGV_T",
	[REGV_B] = "REGV_B",
	[REGV_MIDBUF_T] = "REGV_MIDBUF_T",
	[REGV_HCLKBUF_T] = "REGV_HCLKBUF_T",
	[REGV_HCLKBUF_B] = "REGV_HCLKBUF_B",
	[REGV_MIDBUF_B] = "REGV_MIDBUF_B",
	[REGC_ROUTING] = "REGC_ROUTING",
	[REGC_LOGIC] = "REGC_LOGIC",
	[REGC_CMT] = "REGC_CMT",
	[CENTER] = "CENTER",
	[IO_T] = "IO_T",
	[IO_B] = "IO_B",
	[IO_TERM_T] = "IO_TERM_T",
	[IO_TERM_B] = "IO_TERM_B",
	[IO_ROUTING] = "IO_ROUTING",
	[IO_LOGIC_TERM_T] = "IO_LOGIC_TERM_T",
	[IO_LOGIC_TERM_B] = "IO_LOGIC_TERM_B",
	[IO_OUTER_T] = "IO_OUTER_T",
	[IO_INNER_T] = "IO_INNER_T",
	[IO_OUTER_B] = "IO_OUTER_B",
	[IO_INNER_B] = "IO_INNER_B",
	[IO_BUFPLL_TERM_T] = "IO_BUFPLL_TERM_T",
	[IO_LOGIC_REG_TERM_T] = "IO_LOGIC_REG_TERM_T",
	[IO_BUFPLL_TERM_B] = "IO_BUFPLL_TERM_B",
	[IO_LOGIC_REG_TERM_B] = "IO_LOGIC_REG_TERM_B",
	[LOGIC_ROUTING_TERM_B] = "LOGIC_ROUTING_TERM_B",
	[LOGIC_NOIO_TERM_B] = "LOGIC_NOIO_TERM_B",
	[MACC_ROUTING_TERM_T] = "MACC_ROUTING_TERM_T",
	[MACC_ROUTING_TERM_B] = "MACC_ROUTING_TERM_B",
	[MACC_VIA_TERM_T] = "MACC_VIA_TERM_T",
	[MACC_TERM_TL] = "MACC_TERM_TL",
	[MACC_TERM_TR] = "MACC_TERM_TR",
	[MACC_TERM_BL] = "MACC_TERM_BL",
	[MACC_TERM_BR] = "MACC_TERM_BR",
	[ROUTING_VIA_REGC] = "ROUTING_VIA_REGC",
	[ROUTING_VIA_IO] = "ROUTING_VIA_IO",
	[ROUTING_VIA_IO_DCM] = "ROUTING_VIA_IO_DCM",
	[ROUTING_VIA_CARRY] = "ROUTING_VIA_CARRY",
	[CORNER_TERM_L] = "CORNER_TERM_L",
	[CORNER_TERM_R] = "CORNER_TERM_R",
	[IO_TERM_L_UPPER_TOP] = "IO_TERM_L_UPPER_TOP",
	[IO_TERM_L_UPPER_BOT] = "IO_TERM_L_UPPER_BOT",
	[IO_TERM_L_LOWER_TOP] = "IO_TERM_L_LOWER_TOP",
	[IO_TERM_L_LOWER_BOT] = "IO_TERM_L_LOWER_BOT",
	[IO_TERM_R_UPPER_TOP] = "IO_TERM_R_UPPER_TOP",
	[IO_TERM_R_UPPER_BOT] = "IO_TERM_R_UPPER_BOT",
	[IO_TERM_R_LOWER_TOP] = "IO_TERM_R_LOWER_TOP",
	[IO_TERM_R_LOWER_BOT] = "IO_TERM_R_LOWER_BOT",
	[IO_TERM_L] = "IO_TERM_L",
	[IO_TERM_R] = "IO_TERM_R",
	[HCLK_TERM_L] = "HCLK_TERM_L",
	[HCLK_TERM_R] = "HCLK_TERM_R",
	[REGH_IO_TERM_L] = "REGH_IO_TERM_L",
	[REGH_IO_TERM_R] = "REGH_IO_TERM_R",
	[REG_L] = "REG_L",
	[REG_R] = "REG_R",
	[IO_PCI_L] = "IO_PCI_L",
	[IO_PCI_R] = "IO_PCI_R",
	[IO_RDY_L] = "IO_RDY_L",
	[IO_RDY_R] = "IO_RDY_R",
	[IO_L] = "IO_L",
	[IO_R] = "IO_R",
	[IO_PCI_CONN_L] = "IO_PCI_CONN_L",
	[IO_PCI_CONN_R] = "IO_PCI_CONN_R",
	[CORNER_TERM_T] = "CORNER_TERM_T",
	[CORNER_TERM_B] = "CORNER_TERM_B",
	[ROUTING_IO_L] = "ROUTING_IO_L",
	[HCLK_ROUTING_IO_L] = "HCLK_ROUTING_IO_L",
	[HCLK_ROUTING_IO_R] = "HCLK_ROUTING_IO_R",
	[REGH_ROUTING_IO_L] = "REGH_ROUTING_IO_L",
	[REGH_ROUTING_IO_R] = "REGH_ROUTING_IO_R",
	[ROUTING_IO_L_BRK] = "ROUTING_IO_L_BRK",
	[ROUTING_GCLK] = "ROUTING_GCLK",
	[REGH_IO_L] = "REGH_IO_L",
	[REGH_IO_R] = "REGH_IO_R",
	[REGH_MCB] = "REGH_MCB",
	[HCLK_MCB] = "HCLK_MCB",
	[ROUTING_IO_VIA_L] = "ROUTING_IO_VIA_L",
	[ROUTING_IO_VIA_R] = "ROUTING_IO_VIA_R",
	[ROUTING_IO_PCI_CE_L] = "ROUTING_IO_PCI_CE_L",
	[ROUTING_IO_PCI_CE_R] = "ROUTING_IO_PCI_CE_R",
	[CORNER_TL] = "CORNER_TL",
	[CORNER_BL] = "CORNER_BL",
	[CORNER_TR_UPPER] = "CORNER_TR_UPPER",
	[CORNER_TR_LOWER] = "CORNER_TR_LOWER",
	[CORNER_BR_UPPER] = "CORNER_BR_UPPER",
	[CORNER_BR_LOWER] = "CORNER_BR_LOWER",
	[HCLK_IO_TOP_UP_L] = "HCLK_IO_TOP_UP_L",
	[HCLK_IO_TOP_UP_R] = "HCLK_IO_TOP_UP_R",
	[HCLK_IO_TOP_SPLIT_L] = "HCLK_IO_TOP_SPLIT_L",
	[HCLK_IO_TOP_SPLIT_R] = "HCLK_IO_TOP_SPLIT_R",
	[HCLK_IO_TOP_DN_L] = "HCLK_IO_TOP_DN_L",
	[HCLK_IO_TOP_DN_R] = "HCLK_IO_TOP_DN_R",
	[HCLK_IO_BOT_UP_L] = "HCLK_IO_BOT_UP_L",
	[HCLK_IO_BOT_UP_R] = "HCLK_IO_BOT_UP_R",
	[HCLK_IO_BOT_SPLIT_L] = "HCLK_IO_BOT_SPLIT_L",
	[HCLK_IO_BOT_SPLIT_R] = "HCLK_IO_BOT_SPLIT_R",
	[HCLK_IO_BOT_DN_L] = "HCLK_IO_BOT_DN_L",
	[HCLK_IO_BOT_DN_R] = "HCLK_IO_BOT_DN_R",
};

const char* fpga_tiletype_str(enum fpga_tile_type type)
{
	if (type >= sizeof(fpga_ttstr)/sizeof(fpga_ttstr[0])
	    || !fpga_ttstr[type]) return "UNK";
	return fpga_ttstr[type];
}
