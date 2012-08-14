//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"

int main(int argc, char** argv)
{
	struct fpga_model model;
	int rc;

	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;

	// inform about progress over stdout

	// pick 2 input IOBs, one output IOB and configure them
	// pick 1 logic block and configure
	// printf floorplan

	return EXIT_SUCCESS;
fail:
	return rc;
}
