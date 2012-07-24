//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

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

int init_tiles(struct fpga_model* model);
int run_wires(struct fpga_model* model);

int fpga_build_model(struct fpga_model* model, int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring)
{
	int rc;

	memset(model, 0, sizeof(*model));
	model->cfg_rows = fpga_rows;
	strncpy(model->cfg_columns, columns, sizeof(model->cfg_columns)-1);
	strncpy(model->cfg_left_wiring, left_wiring, sizeof(model->cfg_left_wiring)-1);
	strncpy(model->cfg_right_wiring, right_wiring, sizeof(model->cfg_right_wiring)-1);
	strarray_init(&model->str);

	rc = init_tiles(model);
	if (rc) return rc;

	rc = run_wires(model);
	if (rc) return rc;

	return 0;
}

#define CONN_NAMES_INCREMENT	128
#define CONNS_INCREMENT		128

int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	struct fpga_tile* tile1;
	uint16_t name1_i, name2_i;
	uint16_t* new_ptr;
	int conn_start, num_conns_for_this_wire, rc, i, j;

	tile1 = &model->tiles[y1 * model->tile_x_range + x1];
	rc = strarray_find_or_add(&model->str, name1, &name1_i);
	if (!rc) return -1;
	rc = strarray_find_or_add(&model->str, name2, &name2_i);
	if (!rc) return -1;

	// Search for a connection set under name1.
	for (i = 0; i < tile1->num_conn_names; i++) {
		if (tile1->conn_names[i*2+1] == name1_i)
			break;
	}
	// If this is the first connection under name1, add name1.
	if (i >= tile1->num_conn_names) {
		if (!(tile1->num_conn_names % CONN_NAMES_INCREMENT)) {
			new_ptr = realloc(tile1->conn_names, (tile1->num_conn_names+CONN_NAMES_INCREMENT)*2*sizeof(uint16_t));
			if (!new_ptr) {
				fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
				return 0;
			}
			tile1->conn_names = new_ptr;
		}
		tile1->conn_names[tile1->num_conn_names*2] = tile1->num_conns;
		tile1->conn_names[tile1->num_conn_names*2+1] = name1_i;
		tile1->num_conn_names++;
	}
	conn_start = tile1->conn_names[i*2];
	if (i+1 >= tile1->num_conn_names)
		num_conns_for_this_wire = tile1->num_conns - conn_start;
	else
		num_conns_for_this_wire = tile1->conn_names[(i+1)*2] - conn_start;

	// Is the connection made a second time?
	for (j = conn_start; j < conn_start + num_conns_for_this_wire; j++) {
		if (tile1->conns[j*3] == x2
		    && tile1->conns[j*3+1] == y2
		    && tile1->conns[j*3+2] == name2_i) {
			fprintf(stderr, "Internal error, connection made twice.\n");
			return 0;
		}
	}

	if (!(tile1->num_conns % CONNS_INCREMENT)) {
		new_ptr = realloc(tile1->conns, (tile1->num_conns+CONNS_INCREMENT)*3*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile1->conns = new_ptr;
	}
	if (tile1->num_conns > j)
		memmove(&tile1->conns[(j+1)*3], &tile1->conns[j*3], (tile1->num_conns-j)*3*sizeof(uint16_t));
	tile1->conns[j*3] = x2;
	tile1->conns[j*3+1] = y2;
	tile1->conns[j*3+2] = name2_i;
	tile1->num_conns++;
	return 0;
}

int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	int rc = add_conn_uni(model, y1, x1, name1, y2, x2, name2);
	if (rc) return rc;
	return add_conn_uni(model, y2, x2, name2, y1, x1, name1);
}

const char* pf(const char* fmt, ...)
{
	static char pf_buf[128];
	va_list list;
	pf_buf[0] = 0;
	va_start(list, fmt);
	vsnprintf(pf_buf, sizeof(pf_buf), fmt, list);
	va_end(list);
	return pf_buf;
}

