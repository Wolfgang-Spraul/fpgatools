//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "bit.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	FILE *fbits, *fp = 0;
	int rc = -1;

	fbits = 0;
	if (argc != 3) {
		fprintf(stderr,
			"\n"
			"%s - floorplan to bitstream\n"
			"Usage: %s <floorplan_file|- for stdin> <bits_file>\n"
			"\n", argv[0], argv[0]);
		goto fail;
	}

	if (!strcmp(argv[1], "-"))
		fp = stdin;
	else {
		fp = fopen(argv[1], "r");
		if (!fp) {
			fprintf(stderr, "Error opening %s.\n", argv[1]);
			goto fail;
		}
	}
	fbits = fopen(argv[2], "w");
	if (!fbits) {
		fprintf(stderr, "Error opening %s.\n", argv[2]);
		goto fail;
	}

	if ((rc = fpga_build_model(&model, XC6SLX9, TQG144)))
		goto fail;

	if ((rc = read_floorplan(&model, fp))) goto fail;
	if ((rc = write_bitfile(fbits, &model))) goto fail;
	fclose(fp);
	fclose(fbits);
	return EXIT_SUCCESS;
fail:
	if (fp) fclose(fp);
	if (fbits) fclose(fbits);
	return rc;
}
