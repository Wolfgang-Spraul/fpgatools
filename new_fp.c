//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>

#include "model.h"

#define PRINT_FLAG(f)	if (tf & f) { printf (" %s", #f); tf &= ~f; }

int printf_tiles(struct fpga_model* model);
int printf_devices(struct fpga_model* model);
int printf_ports(struct fpga_model* model);
int printf_conns(struct fpga_model* model);
int printf_switches(struct fpga_model* model);

int main(int argc, char** argv)
{
	struct fpga_model model;
	int rc;

	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;

	printf("fpga_floorplan_format 1\n");

	//
	// What needs to be in the file:
	// - all devices, configuration for each device
	//   probably multiple lines that are adding config strings
	// - wires maybe separately, and/or as named connection points
	//   in tiles?
	// - connection pairs that can be enabled/disabled
	// - global flags and configuration registers
	// - the static data should be optional (unused conn pairs,
	//   unused devices, wires)
	//
	// - each line should be in the global namespace, line order
	//   should not matter
	// - file should be easily parsable with bison
	// - lines should typically not exceed 80 characters
	//

	rc = printf_tiles(&model);
	if (rc) goto fail;

	rc = printf_devices(&model);
	if (rc) goto fail;

	rc = printf_ports(&model);
	if (rc) goto fail;

	rc = printf_conns(&model);
	if (rc) goto fail;

	rc = printf_switches(&model);
	if (rc) goto fail;

	return EXIT_SUCCESS;
fail:
	return rc;
}

int printf_tiles(struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y;

	for (x = 0; x < model->tile_x_range; x++) {
		printf("\n");
		for (y = 0; y < model->tile_y_range; y++) {
			tile = &model->tiles[y*model->tile_x_range + x];

			if (tile->type != NA)
				printf("tile y%02i x%02i name %s\n", y, x,
					fpga_tiletype_str(tile->type));
			if (tile->flags) {
				int tf = tile->flags;
				printf("tile y%02i x%02i flags", y, x);

				PRINT_FLAG(TF_LOGIC_XL_DEV);
				PRINT_FLAG(TF_LOGIC_XM_DEV);
				PRINT_FLAG(TF_IOLOGIC_DELAY_DEV);
				PRINT_FLAG(TF_FABRIC_ROUTING_COL);
				PRINT_FLAG(TF_FABRIC_LOGIC_COL);
				PRINT_FLAG(TF_FABRIC_BRAM_MACC_ROUTING_COL);
				PRINT_FLAG(TF_FABRIC_BRAM_COL);
				PRINT_FLAG(TF_FABRIC_MACC_COL);
				PRINT_FLAG(TF_BRAM_DEV);
				PRINT_FLAG(TF_MACC_DEV);
				PRINT_FLAG(TF_LOGIC_XL_DEV);
				PRINT_FLAG(TF_LOGIC_XM_DEV);
				PRINT_FLAG(TF_IOLOGIC_DELAY_DEV);
				PRINT_FLAG(TF_DCM_DEV);
				PRINT_FLAG(TF_PLL_DEV);

				if (tf) printf(" 0x%x", tf);
				printf("\n");
			}
		}
	}
	return 0;
}

int printf_devices(struct fpga_model* model)
{
	// device y01 x02 type
	return 0;
}

int printf_ports(struct fpga_model* model)
{
	struct fpga_tile* tile;
	const char* conn_point_name_src;
	int x, y, i, conn_point_dests_o, num_dests_for_this_conn_point;
	int first_port_printed;

	for (x = 0; x < model->tile_x_range; x++) {
		for (y = 0; y < model->tile_y_range; y++) {
			tile = &model->tiles[y*model->tile_x_range + x];

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
					printf("\n");
				}
				printf("port y%02i x%02i %s\n", y, x, conn_point_name_src);
			}
		}
	}
	return 0;
}

int printf_conns(struct fpga_model* model)
{
	struct fpga_tile* tile;
	char tmp_line[512];
	const char* conn_point_name_src, *other_tile_connpt_str;
	uint16_t other_tile_connpt_str_i;
	int x, y, i, j, k, conn_point_dests_o, num_dests_for_this_conn_point;
	int other_tile_x, other_tile_y, first_conn_printed;

	for (x = 0; x < model->tile_x_range; x++) {
		for (y = 0; y < model->tile_y_range; y++) {
			tile = &model->tiles[y*model->tile_x_range + x];

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
						printf("\n");
					}
					sprintf(tmp_line, "static_conn y%02i x%02i %s ",
						y, x, conn_point_name_src);
					k = strlen(tmp_line);
					while (k < 45)
						tmp_line[k++] = ' ';
					sprintf(&tmp_line[k], "y%02i x%02i %s\n",
						other_tile_y, other_tile_x, other_tile_connpt_str);
					printf(tmp_line);
				}
			}
		}
	}
	return 0;
}

int printf_switches(struct fpga_model* model)
{
	// switch y01 x02 from direction(->|<->) to
	return 0;
}