int run_wires(struct fpga_model* model)
{
	struct fpga_tile* tile, *tile_up1, *tile_up2;
	char b_wire[16], m_wire[16], e_wire[16];
	char* wire_fmt;
	int x, y, i, rc;

	rc = -1;
	for (y = 0; y < model->tile_y_range; y++) {
		for (x = 0; x < model->tile_x_range; x++) {
			tile = &model->tiles[y * model->tile_x_range + x];
			if (!(tile->flags & TF_DIRWIRE_START))
				continue;

			tile_up1 = &model->tiles[(y-1) * model->tile_x_range + x];
			tile_up2 = &model->tiles[(y-2) * model->tile_x_range + x];

			for (i = 0; i <= 3; i++) {
				sprintf(b_wire, "NN2B%i", i);
				sprintf(m_wire, "NN2M%i", i);
				sprintf(e_wire, "NN2E%i", i);
				if (tile_up1->flags & TF_SECOND_ROW) {
					rc = add_conn_bi(model, y, x, b_wire, y-1, x, pf("IOI_TTERM_%s", b_wire));
					if (rc) goto xout;
				} else if (tile_up2->flags & TF_SECOND_ROW) {
					rc = add_conn_bi(model, y, x, b_wire, y-1, x, m_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-2, x, pf("IOI_TTERM_%s", m_wire));
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, m_wire, y-2, x, pf("IOI_TTERM_%s", m_wire));
					if (rc) goto xout;
				} else if (tile_up1->flags & (TF_ROW_HORIZ_AXSYMM|TF_CHIP_HORIZ_AXSYMM)) {

					if (tile_up1->flags & TF_ROW_HORIZ_AXSYMM)
						wire_fmt = "HCLK_%s";
					else if (tile[3].flags & TF_CHIP_VERT_AXSYMM)
						wire_fmt = "REGC_INT_%s";
					else
						wire_fmt = "REGH_%s";

					rc = add_conn_bi(model, y, x, b_wire, y-1, x, pf(wire_fmt, m_wire));
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-2, x, m_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-3, x, e_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, pf(wire_fmt, m_wire), y-2, x, m_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, pf(wire_fmt, m_wire), y-3, x, e_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-2, x, m_wire, y-3, x, e_wire);
					if (rc) goto xout;

				} else if (tile_up2->flags & (TF_ROW_HORIZ_AXSYMM|TF_CHIP_HORIZ_AXSYMM)) {

					if (tile_up2->flags & TF_ROW_HORIZ_AXSYMM)
						wire_fmt = "HCLK_%s";
					else if (tile[3].flags & TF_CHIP_VERT_AXSYMM)
						wire_fmt = "REGC_INT_%s";
					else
						wire_fmt = "REGH_%s";

					rc = add_conn_bi(model, y, x, b_wire, y-1, x, m_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-2, x, pf(wire_fmt, e_wire));
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-3, x, e_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, m_wire, y-2, x, pf(wire_fmt, e_wire));
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, m_wire, y-3, x, e_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-2, x, pf(wire_fmt, e_wire), y-3, x, e_wire);
					if (rc) goto xout;

				} else {
					rc = add_conn_bi(model, y, x, b_wire, y-1, x, m_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y, x, b_wire, y-2, x, e_wire);
					if (rc) goto xout;
					rc = add_conn_bi(model, y-1, x, m_wire, y-2, x, e_wire);
					if (rc) goto xout;
					if (!i) {
						rc = add_conn_bi(model, y, x, "NN2B0", y-1, x, "NN2E_S0");
						if (rc) goto xout;
						rc = add_conn_bi(model, y-2, x, "NN2E0", y-1, x, "NN2E_S0");
						if (rc) goto xout;
					}
				}
			}
		}
	}
	return 0;
xout:
	return rc;
}

