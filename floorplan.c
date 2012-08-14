//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
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
				PRINT_FLAG(f, TF_FABRIC_LOGIC_COL);
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

int printf_devices(FILE* f, struct fpga_model* model, int config_only)
{
	int x, y, i;
	struct fpga_tile* tile;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);
			for (i = 0; i < tile->num_devices; i++) {
				if (config_only && !(tile->devices[i].instantiated))
					continue;
				switch (tile->devices[i].type) {
					case DEV_LOGIC_M:
						fprintf(f, "device y%02i x%02i SLICEM\n", y, x);
						break;
					case DEV_LOGIC_L:
						fprintf(f, "device y%02i x%02i SLICEL\n", y, x);
						break;
					case DEV_LOGIC_X:
						fprintf(f, "device y%02i x%02i SLICEX\n", y, x);
						break;
					case DEV_MACC:
						fprintf(f, "device y%02i x%02i DSP48A1\n", y, x);
						break;
					case DEV_TIEOFF:
						fprintf(f, "device y%02i x%02i TIEOFF\n", y, x);
						break;
					case DEV_IOB:
						fprintf(f, "device y%02i x%02i %s\n", y, x,
							tile->devices[i].iob.type == IOBM ? "IOBM" : "IOBS");
						break;
					case DEV_ILOGIC:
						fprintf(f, "device y%02i x%02i ILOGIC2\n", y, x);
						break;
					case DEV_OLOGIC:
						fprintf(f, "device y%02i x%02i OLOGIC2\n", y, x);
						break;
					case DEV_IODELAY:
						fprintf(f, "device y%02i x%02i IODELAY2\n", y, x);
						break;
					case DEV_BRAM16:
						fprintf(f, "device y%02i x%02i RAMB16BWER\n", y, x);
						break;
					case DEV_BRAM8:
						fprintf(f, "device y%02i x%02i RAMB8BWER\n", y, x);
						break;
					case DEV_BUFH:
						fprintf(f, "device y%02i x%02i BUFH\n", y, x);
						break;
					case DEV_BUFIO:
						fprintf(f, "device y%02i x%02i BUFIO2\n", y, x);
						break;
					case DEV_BUFIO_FB:
						fprintf(f, "device y%02i x%02i BUFIO2FB\n", y, x);
						break;
					case DEV_BUFPLL:
						fprintf(f, "device y%02i x%02i BUFPLL\n", y, x);
						break;
					case DEV_BUFPLL_MCB:
						fprintf(f, "device y%02i x%02i BUFPLL_MCB\n", y, x);
						break;
					case DEV_BUFGMUX:
						fprintf(f, "device y%02i x%02i BUFGMUX\n", y, x);
						break;
					case DEV_BSCAN:
						fprintf(f, "device y%02i x%02i BSCAN\n", y, x);
						break;
					case DEV_DCM:
						fprintf(f, "device y%02i x%02i DCM\n", y, x);
						break;
					case DEV_PLL:
						fprintf(f, "device y%02i x%02i PLL_ADV\n", y, x);
						break;
					case DEV_ICAP:
						fprintf(f, "device y%02i x%02i ICAP\n", y, x);
						break;
					case DEV_POST_CRC_INTERNAL:
						fprintf(f, "device y%02i x%02i POST_CRC_INTERNAL\n", y, x);
						break;
					case DEV_STARTUP:
						fprintf(f, "device y%02i x%02i STARTUP\n", y, x);
						break;
					case DEV_SLAVE_SPI:
						fprintf(f, "device y%02i x%02i SLAVE_SPI\n", y, x);
						break;
					case DEV_SUSPEND_SYNC:
						fprintf(f, "device y%02i x%02i SUSPEND_SYNC\n", y, x);
						break;
					case DEV_OCT_CALIBRATE:
						fprintf(f, "device y%02i x%02i OCT_CALIBRATE\n", y, x);
						break;
					case DEV_SPI_ACCESS:
						fprintf(f, "device y%02i x%02i SPI_ACCESS\n", y, x);
						break;
				}
			}
		}
	}
	return 0;
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
					sprintf(tmp_line, "static_conn y%02i x%02i %s ",
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

int printf_switches(FILE* f, struct fpga_model* model, int enabled_only)
{
	struct fpga_tile* tile;
	int x, y, i, from_connpt_o, to_connpt_o, from_str_i, to_str_i;
	int first_switch_printed, is_bidirectional, is_on;
	const char* from_str, *to_str;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);

			first_switch_printed = 0;
			for (i = 0; i < tile->num_switches; i++) {
				from_connpt_o = (tile->switches[i] & 0x3FFF8000) >> 15;
				to_connpt_o = tile->switches[i] & 0x00007FFF;
				is_bidirectional = (tile->switches[i] & SWITCH_BIDIRECTIONAL) != 0;
				is_on = (tile->switches[i] & SWITCH_ON) != 0;

				from_str_i = tile->conn_point_names[from_connpt_o*2+1];
				to_str_i = tile->conn_point_names[to_connpt_o*2+1];

				from_str = strarray_lookup(&model->str, from_str_i);
				to_str = strarray_lookup(&model->str, to_str_i);
				if (!from_str || !to_str) {
					fprintf(stderr, "Internal error in %s:%i, cannot lookup from_i %i or to_i %i\n",
						__FILE__, __LINE__, from_str_i, to_str_i);
					continue;
				}
				if (enabled_only && !is_on)
					continue;
				if (!first_switch_printed) {
					first_switch_printed = 1;	
					fprintf(f, "\n");
				}
				fprintf(f, "switch y%02i x%02i %s %s %s\n",
					y, x, from_str, is_bidirectional
					? "<->" : "->", to_str);
			}
		}
	}
	return 0;
}
