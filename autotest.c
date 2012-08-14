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

#define AUTOTEST_TMP_DIR	"autotest.tmp"

static int dump_file(const char* path)
{
	char line[1024];
	FILE* f;

	printf("O begin dump %s\n", path);
	f = fopen(path, "r");
	EXIT(!f);
	while (fgets(line, sizeof(line), f)) {
		if (!strncmp(line, "--- ", 4)
		    || !strncmp(line, "+++ ", 4)
		    || !strncmp(line, "@@ ", 3))
			continue;
		printf(line);
	}
	fclose(f);
	printf("O end dump %s\n", path);
	return 0;
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	FILE* dest_f;
	struct fpga_device* dev;
	int iob_y, iob_x, iob_idx, rc;

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

	// todo: pick 2 input IOBs, one output IOB and configure them
	// todo: pick 1 logic block and configure
	rc = fpga_find_iob(&model, "P46", &iob_y, &iob_x, &iob_idx);
	EXIT(rc);

	dev = fpga_dev(&model, iob_y, iob_x, DEV_IOB, iob_idx);
	EXIT(!dev);
	dev->instantiated = 1;
	strcpy(dev->iob.istandard, IO_LVCMOS33);
	dev->iob.bypass_mux = BYPASS_MUX_I;
	dev->iob.imux = IMUX_I;

	mkdir(AUTOTEST_TMP_DIR, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	dest_f = fopen(AUTOTEST_TMP_DIR "/test_0001.fp", "w");
	EXIT(!dest_f);
	rc = printf_devices(dest_f, &model, /*config_only*/ 1);
	EXIT(rc);
	rc = printf_switches(dest_f, &model, /*enabled_only*/ 1);
	EXIT(rc);
	fclose(dest_f);
	rc = system("./autotest_diff.sh autotest.tmp/test_0001.fp");
	EXIT(rc);
	rc = dump_file(AUTOTEST_TMP_DIR "/test_0001.diff");
	EXIT(rc);

	// todo: start routing, step by step
	// todo: after each step, printf floorplan diff (test_diff.sh)
	// todo: popen/fork/pipe

	printf("\n");
	printf("O Test suite completed.\n");
	printf("\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