int init_tiles(struct fpga_model* model)
{
	int tile_rows, tile_columns, i, j, k, l, x, y, row_top_y, center_row, left_side;
	int start, end;
	struct fpga_tile* tile_i0;

	tile_rows = 1 /* middle */ + (8+1+8)*model->cfg_rows + 2+2 /* two extra tiles at top and bottom */;
	tile_columns = 5 /* left */ + 5 /* right */;
	for (i = 0; model->cfg_columns[i] != 0; i++) {
		tile_columns += 2; // 2 for logic blocks L/M and minimum for others
		if (model->cfg_columns[i] == 'B' || model->cfg_columns[i] == 'D')
			tile_columns++; // 3 for bram or macc
		else if (model->cfg_columns[i] == 'R')
			tile_columns+=2; // 2+2 for middle IO+logic+PLL/DCM
	}
	model->tile_x_range = tile_columns;
	model->tile_y_range = tile_rows;
	model->tiles = calloc(tile_columns * tile_rows, sizeof(struct fpga_tile));
	if (!model->tiles) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return -1;
	}
	for (i = 0; i < tile_rows * tile_columns; i++)
		model->tiles[i].type = NA;
	if (!(tile_rows % 2))
		fprintf(stderr, "Unexpected even number of tile rows (%i).\n", tile_rows);
	center_row = 2 /* top IO files */ + (model->cfg_rows/2)*(8+1/*middle of row clock*/+8);

	// flag horizontal rows
	for (x = 0; x < model->tile_x_range; x++) {
		model->tiles[x].flags |= TF_FIRST_ROW;
		model->tiles[model->tile_x_range + x].flags |= TF_SECOND_ROW;
		for (i = model->cfg_rows-1; i >= 0; i--) {
			row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-i)*(8+1/*middle of row clock*/+8);
			if (i<(model->cfg_rows/2)) row_top_y++; // middle system tiles
			model->tiles[(row_top_y+8)*model->tile_x_range + x].flags |= TF_ROW_HORIZ_AXSYMM;
		}
		model->tiles[center_row * model->tile_x_range + x].flags |= TF_CHIP_HORIZ_AXSYMM;
		model->tiles[(model->tile_y_range-2)*model->tile_x_range + x].flags |= TF_SECOND_TO_LAST_ROW;
		model->tiles[(model->tile_y_range-1)*model->tile_x_range + x].flags |= TF_LAST_ROW;
	}

	//
	// top, bottom, center:
	// go through columns from left to right, rows from top to bottom
	//

	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
	i = 5; // skip left IO columns
	for (j = 0; model->cfg_columns[j]; j++) {
		switch (model->cfg_columns[j]) {
			case 'L':
			case 'l':
			case 'M':
			case 'm':
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles (center row)
					start = ((k == model->cfg_rows-1 && (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M')) ? 2 : 0);
					end = ((k == 0 && (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M')) ? 14 : 16);
					for (l = start; l < end; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15 || (!k && (model->cfg_columns[j] == 'l' || model->cfg_columns[j] == 'm'))) {
							tile_i0->type = ROUTING;
							tile_i0->flags |= TF_DIRWIRE_START;
						} else
							tile_i0->type = ROUTING_BRK;
						tile_i0[1].type
							= (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'l') ? LOGIC_XL : LOGIC_XM;
					}
					if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'l') {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					} else {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XM;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XM;
					}
				}

				if (j && model->cfg_columns[j-1] == 'R') {
					model->tiles[tile_columns + i].type = IO_BUFPLL_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i].type = IO_BUFPLL_TERM_B;
				} else {
					model->tiles[tile_columns + i].type = IO_TERM_T;
					if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M')
						model->tiles[(tile_rows-2)*tile_columns + i].type = IO_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i].type = LOGIC_ROUTING_TERM_B;
				}
				if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M') {
					model->tiles[i].type = IO_T;
					model->tiles[(tile_rows-1)*tile_columns + i].type = IO_B;
					model->tiles[2*tile_columns + i].type = IO_ROUTING;
					model->tiles[2*tile_columns + i].flags |= TF_DIRWIRE_START;
					model->tiles[3*tile_columns + i].type = IO_ROUTING;
					model->tiles[3*tile_columns + i].flags |= TF_DIRWIRE_START;
					model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-4)*tile_columns + i].flags |= TF_DIRWIRE_START;
					model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;
					model->tiles[(tile_rows-3)*tile_columns + i].flags |= TF_DIRWIRE_START;
				}

				if (j && model->cfg_columns[j-1] == 'R') {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
					model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				} else {
					model->tiles[tile_columns + i + 1].type = IO_LOGIC_TERM_T;
					if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M')
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_TERM_B;
					else
						model->tiles[(tile_rows-2)*tile_columns + i + 1].type = LOGIC_NOIO_TERM_B;
				}
				if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'M') {
					model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
					model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
					model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
					model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;
				}

				if (model->cfg_columns[j] == 'L' || model->cfg_columns[j] == 'l') {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XL;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XL;
				} else {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XM;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XM;
				}
				i += 2;
				break;
			case 'B':
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15) {
							tile_i0->type = BRAM_ROUTING;
							tile_i0->flags |= TF_DIRWIRE_START;
						} else
							tile_i0->type = BRAM_ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = BRAM;
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

				model->tiles[center_row*tile_columns + i].type = REGH_BRAM_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_BRAM_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = left_side ? REGH_BRAM_L : REGH_BRAM_R;
				i += 3;
				break;
			case 'D':
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
						if (l < 15) {
							tile_i0->type = ROUTING;
							tile_i0->flags |= TF_DIRWIRE_START;
						} else
							tile_i0->type = ROUTING_BRK;
						tile_i0[1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = MACC;
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

				model->tiles[center_row*tile_columns + i].type = REGH_MACC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_MACC_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = REGH_MACC_L;
				i += 3;
				break;
			case 'R':
				if (model->cfg_columns[j+1] != 'M') {
					// We expect a LOGIC_XM column to follow the center for
					// the top and bottom bufpll and reg routing.
					fprintf(stderr, "Expecting LOGIC_XM after center but found '%c'\n", model->cfg_columns[j+1]);
				}
				left_side = 0;
				for (k = model->cfg_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (model->cfg_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(model->cfg_rows/2)) row_top_y++; // middle system tiles

					for (l = 0; l < 16; l++) {

						if ((k < model->cfg_rows-1 || l >= 2) && (k || l<14)) {
							tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
							if (l < 15) {
								tile_i0->type = ROUTING;
								tile_i0->flags |= TF_DIRWIRE_START;
							} else
								tile_i0->type = ROUTING_BRK;
							if (l == 7)
								tile_i0[1].type = ROUTING_VIA_IO;
							else if (l == 8)
								tile_i0[1].type = (k%2) ? ROUTING_VIA_CARRY : ROUTING_VIA_IO_DCM;
							else if (l == 15 && k==model->cfg_rows/2)
								tile_i0[1].type = ROUTING_VIA_REGC;
							else
								tile_i0[1].type = LOGIC_XL;
						}
						if (l == 7
						    || (l == 8 && !(k%2))) { // even row, together with DCM
							tile_i0 = &model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i];
							tile_i0->type = IO_ROUTING;
							tile_i0->flags |= TF_DIRWIRE_START;
						}

						if (l == 7) {
							if (k%2) // odd
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? PLL_B : PLL_T;
							else // even
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(model->cfg_rows/2)) ? DCM_B : DCM_T;
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
				model->tiles[2*tile_columns + i].flags |= TF_DIRWIRE_START;
				model->tiles[3*tile_columns + i].type = IO_ROUTING;
				model->tiles[3*tile_columns + i].flags |= TF_DIRWIRE_START;
				model->tiles[(tile_rows-4)*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-4)*tile_columns + i].flags |= TF_DIRWIRE_START;
				model->tiles[(tile_rows-3)*tile_columns + i].type = IO_ROUTING;
				model->tiles[(tile_rows-3)*tile_columns + i].flags |= TF_DIRWIRE_START;

				model->tiles[tile_columns + i + 1].type = IO_LOGIC_REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 1].type = IO_LOGIC_REG_TERM_B;
				model->tiles[2*tile_columns + i + 1].type = IO_OUTER_T;
				model->tiles[3*tile_columns + i + 1].type = IO_INNER_T;
				model->tiles[(tile_rows-4)*tile_columns + i + 1].type = IO_INNER_B;
				model->tiles[(tile_rows-3)*tile_columns + i + 1].type = IO_OUTER_B;

				model->tiles[i + 2].type = REG_T;
				model->tiles[tile_columns + i + 2].type = REG_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 2].type = REG_TERM_B;
				model->tiles[(tile_rows-1)*tile_columns + i + 2].type = REG_B;
				model->tiles[tile_columns + i + 3].type = REGV_TERM_T;
				model->tiles[(tile_rows-2)*tile_columns + i + 3].type = REGV_TERM_B;

				model->tiles[center_row*tile_columns + i].type = REGC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGC_LOGIC;
				model->tiles[center_row*tile_columns + i + 2].type = REGC_CMT;
				model->tiles[center_row*tile_columns + i + 3].type = CENTER;

				// flag vertical axis of symmetry
				for (y = 0; y < model->tile_y_range; y++)
					model->tiles[y*model->tile_x_range + i + 3].flags |= TF_CHIP_VERT_AXSYMM;

				i += 4;
				break;
			default:
				fprintf(stderr, "Unexpected column identifier '%c'\n", model->cfg_columns[j]);
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
				else {
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].type = ROUTING;
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + 2].flags |= TF_DIRWIRE_START;
				}
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

	model->tiles[(center_row-3)*tile_columns].type = IO_PCI_L;
	model->tiles[(center_row-2)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[(center_row-1)*tile_columns].type = IO_PCI_CONN_L;
	model->tiles[center_row*tile_columns].type = REG_L;
	model->tiles[(center_row+1)*tile_columns].type = IO_RDY_L;

	model->tiles[center_row*tile_columns + 1].type = REGH_IO_TERM_L;

	model->tiles[tile_columns + 2].type = CORNER_TERM_T;
	model->tiles[(tile_rows-2)*tile_columns + 2].type = CORNER_TERM_B;
	model->tiles[center_row*tile_columns + 2].type = REGH_ROUTING_IO_L;

	model->tiles[tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[(tile_rows-2)*tile_columns + 3].type = ROUTING_IO_PCI_CE_L;
	model->tiles[center_row*tile_columns + 3].type = REGH_IO_L;
	model->tiles[center_row*tile_columns + 4].type = REGH_MCB;

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
			if (model->cfg_right_wiring[(model->cfg_rows-1-k)*16+l] == 'W') {
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = IO_ROUTING;
				model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].flags |= TF_DIRWIRE_START;
			} else {
				if (k && k != model->cfg_rows/2 && l == 15)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_BRK;
				else if (k == model->cfg_rows/2 && l == 14)
					model->tiles[(row_top_y+l+1)*tile_columns + tile_columns - 5].type = ROUTING_GCLK;
				else {
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].type = ROUTING;
					model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + tile_columns - 5].flags |= TF_DIRWIRE_START;
				}
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
	model->tiles[center_row*tile_columns + tile_columns - 1].type = REG_R;
	model->tiles[center_row*tile_columns + tile_columns - 2].type = REGH_IO_TERM_R;
	model->tiles[center_row*tile_columns + tile_columns - 3].type = REGH_MCB;
	model->tiles[center_row*tile_columns + tile_columns - 4].type = REGH_IO_R;
	model->tiles[center_row*tile_columns + tile_columns - 5].type = REGH_ROUTING_IO_R;
	return 0;
}

