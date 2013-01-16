//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "bit.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help")) {
			printf( "\n%s\n\n"
				"Usage: %s [--part=xc6slx9]\n"
				"       %*s [--help]\n\n",
				*argv, *argv, (int) strlen(*argv), "");
			return 1;
		}
	}
	fpga_build_model(&model, cmdline_part(argc, argv),
		cmdline_package(argc, argv));
	printf_swbits(&model);
	return fpga_free_model(&model);
}
