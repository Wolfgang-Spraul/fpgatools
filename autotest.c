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

	printf("\n");
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
	printf("\n");
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
	if (!dest_f) FAIL(errno);

	rc = printf_devices(dest_f, tstate->model, /*config_only*/ 1);
	if (rc) FAIL(rc);
	rc = printf_switches(dest_f, tstate->model, /*enabled_only*/ 1);
	if (rc) FAIL(rc);

	fclose(dest_f);
	dest_f = 0;
	path[path_base] = 0;
	snprintf(tmp, sizeof(tmp), "./autotest_diff.sh %s %s.fp >%s.log 2>&1",
		prior_fp, path, path);
	rc = system(tmp);
	if (rc) FAIL(rc);

	strcpy(&path[path_base], ".diff");
	rc = dump_file(path);
	if (rc) FAIL(rc);

	tstate->next_diff_counter++;
	return 0;
fail:
	if (dest_f) fclose(dest_f);
	return rc;
}

static const char* s_spaces = "                                              ";

static int printf_switchtree(struct fpga_model* model, int y, int x,
	const char* start, int indent)
{
	int i, idx, conn_to_y, conn_to_x, rc;
	const char* to_str;

	printf("%.*sy%02i x%02i %s\n", indent, s_spaces, y, x, start);
	for (i = 0;; i++) {
		idx = fpga_conn_dest(model, y, x, start, i);
		if (idx == NO_CONN)
			break;
		if (!i)
			printf("%.*s| connects to:\n", indent, s_spaces);
		to_str = fpga_conn_to(model, y, x,
			idx, &conn_to_y, &conn_to_x);
		printf("%.*s  y%02i x%02i %s\n", indent, s_spaces,
			conn_to_y, conn_to_x, to_str);
	}

	idx = fpga_switch_first(model, y, x, start, SW_TO);
	if (idx != NO_SWITCH)
		printf("%.*s| can be switched from:\n", indent, s_spaces);
	while (idx != NO_SWITCH) {
		to_str = fpga_switch_str(model, y, x, idx, SW_FROM);
		printf("%.*s  %s %s\n", indent, s_spaces,
			fpga_switch_is_bidir(model, y, x, idx) ? "<->" : "<-",
			to_str);
		idx = fpga_switch_next(model, y, x, idx, SW_TO);
	}

	idx = fpga_switch_first(model, y, x, start, SW_FROM);
	if (idx != NO_SWITCH)
		printf("%.*s| switches to:\n", indent, s_spaces);
	while (idx != NO_SWITCH) {
		to_str = fpga_switch_str(model, y, x, idx, SW_TO);
		printf("%.*s  %s %s\n", indent, s_spaces,
			fpga_switch_is_bidir(model, y, x, idx) ? "<->" : "->",
			to_str);
		rc = printf_switchtree(model, y, x, to_str, indent+2);
		if (rc) FAIL(rc);
		idx = fpga_switch_next(model, y, x, idx, SW_FROM);
	}
	return 0;
fail:
	return rc;
}
int main(int argc, char** argv)
{
	struct fpga_model model;
	struct fpga_device* P46_dev, *P48_dev, *logic_dev;
	int P46_y, P46_x, P46_idx, P48_y, P48_x, P48_idx, i, sw_idx, rc;
	const char* str;
	char tmp_str[128];
	struct test_state tstate;
	const char* conn_to_str;
	int conn_idx, conn_to_y, conn_to_x;

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
	if (rc) FAIL(rc);
	
	// configure P46
	rc = fpga_find_iob(&model, "P46", &P46_y, &P46_x, &P46_idx);
	if (rc) FAIL(rc);
	P46_dev = fpga_dev(&model, P46_y, P46_x, DEV_IOB, P46_idx);
	if (!P46_dev) FAIL(EINVAL);
	P46_dev->instantiated = 1;
	strcpy(P46_dev->iob.istandard, IO_LVCMOS33);
	P46_dev->iob.bypass_mux = BYPASS_MUX_I;
	P46_dev->iob.I_mux = IMUX_I;

	// configure P48
	rc = fpga_find_iob(&model, "P48", &P48_y, &P48_x, &P48_idx);
	if (rc) FAIL(rc);
	P48_dev = fpga_dev(&model, P48_y, P48_x, DEV_IOB, P48_idx);
	if (!P48_dev) FAIL(EINVAL);
	P48_dev->instantiated = 1;
	strcpy(P48_dev->iob.ostandard, IO_LVCMOS33);
	P48_dev->iob.drive_strength = 12;
	P48_dev->iob.O_used = 1;
	P48_dev->iob.slew = SLEW_SLOW;
	P48_dev->iob.suspend = SUSP_3STATE;

	// configure logic
	logic_dev = fpga_dev(&model, /*y*/ 68, /*x*/ 13, DEV_LOGIC, DEV_LOGX);
	if (!logic_dev) FAIL(EINVAL);
	logic_dev->instantiated = 1;
	logic_dev->logic.D_used = 1;
	rc = fpga_set_lut(&model, logic_dev, D6_LUT, "A3", ZTERM);
	if (rc) FAIL(rc);

	rc = diff_printf(&tstate);
	if (rc) goto fail;

	printf("P46 I pinw %s\n", P46_dev->iob.pinw_out_I);
	sw_idx = fpga_switch_first(&model, P46_y, P46_x,
		P46_dev->iob.pinw_out_I, SW_FROM);
	while (sw_idx != NO_SWITCH) {
		str = fpga_switch_str(&model, P46_y, P46_x, sw_idx, SW_TO);
		if (!str) FAIL(EINVAL);
		strcpy(tmp_str, str); // ringbuffer too small for long use
		printf(" from %s to %s\n", P46_dev->iob.pinw_out_I, tmp_str);
		for (i = 0;; i++) {
			conn_idx = fpga_conn_dest(&model, P46_y, P46_x, tmp_str, i);
			if (conn_idx == NO_CONN)
				break;
			conn_to_str = fpga_conn_to(&model, P46_y, P46_x,
				conn_idx, &conn_to_y, &conn_to_x);
			printf("  %s goes to y%02i x%02i %s\n",
				tmp_str, conn_to_y, conn_to_x, conn_to_str);
			if (is_aty(Y_TOP_INNER_IO|Y_BOT_INNER_IO, &model, conn_to_y)) {
				rc = printf_switchtree(&model, conn_to_y,
					conn_to_x, conn_to_str, /*indent*/ 3);
				if (rc) FAIL(rc);

// go through whole tree of what is reachable from that
// switch, look for outside connections that reach to a routing tile
				{ swidx_t idx;
				swidx_t* parents; int num_parents;

				idx = fpga_switch_tree(&model, conn_to_y, conn_to_x,
					conn_to_str, SW_FROM, &parents, &num_parents);
				while (idx != NO_SWITCH) {
					printf("idx %i num_parents %i from %s to %s\n",
						idx, num_parents,
						fpga_switch_str(&model, conn_to_y, conn_to_x, idx, SW_FROM),
						fpga_switch_str(&model, conn_to_y, conn_to_x, idx, SW_TO));
					idx = fpga_switch_tree(&model, conn_to_y, conn_to_x,
						SW_TREE_NEXT, SW_FROM, &parents, &num_parents);
				}
				}
			}
		}
		sw_idx = fpga_switch_next(&model, P46_y, P46_x, sw_idx, SW_FROM);
	}
	printf("P48 O pinw %s\n", P48_dev->iob.pinw_in_O);

	printf("\n");
	printf("O Test suite completed.\n");
	TIME_AND_MEM();
	printf("\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