void fpga_free_model(struct fpga_model* model)
{
	if (!model) return;
	strarray_free(&model->str);
	free(model->tiles);
	memset(model, 0, sizeof(*model));
}

const char* fpga_tiletype_str(enum fpga_tile_type type)
{
	if (type >= sizeof(fpga_ttstr)/sizeof(fpga_ttstr[0])
	    || !fpga_ttstr[type]) return "UNK";
	return fpga_ttstr[type];
}

// Dan Bernstein's hash function
uint32_t hash_djb2(const unsigned char* str)
{
	uint32_t hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        return hash;
}

//
// The format of each entry in a bin is.
//   uint16_t idx
//   uint16_t entry len including 4-byte header
//   char[]   zero-terminated string
//
// Offsets point to the zero-terminated string, so the len
// is at off-2, the index at off-4. bin0 offset0 can thus be
// used as a special value to signal 'no entry'.
//

const char* strarray_lookup(struct hashed_strarray* array, uint16_t idx)
{
	int bin, offset;

	if (!array->index_to_bin || !array->bin_offsets || !idx)
		return 0;

	bin = array->index_to_bin[idx];
	offset = array->bin_offsets[idx];

	// bin 0 offset 0 is a special value that signals 'no
	// entry'. Normal offsets cannot be less than 4.
	if (!bin && !offset) return 0;

	if (!array->bin_strings[bin] || offset >= array->bin_len[bin]
	    || offset < 4) {
		// This really should never happen and is an internal error.
		fprintf(stderr, "Internal error.\n");
		return 0;
	}

	return &array->bin_strings[bin][offset];
}

