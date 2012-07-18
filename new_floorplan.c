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
	struct fpga_model* model = 0;
	int x, y;

	model = fpga_build_model(XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING);
	if (!model) goto fail;

	printf("fpga_floorplan_format 1\n");
	for (y = 0; y < model->tile_y_range; y++) {
		for (x = 0; x < model->tile_x_range; x++) {
			printf("x%i y%i %s\n", x, y,
				fpga_tiletype_str(model->tiles[y*model->tile_x_range + x].type));
		}
	}
	return EXIT_SUCCESS;

fail:
	return EXIT_FAILURE;
}
