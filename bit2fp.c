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
	int bits_only, file_arg, rc = -1;
	struct fpga_config config;

	// parameters
	if (argc < 2) {
		fprintf(stderr,
			"\n"
			"%s - bitstream to floorplan\n"
			"Usage: %s [--bits-only] <bitstream_file>\n"
			"\n", argv[0], argv[0]);
		goto fail;
	}
	bits_only = 0;
	file_arg = 1;
	if (!strcmp(argv[1], "--bits-only")) {
		bits_only = 1;
		file_arg = 2;
	} 

	// read bitstream file
	{
		FILE* fbits = fopen(argv[file_arg], "r");
		if (!fbits) {
			fprintf(stderr, "Error opening %s.\n", argv[file_arg]);
			goto fail;
		}
		rc = read_bitfile(&config, fbits);
		fclose(fbits);
		if (rc) FAIL(rc);
	}

	// build model and fill from bitstream
	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING))) FAIL(rc);
	if ((rc = extract_model(&model, config.bits, config.bits_len))) FAIL(rc);

	// dump model
	if ((rc = write_floorplan(stdout, &model,
		bits_only ? FP_BITS_ONLY : FP_BITS_DEFAULT))) FAIL(rc);

	// dump what doesn't fit into the model
	if ((rc = dump_config(&config, bits_only ? DUMP_BITS
		: DUMP_BITS|DUMP_HEADER_STR|DUMP_REGS))) FAIL(rc);
	return EXIT_SUCCESS;
fail:
	return rc;
}
