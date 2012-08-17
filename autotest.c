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

struct test_state
{
	struct fpga_model* model;
	// test filenames are: tmp_dir/autotest_<base_name>_<diff_counter>.???
	char tmp_dir[256];
	char base_name[256];
	int next_diff_counter;
};

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

static int diff_start(struct test_state* tstate, const char* base_name)
{
	strcpy(tstate->base_name, base_name);
	tstate->next_diff_counter = 1;
	return 0;
}

static int diff_printf(struct test_state* tstate)
{
	char path[1024], tmp[1024], prior_fp[1024];
	int path_base;
	FILE* dest_f = 0;
	int rc;

	snprintf(path, sizeof(path), "%s/autotest_%s_%04i", tstate->tmp_dir,
		tstate->base_name, tstate->next_diff_counter);
	path_base = strlen(path);
	if (tstate->next_diff_counter == 1)
		strcpy(prior_fp, "/dev/null");
	else {
		snprintf(prior_fp, sizeof(prior_fp), "%s/autotest_%s_%04i.fp",
			tstate->tmp_dir, tstate->base_name,
			tstate->next_diff_counter-1);
	}

	strcpy(&path[path_base], ".fp");
	dest_f = fopen(path, "w");
	if (!dest_f) { rc = -1; FAIL(); }

	rc = printf_devices(dest_f, tstate->model, /*config_only*/ 1);
	if (rc) FAIL();
	rc = printf_switches(dest_f, tstate->model, /*enabled_only*/ 1);
	if (rc) FAIL();

	fclose(dest_f);
	dest_f = 0;
	path[path_base] = 0;
	snprintf(tmp, sizeof(tmp), "./autotest_diff.sh %s %s.fp >%s.log 2>&1",
		prior_fp, path, path);
	rc = system(tmp);
	if (rc) FAIL();

	strcpy(&path[path_base], ".diff");
	rc = dump_file(path);
	if (rc) FAIL();

	tstate->next_diff_counter++;
	return 0;
fail:
	if (dest_f) fclose(dest_f);
	return rc;
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	struct fpga_device* dev;
	int iob_y, iob_x, iob_idx, rc;
	struct test_state tstate;

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

	tstate.model = &model;
	strcpy(tstate.tmp_dir, AUTOTEST_TMP_DIR);
	mkdir(tstate.tmp_dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	rc = diff_start(&tstate, "and");
	if (rc) FAIL();
	
	// configure P46
	rc = fpga_find_iob(&model, "P46", &iob_y, &iob_x, &iob_idx);
	if (rc) FAIL();
	dev = fpga_dev(&model, iob_y, iob_x, DEV_IOB, iob_idx);
	if (!dev) { rc = -1; FAIL(); }
	dev->instantiated = 1;
	strcpy(dev->iob.istandard, IO_LVCMOS33);
	dev->iob.bypass_mux = BYPASS_MUX_I;
	dev->iob.I_mux = IMUX_I;

	// configure P48
	rc = fpga_find_iob(&model, "P48", &iob_y, &iob_x, &iob_idx);
	if (rc) FAIL();
	dev = fpga_dev(&model, iob_y, iob_x, DEV_IOB, iob_idx);
	if (!dev) { rc = -1; FAIL(); }
	dev->instantiated = 1;
	strcpy(dev->iob.ostandard, IO_LVCMOS33);
	dev->iob.drive_strength = 12;
	dev->iob.O_used = 1;
	dev->iob.slew = SLEW_SLOW;
	dev->iob.suspend = SUSP_3STATE;

	rc = diff_printf(&tstate);
	if (rc) goto fail;

	// configure logic
	dev = fpga_dev(&model, /*y*/ 68, /*x*/ 13, DEV_LOGIC, /*LOGIC_X*/ 1);
	if (!dev) { rc = -1; FAIL(); }
	dev->instantiated = 1;
	dev->logic.D_used = 1;
	rc = fpga_set_lut(&model, dev, D6_LUT, "A3", ZTERM);
	if (rc) FAIL();

	rc = diff_printf(&tstate);
	if (rc) goto fail;

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
