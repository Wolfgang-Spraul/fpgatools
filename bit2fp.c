//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "bits.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	FILE* fbits;
	int rc = -1;

	if (argc != 3) {
		fprintf(stderr,
			"\n"
			"%s - bitstream to floorplan\n"
			"Usage: %s <bitstream_file>\n"
			"\n", argv[0], argv[0]);
		goto fail;
	}

	fbits = fopen(argv[1], "r");
	if (!fbits) {
		fprintf(stderr, "Error opening %s.\n", argv[1]);
		goto fail;
	}

	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;

	if ((rc = read_bits(&model, fbits))) goto fail;
	if ((rc = write_floorplan(stdout, &model))) goto fail;
	return EXIT_SUCCESS;
fail:
	return rc;
}
