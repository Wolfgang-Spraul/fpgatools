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
	for (y = 0; y < model.tile_y_range; y++) {
		for (x = 0; x < model.tile_x_range; x++) {
			printf("y%i x%i %s\n", y, x,
				fpga_tiletype_str(model.tiles[y*model.tile_x_range + x].type));
		}
	}
	return EXIT_SUCCESS;

fail:
	return EXIT_FAILURE;
}
