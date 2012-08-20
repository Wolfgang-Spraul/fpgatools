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
	int bit_header, bit_regs, fp_header, pull_model, file_arg, flags, rc = -1;
	struct fpga_config config;

	// parameters
	if (argc < 2) {
		fprintf(stderr,
			"\n"
			"%s - bitstream to floorplan\n"
			"Usage: %s [--bit-header] [--bit-regs] [--no-model] [--no-fp-header] <bitstream_file>\n"
			"\n", argv[0], argv[0]);
		goto fail;
	}
   	bit_header = 0;
	bit_regs = 0;
	pull_model = 1;
	fp_header = 1;
	file_arg = 1;
	while (!strncmp(argv[file_arg], "--", 2)) {
		if (!strcmp(argv[file_arg], "--bit-header"))
			bit_header = 1;
		else if (!strcmp(argv[file_arg], "--bit-regs"))
			bit_regs = 1;
		else if (!strcmp(argv[file_arg], "--no-model"))
			pull_model = 0;
		else if (!strcmp(argv[file_arg], "--no-fp-header"))
			fp_header = 0;
		else break;
		file_arg++;
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
	if (pull_model)
		if ((rc = extract_model(&model, &config.bits))) FAIL(rc);

	// dump model
	flags = FP_DEFAULT;
	if (!fp_header) flags |= FP_NO_HEADER;
	if ((rc = write_floorplan(stdout, &model, flags))) FAIL(rc);

	// dump what doesn't fit into the model
   	flags = DUMP_BITS;
	if (bit_header) flags |= DUMP_HEADER_STR;
	if (bit_regs) flags |= DUMP_REGS;
	if ((rc = dump_config(&config, flags))) FAIL(rc);
	return EXIT_SUCCESS;
fail:
	return rc;
}
