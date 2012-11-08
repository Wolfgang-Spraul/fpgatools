//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "parts.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	int no_conns, rc;

	if ((rc = fpga_build_model(&model, XC6SLX9, XC6SLX9_ROWS,
		  XC6SLX9_COLUMNS, XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;

	no_conns = 0;
	if (argc > 1 && !strcmp(argv[1], "--no-conns"))
		no_conns = 1;

	printf_version(stdout);

	rc = printf_tiles(stdout, &model);
	if (rc) goto fail;

	rc = printf_devices(stdout, &model, /*config_only*/ 0);
	if (rc) goto fail;

	rc = printf_ports(stdout, &model);
	if (rc) goto fail;

	if (!no_conns) {
		rc = printf_conns(stdout, &model);
		if (rc) goto fail;
	}

	rc = printf_switches(stdout, &model);
	if (rc) goto fail;

	return EXIT_SUCCESS;
fail:
	return rc;
}
