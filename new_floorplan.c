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

int main(int argc, char** argv)
{
	struct fpga_model model;
	struct fpga_tile* tile;
	int x, y;

	if (fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING))
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

	for (x = 0; x < model.tile_x_range; x++) {
		printf("\n");
		for (y = 0; y < model.tile_y_range; y++) {
			tile = &model.tiles[y*model.tile_x_range + x];

			printf("tile y%02i x%02i", y, x);

			if (tile->type == NA && !(tile->flags)) {
				printf(" -\n");
				continue;
			}
			if (tile->type != NA)
				printf(" name %s", fpga_tiletype_str(tile->type));
			if (tile->flags) {
				int tf = tile->flags;
				printf(" flags");
				if (tf & TF_DIRWIRE_START) {
					printf(" %s", MACRO_STR(TF_DIRWIRE_START));
					tf &= ~TF_DIRWIRE_START;
				}
				if (tf & TF_TOPMOST_TILE) {
					printf(" %s", MACRO_STR(TF_TOPMOST_TILE));
					tf &= ~TF_TOPMOST_TILE;
				}
				if (tf & TF_UNDER_TOPMOST_TILE) {
					printf(" %s", MACRO_STR(TF_UNDER_TOPMOST_TILE));
					tf &= ~TF_UNDER_TOPMOST_TILE;
				}
				if (tf & TF_ABOVE_BOTTOMMOST_TILE) {
					printf(" %s", MACRO_STR(TF_ABOVE_BOTTOMMOST_TILE));
					tf &= ~TF_ABOVE_BOTTOMMOST_TILE;
				}
				if (tf & TF_BOTTOMMOST_TILE) {
					printf(" %s", MACRO_STR(TF_BOTTOMMOST_TILE));
					tf &= ~TF_BOTTOMMOST_TILE;
				}
				if (tf & TF_ROW_HORIZ_AXSYMM) {
					printf(" %s", MACRO_STR(TF_ROW_HORIZ_AXSYMM));
					tf &= ~TF_ROW_HORIZ_AXSYMM;
				}
				if (tf & TF_BOTTOM_OF_ROW) {
					printf(" %s", MACRO_STR(TF_BOTTOM_OF_ROW));
					tf &= ~TF_BOTTOM_OF_ROW;
				}
				if (tf & TF_CHIP_HORIZ_AXSYMM) {
					printf(" %s", MACRO_STR(TF_CHIP_HORIZ_AXSYMM));
					tf &= ~TF_CHIP_HORIZ_AXSYMM;
				}
				if (tf & TF_CHIP_VERT_AXSYMM) {
					printf(" %s", MACRO_STR(TF_CHIP_VERT_AXSYMM));
					tf &= ~TF_CHIP_VERT_AXSYMM;
				}
				if (tf & TF_VERT_ROUTING) {
					printf(" %s", MACRO_STR(TF_VERT_ROUTING));
					tf &= ~TF_VERT_ROUTING;
				}
				if (tf)
					printf(" 0x%x", tf);
			}
			printf("\n");
		}
	}
	// todo: static_conn y%02i-x%02i-name y%02i-x%02i-name
	// todo: static_net <lists all wires connected together statically in a long line>
	return EXIT_SUCCESS;

fail:
	return EXIT_FAILURE;
}
