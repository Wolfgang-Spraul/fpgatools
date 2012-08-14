//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <time.h>

#include "model.h"
#include "floorplan.h"
#include "control.h"

time_t g_start_time;
#define TIME()		(time(0)-g_start_time)
#define TIMESTAMP()	printf("O timestamp %lld\n", (long long) TIME())
#define MEMUSAGE()	printf("O memusage %i\n", get_vm_mb());
#define TIME_AND_MEM()	TIMESTAMP(); MEMUSAGE()

int main(int argc, char** argv)
{
	struct fpga_model model;
	int rc;

	printf("\n");
	printf("O fpgatools automatic test suite. Be welcome and be "
			"our guest. namo namaha.\n");
	printf("\n");
	printf("O Time measured in seconds from 0.\n");
	g_start_time = time(0);
	TIMESTAMP();
	printf("O Memory usage reported in megabytes.\n");
	MEMUSAGE();

	printf("O Building memory model...\n");
	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;
	printf("O Done\n");
	TIME_AND_MEM();

	// pick 2 input IOBs, one output IOB and configure them
	// pick 1 logic block and configure
	// printf floorplan
	// start routing, step by step
	// after each step, printf floorplan diff (test_diff.sh)
	// popen/fork/pipe

	printf("\n");
	printf("O Test suite completed.\n");
	printf("\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