#define BIN_INCREMENT 32768

int strarray_find_or_add(struct hashed_strarray* array, const char* str,
	uint16_t* idx)
{
	int bin, search_off, str_len, i, free_index;
	int new_alloclen, start_index;
	unsigned long hash;
	void* new_ptr;

	hash = hash_djb2((const unsigned char*) str);
	str_len = strlen(str);
	bin = hash % (sizeof(array->bin_strings)/sizeof(array->bin_strings[0]));
	// iterate over strings in bin to find match
	if (array->bin_strings[bin]) {
		search_off = 4;
		while (search_off < array->bin_len[bin]) {
			if (!strcmp(&array->bin_strings[bin][search_off],
				str)) {
				*idx = *(uint16_t*)&array->bin_strings
					[bin][search_off-4];
				if (!(*idx)) {
					fprintf(stderr, "Internal error - index 0.\n");
					return 0;
				}
				return 1;
			}
			search_off += *(uint16_t*)&array->bin_strings
				[bin][search_off-2];
		}
	}
	// search free index
	start_index = (uint16_t) ((hash >> 16) ^ (hash & 0xFFFF));
	for (i = 0; i < HASHARRAY_NUM_INDICES; i++) {
		int cur_i = (start_index+i)%HASHARRAY_NUM_INDICES;
		if (!cur_i) // never issue index 0
			continue;
		if (!array->bin_offsets[cur_i])
			break;
	}
	if (i >= HASHARRAY_NUM_INDICES) {
		fprintf(stderr, "All array indices full.\n");
		return 0;
	}
	free_index = (start_index+i)%HASHARRAY_NUM_INDICES;
	// check whether bin needs expansion
	if (!(array->bin_len[bin]%BIN_INCREMENT)
	    || array->bin_len[bin]%BIN_INCREMENT + 4+str_len+1 > BIN_INCREMENT)
	{
		new_alloclen = 
 			((array->bin_len[bin]
				+ 4+str_len+1)/BIN_INCREMENT + 1)
			  * BIN_INCREMENT;
		new_ptr = realloc(array->bin_strings[bin], new_alloclen);
		if (!new_ptr) {
			fprintf(stderr, "Out of memory.\n");
			return 0;
		}
		array->bin_strings[bin] = new_ptr;
	}
	// append new string at end of bin
	*(uint16_t*)&array->bin_strings[bin][array->bin_len[bin]] = free_index;
	*(uint16_t*)&array->bin_strings[bin][array->bin_len[bin]+2] = 4+str_len+1;
	strcpy(&array->bin_strings[bin][array->bin_len[bin]+4], str);
	array->index_to_bin[free_index] = bin;
	array->bin_offsets[free_index] = array->bin_len[bin]+4;
	array->bin_len[bin] += 4+str_len+1;
	*idx = free_index;
	return 1;
}

int strarray_used_slots(struct hashed_strarray* array)
{
	int i, num_used_slots;
	num_used_slots = 0;
	if (!array->bin_offsets) return 0;
	for (i = 0; i < sizeof(array->bin_offsets)/sizeof(*array->bin_offsets); i++) {
		if (array->bin_offsets[i])
			num_used_slots++;
	}
	return num_used_slots;
}

void strarray_init(struct hashed_strarray* array)
{
	memset(array, 0, sizeof(*array));
}

void strarray_free(struct hashed_strarray* array)
{
	int i;
	for (i = 0; i < sizeof(array->bin_strings)/
		sizeof(array->bin_strings[0]); i++) {
		free(array->bin_strings[i]);
		array->bin_strings[i] = 0;
	}
}
