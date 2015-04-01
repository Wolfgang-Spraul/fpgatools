//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	int no_conns, rc;

	if ((rc = fpga_build_model(&model, XC6SLX9, TQG144)))
		goto fail;

	no_conns = 0;
	if (argc > 1 && !strcmp(argv[1], "--no-conns"))
		no_conns = 1;

	printf("{\n");
	printf_version(stdout);
	printf(",\n");

	rc = printf_tiles(stdout, &model);
	if (rc) goto fail;

	printf(",\n");
	rc = printf_devices(stdout, &model, /*config_only*/ 0, /*no_json*/ 0);
	if (rc) goto fail;

	printf(",\n");
	rc = printf_ports(stdout, &model);
	if (rc) goto fail;

	if (!no_conns) {
		printf(",\n");
		rc = printf_conns(stdout, &model);
		if (rc) goto fail;
	}

	printf(",\n");
	rc = printf_switches(stdout, &model);
	if (rc) goto fail;

	printf("\n}\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
