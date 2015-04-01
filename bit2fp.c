//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "bit.h"

static void help_exit(int argc, char **argv)
{
	fprintf(stderr,
		"\n"
		"%s - bitstream to floorplan\n"
		"Usage: %s [--help] [--verbose] [--bit-header] [--bit-regs] [--bit-crc]\n"
		"       %*s [--no-model] [--no-json] <bitstream_file>\n"
		"\n", argv[0], argv[0], (int) strlen(argv[0]), "");
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	int bit_header, bit_regs, bit_crc, json, pull_model, file_arg;
	int verbose, flags, rc = -1;
	struct fpga_config config;

	// parameters
	if (argc < 2) help_exit(argc, argv);
	verbose = 0;
   	bit_header = 0;
	bit_regs = 0;
	bit_crc = 0;
	pull_model = 1;
	json = 1;
	file_arg = 1;
	while (file_arg < argc && !strncmp(argv[file_arg], "--", 2)) {
		if (!strcmp(argv[file_arg], "--help"))
			help_exit(argc, argv);

		if (!strcmp(argv[file_arg], "--verbose"))
			verbose = 1;
		else if (!strcmp(argv[file_arg], "--bit-header"))
			bit_header = 1;
		else if (!strcmp(argv[file_arg], "--bit-regs"))
			bit_regs = 1;
		else if (!strcmp(argv[file_arg], "--bit-crc"))
			bit_crc = 1;
		else if (!strcmp(argv[file_arg], "--no-model"))
			pull_model = 0;
		else if (!strcmp(argv[file_arg], "--no-json"))
			json = 0;
		else break;
		file_arg++;
	}

	// read binary configuration file
	{
		FILE* fbits = fopen(argv[file_arg], "r");
		if (!fbits) {
			fprintf(stderr, "Error opening %s.\n", argv[file_arg]);
			goto fail;
		}
		rc = read_bitfile(&config, fbits, verbose);
		fclose(fbits);
		if (rc) FAIL(rc);
	}

	// build model
	if (config.idcode_reg == -1) FAIL(EINVAL);
	// todo: scanf package from header string, better default for part
	//   1. cmd line
	//   2. header string
	//   3. part-default
	if ((rc = fpga_build_model(&model, config.reg[config.idcode_reg].int_v,
		cmdline_package(argc, argv)))) FAIL(rc);

	// fill model from binary configuration
	if (pull_model)
		if ((rc = extract_model(&model, &config.bits))) FAIL(rc);

	// dump model
	flags = FP_DEFAULT;
	if (!json) flags |= FP_NO_JSON;
	if ((rc = write_floorplan(stdout, &model, flags))) FAIL(rc);

	// dump what doesn't fit into the model
   	flags = DUMP_BITS;
	if (bit_header) flags |= DUMP_HEADER_STR;
	if (bit_regs) flags |= DUMP_REGS;
	if (bit_crc) flags |= DUMP_CRC;
	if ((rc = dump_config(&config, flags))) FAIL(rc);
	return EXIT_SUCCESS;
fail:
	return rc;
}
