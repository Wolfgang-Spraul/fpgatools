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
#define TIMESTAMP()	printf("O #NODIFF timestamp %lld\n", (long long) TIME())
#define MEMUSAGE()	printf("O #NODIFF memusage %i\n", get_vm_mb());
#define TIME_AND_MEM()	TIMESTAMP(); MEMUSAGE()

#define AUTOTEST_TMP_DIR	"autotest.tmp"

struct test_state
{
	int cmdline_skip;
	char cmdline_diff_exec[1024];
	int dry_run;

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
	if (!(f = fopen(path, "r")))
		printf("#E error opening %s\n", path);
	else {
		while (fgets(line, sizeof(line), f)) {
			if (!strncmp(line, "--- ", 4)
			    || !strncmp(line, "+++ ", 4)
			    || !strncmp(line, "@@ ", 3))
				continue;
			printf(line);
		}
		fclose(f);
	}
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

	if (tstate->dry_run) {
		printf("O Dry run, skipping diff %i.\n", tstate->next_diff_counter++);
		return 0;
	}
	if (tstate->cmdline_skip >= tstate->next_diff_counter) {
		printf("O Skipping diff %i.\n", tstate->next_diff_counter++);
		return 0;
	}

	snprintf(path, sizeof(path), "%s/autotest_%s_%06i", tstate->tmp_dir,
		tstate->base_name, tstate->next_diff_counter);
	path_base = strlen(path);
	if (tstate->next_diff_counter == tstate->cmdline_skip + 1)
		strcpy(prior_fp, "/dev/null");
	else {
		snprintf(prior_fp, sizeof(prior_fp), "%s/autotest_%s_%06i.fp",
			tstate->tmp_dir, tstate->base_name,
			tstate->next_diff_counter-1);
	}

	strcpy(&path[path_base], ".fp");
	dest_f = fopen(path, "w");
	if (!dest_f) FAIL(errno);

	rc = printf_devices(dest_f, tstate->model, /*config_only*/ 1);
	if (rc) FAIL(rc);
	rc = printf_nets(dest_f, tstate->model);
	if (rc) FAIL(rc);

	fclose(dest_f);
	dest_f = 0;
	path[path_base] = 0;
	snprintf(tmp, sizeof(tmp), "%s %s %s.fp >%s.log 2>&1",
		tstate->cmdline_diff_exec, prior_fp, path, path);
	rc = system(tmp);
	if (rc) {
		printf("#E %s:%i system call '%s' failed with code %i, "
			"check %s.log\n", __FILE__, __LINE__, tmp, rc, path);
		// ENOENT comes back when pressing ctrl-c
		if (rc == ENOENT) EXIT(rc);
// todo: report the error up so we can avoid adding a switch to the block list etc.
	}

	strcpy(&path[path_base], ".diff");
	rc = dump_file(path);
	if (rc) FAIL(rc);

	tstate->next_diff_counter++;
	return 0;
fail:
	if (dest_f) fclose(dest_f);
	return rc;
}

// goal: configure logic devices in all supported variations
static int test_logic_config(struct test_state* tstate)
{
	int idx_enum[] = { DEV_LOGM, DEV_LOGX };
	int y, x, i, j, k, rc;

        y = 68;
	x = 13;

	for (i = 0; i < sizeof(idx_enum)/sizeof(*idx_enum); i++) {
		for (j = LUT_A; j <= LUT_D; j++) {

			// A1..A6 to A..D
			for (k = '1'; k <= '6'; k++) {
				rc = fdev_logic_set_lut(tstate->model, y, x,
					idx_enum[i], j, 6, pf("A%c", k), ZTERM);
				if (rc) FAIL(rc);
				rc = fdev_set_required_pins(tstate->model, y, x,
					DEV_LOGIC, idx_enum[i]);
				if (rc) FAIL(rc);

				if (tstate->dry_run)
					fdev_print_required_pins(tstate->model,
						y, x, DEV_LOGIC, idx_enum[i]);
				rc = diff_printf(tstate);
				if (rc) FAIL(rc);
				fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
			}

			// A1 to O6 to FF to AQ
			rc = fdev_logic_set_lut(tstate->model, y, x,
				idx_enum[i], j, 6, "A1", ZTERM);
			if (rc) FAIL(rc);
			rc = fdev_logic_FF(tstate->model, y, x, idx_enum[i],
				j, MUX_O6, FF_SRINIT0);
			if (rc) FAIL(rc);
			rc = fdev_logic_sync(tstate->model, y, x, idx_enum[i],
				SYNCATTR_ASYNC);
			if (rc) FAIL(rc);
			rc = fdev_logic_clk(tstate->model, y, x, idx_enum[i],
				CLKINV_B);
			if (rc) FAIL(rc);
			rc = fdev_logic_ceused(tstate->model, y, x, idx_enum[i]);
			if (rc) FAIL(rc);
			rc = fdev_logic_srused(tstate->model, y, x, idx_enum[i]);
			if (rc) FAIL(rc);

			rc = fdev_set_required_pins(tstate->model, y, x,
				DEV_LOGIC, idx_enum[i]);
			if (rc) FAIL(rc);

			if (tstate->dry_run)
				fdev_print_required_pins(tstate->model,
					y, x, DEV_LOGIC, idx_enum[i]);
			rc = diff_printf(tstate);
			if (rc) FAIL(rc);
			fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
		}
	}
	return 0;
fail:
	return rc;
}

static int test_logic_net(struct test_state* tstate, int logic_y, int logic_x,
	int type_idx, pinw_idx_t port, const struct sw_set* logic_switch_set,
	int routing_y, int routing_x, swidx_t routing_sw1, swidx_t routing_sw2)
{
	net_idx_t net_idx;
	struct sw_set routing_switches;
	int dbg, rc;

	dbg = 0;
	rc = fpga_net_new(tstate->model, &net_idx);
	if (rc) FAIL(rc);

	// add port
	rc = fpga_net_add_port(tstate->model, net_idx, logic_y, logic_x,
		fpga_dev_idx(tstate->model, logic_y, logic_x, DEV_LOGIC,
		type_idx), port);
	if (rc) FAIL(rc);

	// add (one) switch in logic tile
	rc = fpga_net_add_switches(tstate->model, net_idx,
		logic_y, logic_x, logic_switch_set);
	if (rc) FAIL(rc);

	// add switches in routing tile
	routing_switches.len = 0;
	if (routing_sw1 == NO_SWITCH) FAIL(EINVAL);
	routing_switches.sw[routing_switches.len++] = routing_sw1;
	if (routing_sw2 != NO_SWITCH)
		routing_switches.sw[routing_switches.len++] = routing_sw2;
	rc = fpga_net_add_switches(tstate->model, net_idx,
		routing_y, routing_x, &routing_switches);
	if (rc) FAIL(rc);

	if (dbg)
		printf("lnet %s %s\n",
			routing_sw2 == NO_SWITCH ? "" : fpga_switch_print(
				tstate->model, routing_y, routing_x,
				routing_sw2),
			fpga_switch_print(tstate->model,
				routing_y, routing_x, routing_sw1));

	rc = diff_printf(tstate);
	if (rc) FAIL(rc);

	fpga_net_delete(tstate->model, net_idx);
	return 0;
fail:
	return rc;
}

static int test_logic_net_l2(struct test_state* tstate, int y, int x,
	int type, int type_idx, str16_t* done_pinw_list, int* done_pinw_len,
	swidx_t* l2_done_list, int* l2_done_len)
{
	struct fpga_device* dev;
	struct switch_to_yx switch_to;
	int i, j, k, l, m, from_to, rc;
	struct sw_set set_l1, set_l2;
	struct fpga_tile* switch_tile;
	int dbg = 0;

	rc = fdev_set_required_pins(tstate->model, y, x, type, type_idx);
	if (rc) FAIL(rc);
	if (dbg)
		fdev_print_required_pins(tstate->model, y, x, type, type_idx);

	dev = fdev_p(tstate->model, y, x, type, type_idx);
	if (!dev) FAIL(EINVAL);
	for (i = 0; i < dev->pinw_req_total; i++) {

		// do every pinw only once across all configs
		for (j = 0; j < *done_pinw_len; j++) {
			if (done_pinw_list[j] == dev->pinw[dev->pinw_req_for_cfg[i]])
				break;
		}
		if (j < *done_pinw_len)
			continue;
		done_pinw_list[(*done_pinw_len)++] = dev->pinw[dev->pinw_req_for_cfg[i]];
	
		from_to = (i < dev->pinw_req_in) ? SW_TO : SW_FROM;
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = tstate->model;
		switch_to.y = y;
		switch_to.x = x;
		switch_to.start_switch = dev->pinw[dev->pinw_req_for_cfg[i]];
		switch_to.from_to = from_to;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		if (dbg)
			printf_switch_to_result(&switch_to);

		switch_tile = YX_TILE(tstate->model, switch_to.dest_y, switch_to.dest_x);
		rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
			switch_to.dest_x, switch_to.dest_connpt, from_to, &set_l1);
		if (rc) FAIL(rc);
		if (dbg)
			fpga_swset_print(tstate->model, switch_to.dest_y,
				switch_to.dest_x, &set_l1, from_to);

		for (j = 0; j < set_l1.len; j++) {
   			for (k = 0; k < 2; k++) {
				// k == 0 is the SW_FROM round, k == 1 is the SW_TO round.
				// For out-pins, we don't need the SW_TO round because they
				// would create multiple sources driving one pin which is
				// illegal.
				if (k && i >= dev->pinw_req_in)
					break;

				rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
					switch_to.dest_x, CONNPT_STR16(switch_tile,
					SW_I(switch_tile->switches[set_l1.sw[j]], !from_to)),
					k ? SW_TO : SW_FROM, &set_l2);
				if (rc) FAIL(rc);

				for (l = 0; l < set_l2.len; l++) {

					// duplicate check
					for (m = 0; m < *l2_done_len; m++) {
						if (l2_done_list[m] == set_l2.sw[l])
							break;
					}
					if (m < *l2_done_len)
						continue;
					l2_done_list[(*l2_done_len)++] = set_l2.sw[l];
					if (tstate->dry_run)
						printf("l2_done_list %s at %i\n", fpga_switch_print(tstate->model,
							switch_to.dest_y, switch_to.dest_x, l2_done_list[(*l2_done_len)-1]),
							(*l2_done_len)-1);

					// we did the l1 switches in an earlier round, but have to
					// redo them before every l2 switch to make a clean diff
					// on top of l1. The l2 can be in the same mip as the l1
					// so it has to be repeated for every l2 switch, not just
					// once for the set.
					rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
						&switch_to.set, switch_to.dest_y, switch_to.dest_x,
						set_l1.sw[j], NO_SWITCH);
					if (rc) FAIL(rc);

					rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
						&switch_to.set, switch_to.dest_y, switch_to.dest_x,
						set_l1.sw[j], set_l2.sw[l]);
					if (rc) FAIL(rc);
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int test_logic_net_l1(struct test_state* tstate, int y, int x,
	int type, int type_idx, str16_t* done_pinw_list, int* done_pinw_len,
	swidx_t* done_sw_list, int* done_sw_len)
{
	struct fpga_device* dev;
	struct switch_to_yx switch_to;
	int i, j, k, from_to, rc;
	struct sw_set set_l1;
	int dbg = 0;

	rc = fdev_set_required_pins(tstate->model, y, x, type, type_idx);
	if (rc) FAIL(rc);
	if (tstate->dry_run)
		fdev_print_required_pins(tstate->model, y, x, type, type_idx);

	dev = fdev_p(tstate->model, y, x, type, type_idx);
	if (!dev) FAIL(EINVAL);
	for (i = 0; i < dev->pinw_req_total; i++) {

		// do every pinw only once across all configs
		for (j = 0; j < *done_pinw_len; j++) {
			if (done_pinw_list[j] == dev->pinw[dev->pinw_req_for_cfg[i]])
				break;
		}
		if (j < *done_pinw_len)
			continue;
		done_pinw_list[(*done_pinw_len)++] = dev->pinw[dev->pinw_req_for_cfg[i]];

		from_to = (i < dev->pinw_req_in) ? SW_TO : SW_FROM;
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = tstate->model;
		switch_to.y = y;
		switch_to.x = x;
		switch_to.start_switch = dev->pinw[dev->pinw_req_for_cfg[i]];
		switch_to.from_to = from_to;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		if (tstate->dry_run)
			printf_switch_to_result(&switch_to);

		rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
			switch_to.dest_x, switch_to.dest_connpt, from_to, &set_l1);
		if (rc) FAIL(rc);
		if (dbg)
			fpga_swset_print(tstate->model, switch_to.dest_y,
				switch_to.dest_x, &set_l1, from_to);

		for (j = 0; j < set_l1.len; j++) {
			// an out-pin can go directly to an in-pin and
			// we don't need that pin twice
			for (k = 0; k < *done_sw_len; k++) {
				if (done_sw_list[k] == set_l1.sw[j])
					break;
			}
			if (k < *done_sw_len)
				continue;

			rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
				&switch_to.set, switch_to.dest_y, switch_to.dest_x,
				set_l1.sw[j], NO_SWITCH);
			if (rc) FAIL(rc);
			done_sw_list[(*done_sw_len)++] = set_l1.sw[j];
			if (tstate->dry_run)
				printf("done_list %s at %i\n", fpga_switch_print(tstate->model,
					switch_to.dest_y, switch_to.dest_x, set_l1.sw[j]),
					(*done_sw_len)-1);
		}
	}
	return 0;
fail:
	return rc;
}

// goal: use all switches in a routing switchbox
static int test_logic_switches(struct test_state* tstate)
{
	int idx_enum[] = { DEV_LOGM, DEV_LOGX };
	int y, x, i, j, k, r, rc;
	swidx_t done_sw_list[MAX_SWITCHBOX_SIZE];
	int done_sw_len;
	str16_t done_pinw_list[2000];
	int done_pinw_len;
	
        y = 68;
	x = 13;

	done_sw_len = 0;
	for (r = 0; r <= 1; r++) {
		// two rounds:
		// r == 0: round over all configs with single-level nets only
		// r == 1: second round with two-level nets

		done_pinw_len = 0; // reset done pinwires for each round

		for (i = 0; i < sizeof(idx_enum)/sizeof(*idx_enum); i++) {
			for (j = LUT_A; j <= LUT_D; j++) {
	
				// A1-A6 to A (same for lut B-D)
				for (k = '1'; k <= '6'; k++) {
					rc = fdev_logic_set_lut(tstate->model, y, x,
						idx_enum[i], j, 6, pf("A%c", k), ZTERM);
					if (rc) FAIL(rc);
					rc = fdev_logic_out_used(tstate->model, y, x,
						idx_enum[i], j);
					if (rc) FAIL(rc);
	
					if (!r)
						rc = test_logic_net_l1(tstate, y, x, DEV_LOGIC,
							idx_enum[i], done_pinw_list, &done_pinw_len,
							done_sw_list, &done_sw_len);
					else
						rc = test_logic_net_l2(tstate, y, x, DEV_LOGIC,
							idx_enum[i], done_pinw_list, &done_pinw_len,
							done_sw_list, &done_sw_len);
					if (rc) FAIL(rc);
					fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
				}
	
				// A1->O6->FF->AQ (same for lut B-D)
				rc = fdev_logic_set_lut(tstate->model, y, x,
					idx_enum[i], j, 6, "A1", ZTERM);
				if (rc) FAIL(rc);
				rc = fdev_logic_FF(tstate->model, y, x, idx_enum[i],
					j, MUX_O6, FF_SRINIT0);
				if (rc) FAIL(rc);
				rc = fdev_logic_sync(tstate->model, y, x, idx_enum[i],
					SYNCATTR_ASYNC);
				if (rc) FAIL(rc);
				rc = fdev_logic_clk(tstate->model, y, x, idx_enum[i],
					CLKINV_B);
				if (rc) FAIL(rc);
				rc = fdev_logic_ceused(tstate->model, y, x, idx_enum[i]);
				if (rc) FAIL(rc);
				rc = fdev_logic_srused(tstate->model, y, x, idx_enum[i]);
				if (rc) FAIL(rc);
	
				rc = fdev_set_required_pins(tstate->model, y, x,
					DEV_LOGIC, idx_enum[i]);
				if (rc) FAIL(rc);
	
				if (!r)
					rc = test_logic_net_l1(tstate, y, x, DEV_LOGIC,
						idx_enum[i], done_pinw_list, &done_pinw_len,
						done_sw_list, &done_sw_len);
				else
					rc = test_logic_net_l2(tstate, y, x, DEV_LOGIC,
						idx_enum[i], done_pinw_list, &done_pinw_len,
						done_sw_list, &done_sw_len);
				if (rc) FAIL(rc);
				fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
			}
		}
	}
	return 0;
fail:
	return rc;
}

#define DEFAULT_DIFF_EXEC "./autotest_diff.sh"

static void printf_help(const char* argv_0, const char** available_tests)
{
	printf( "\n"
		"fpgatools automatic test suite\n"
		"\n"
		"Usage: %s [--test=<name>] [--diff=<diff executable>] [--skip=<num>]\n"
		"       %*s [--dry-run]\n"
		"Default diff executable: " DEFAULT_DIFF_EXEC "\n", argv_0, (int) strlen(argv_0), "");

	if (available_tests) {
		int i = 0;
		printf("\n");
		while (available_tests[i] && available_tests[i][0]) {
			printf("%s %s\n", !i ? "Available tests:"
				: "                ", available_tests[i]);
			i++;
		}
	}
	printf("\n");
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	struct test_state tstate;
	char param[1024], cmdline_test[1024];
	int i, param_skip, rc;
	const char* available_tests[] =
		{ "logic_cfg", "logic_sw", 0 };

	// flush after every line is better for the autotest
	// output, tee, etc.
	// for example: ./autotest 2>&1 | tee autotest.log
	setvbuf(stdout, /*buf*/ 0, _IOLBF, /*size*/ 0);

	if (argc < 2) {
		printf_help(argv[0], available_tests);
		return 0;
	}

	//
	// command line
	//

	memset(&tstate, 0, sizeof(tstate));
	tstate.cmdline_skip = -1;
	tstate.cmdline_diff_exec[0] = 0;
	cmdline_test[0] = 0;
	tstate.dry_run = -1;
	for (i = 1; i < argc; i++) {
		memset(param, 0, sizeof(param));
		if (sscanf(argv[i], "--test=%1023c", param) == 1) {
			if (cmdline_test[0]) {
				printf_help(argv[0], available_tests);
				return EINVAL;
			}
			strcpy(cmdline_test, param);
			continue;
		}
		memset(param, 0, sizeof(param));
		if (sscanf(argv[i], "--diff=%1023c", param) == 1) {
			if (tstate.cmdline_diff_exec[0]) {
				printf_help(argv[0], available_tests);
				return EINVAL;
			}
			strcpy(tstate.cmdline_diff_exec, param);
			continue;
		}
		if (sscanf(argv[i], "--skip=%i", &param_skip) == 1) {
			if (tstate.cmdline_skip != -1) {
				printf_help(argv[0], available_tests);
				return EINVAL;
			}
			tstate.cmdline_skip = param_skip;
			continue;
		}
		if (!strcmp(argv[i], "--dry-run")) {
			tstate.dry_run = 1;
			continue;
		}
		printf_help(argv[0], available_tests);
		return EINVAL;
	}
	if (!cmdline_test[0]) {
		printf_help(argv[0], available_tests);
		return EINVAL;
	}
	i = 0;
	while (available_tests[i] && available_tests[i][0]) {
		if (!strcmp(available_tests[i], cmdline_test))
			break;
		i++;
	}
	if (!available_tests[i] || !available_tests[i][0]) {
		printf_help(argv[0], available_tests);
		return EINVAL;
	}
	if (!tstate.cmdline_diff_exec[0])
		strcpy(tstate.cmdline_diff_exec, DEFAULT_DIFF_EXEC);
	if (tstate.cmdline_skip == -1)
		tstate.cmdline_skip = 0;
	if (tstate.dry_run == -1)
		tstate.dry_run = 0;

	//
	// test
	//

	printf("\n");
	printf("O fpgatools automatic test suite. Be welcome and be "
			"our guest. namo namaha.\n");
	printf("\n");
	printf("O Test: %s\n", cmdline_test);
	printf("O Diff: %s\n", tstate.cmdline_diff_exec);
	printf("O Skip: %i\n", tstate.cmdline_skip);
	printf("O Dry run: %i\n", tstate.dry_run);
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
	rc = diff_start(&tstate, cmdline_test);
	if (rc) FAIL(rc);

	if (!strcmp(cmdline_test, "logic_cfg")) {
		rc = test_logic_config(&tstate);
		if (rc) FAIL(rc);
	}
	if (!strcmp(cmdline_test, "logic_sw")) {
		rc = test_logic_switches(&tstate);
		if (rc) FAIL(rc);
	}

	// iob_sw: test_iob_switches
	// test_iologic_switches

#if 0
// test_swchain:
//	printf_swchain(&model, 69, 13, strarray_find(&model.str, "BIOI_INNER_IBUF0"), SW_FROM, SW_SET_SIZE);
	{
		swidx_t done_list[MAX_SWITCHBOX_SIZE];
		int done_list_len = 0;
		printf_swchain(&model, 68, 12, strarray_find(&model.str, "LOGICIN_B29"),
			SW_TO, SW_SET_SIZE, done_list, &done_list_len);
#if 0
		printf_swchain(&model, 68, 12, strarray_find(&model.str, "NR1E0"),
			SW_FROM, SW_SET_SIZE, done_list, &done_list_len);
		printf_swchain(&model, 68, 12, strarray_find(&model.str, "NN2E0"),
			SW_FROM, SW_SET_SIZE, done_list, &done_list_len);
		printf_swchain(&model, 68, 12, strarray_find(&model.str, "SE2E2"),
			SW_FROM, SW_SET_SIZE, done_list, &done_list_len);
#endif
	}
#endif
	
#if 0
	struct fpga_model model;
	struct fpga_device* P46_dev, *P48_dev, *logic_dev;
	int P46_y, P46_x, P46_dev_idx, P46_type_idx;
	int P48_y, P48_x, P48_dev_idx, P48_type_idx, logic_dev_idx, rc;
	struct test_state tstate;
	struct switch_to_yx switch_to;
	net_idx_t P46_net;

	// configure P46
	rc = fpga_find_iob(&model, "P46", &P46_y, &P46_x, &P46_type_idx);
	if (rc) FAIL(rc);
	P46_dev_idx = fpga_dev_idx(&model, P46_y, P46_x, DEV_IOB, P46_type_idx);
	if (P46_dev_idx == NO_DEV) FAIL(EINVAL);
	P46_dev = FPGA_DEV(&model, P46_y, P46_x, P46_dev_idx);
	P46_dev->instantiated = 1;
	strcpy(P46_dev->iob.istandard, IO_LVCMOS33);
	P46_dev->iob.bypass_mux = BYPASS_MUX_I;
	P46_dev->iob.I_mux = IMUX_I;

	// configure P48
	rc = fpga_find_iob(&model, "P48", &P48_y, &P48_x, &P48_type_idx);
	if (rc) FAIL(rc);
	P48_dev_idx = fpga_dev_idx(&model, P48_y, P48_x, DEV_IOB, P48_type_idx);
	if (P48_dev_idx == NO_DEV) FAIL(EINVAL);
	P48_dev = FPGA_DEV(&model, P48_y, P48_x, P48_dev_idx);
	P48_dev->instantiated = 1;
	strcpy(P48_dev->iob.ostandard, IO_LVCMOS33);
	P48_dev->iob.drive_strength = 12;
	P48_dev->iob.O_used = 1;
	P48_dev->iob.slew = SLEW_SLOW;
	P48_dev->iob.suspend = SUSP_3STATE;

	// configure logic
	logic_dev_idx = fpga_dev_idx(&model, /*y*/ 68, /*x*/ 13,
		DEV_LOGIC, DEV_LOGX);
	if (logic_dev_idx == NO_DEV) FAIL(EINVAL);
	logic_dev = FPGA_DEV(&model, /*y*/ 68, /*x*/ 13, logic_dev_idx);
	logic_dev->instantiated = 1;
	logic_dev->logic.D_used = 1;
	rc = fpga_set_lut(&model, logic_dev, D6_LUT, "A3", ZTERM);
	if (rc) FAIL(rc);

	rc = diff_printf(&tstate);
	if (rc) FAIL(rc);

	// configure net from P46.I to logic.D3
	rc = fpga_net_new(&model, &P46_net);
	if (rc) FAIL(rc);
	rc = fpga_net_add_port(&model, P46_net, P46_y, P46_x,
		P46_dev_idx, IOB_OUT_I);
	if (rc) FAIL(rc);
	rc = fpga_net_add_port(&model, P46_net, /*y*/ 68, /*x*/ 13,
		logic_dev_idx, LI_D3);
	if (rc) FAIL(rc);

	switch_to.yx_req = YX_DEV_ILOGIC;
	switch_to.flags = SWTO_YX_DEF;
	switch_to.model = &model;
	switch_to.y = P46_y;
	switch_to.x = P46_x;
	switch_to.start_switch = P46_dev->pinw[IOB_OUT_I];
	switch_to.from_to = SW_FROM;
	rc = fpga_switch_to_yx(&switch_to);
	if (rc) FAIL(rc);
	rc = fpga_net_add_switches(&model, P46_net, switch_to.y,
		switch_to.x, &switch_to.set);
	if (rc) FAIL(rc);

	switch_to.yx_req = YX_ROUTING_TILE;
	switch_to.flags = SWTO_YX_DEF;
	switch_to.model = &model;
	switch_to.y = switch_to.dest_y;
	switch_to.x = switch_to.dest_x;
	switch_to.start_switch = switch_to.dest_connpt;
	switch_to.from_to = SW_FROM;
	rc = fpga_switch_to_yx(&switch_to);
	if (rc) FAIL(rc);
	rc = fpga_net_add_switches(&model, P46_net, switch_to.y,
		switch_to.x, &switch_to.set);
	if (rc) FAIL(rc);

	switch_to.yx_req = YX_ROUTING_TO_FABLOGIC;
	switch_to.flags = SWTO_YX_CLOSEST;
	switch_to.model = &model;
	switch_to.y = switch_to.dest_y;
	switch_to.x = switch_to.dest_x;
	switch_to.start_switch = switch_to.dest_connpt;
	switch_to.from_to = SW_FROM;
	rc = fpga_switch_to_yx(&switch_to);
	if (rc) FAIL(rc);
	rc = fpga_net_add_switches(&model, P46_net, switch_to.y,
		switch_to.x, &switch_to.set);
	if (rc) FAIL(rc);

	switch_to.yx_req = YX_DEV_LOGIC;
	switch_to.flags = SWTO_YX_TARGET_CONNPT|SWTO_YX_MAX_SWITCH_DEPTH;
	switch_to.model = &model;
	switch_to.y = switch_to.dest_y;
	switch_to.x = switch_to.dest_x;
	switch_to.start_switch = switch_to.dest_connpt;
	switch_to.from_to = SW_FROM;
	switch_to.max_switch_depth = 1;
	{
		note: update to constructor
		struct sw_chain c = {
			.model = &model, .y = switch_to.dest_y,
			.x = switch_to.dest_x+1,
			.start_switch = logic_dev->pinw[LI_D3],
			.from_to = SW_TO,
			.max_depth = SW_SET_SIZE,
			.block_list = 0 };
		if (fpga_switch_chain(&c) == NO_CONN) FAIL(EINVAL);
		if (c.set.len == 0) { HERE(); FAIL(EINVAL); }
		switch_to.target_connpt = fpga_switch_str_i(&model,
			switch_to.dest_y, switch_to.dest_x+1,
			c.set.sw[c.set.len-1], SW_FROM);
	}
	rc = fpga_switch_to_yx(&switch_to);
	if (rc) FAIL(rc);
	rc = fpga_net_add_switches(&model, P46_net, switch_to.y,
		switch_to.x, &switch_to.set);
	if (rc) FAIL(rc);

	{
		note: update to constructor
		struct sw_chain c = {
			.model = &model, .y = switch_to.dest_y,
			.x = switch_to.dest_x,
			.start_switch = switch_to.dest_connpt,
			.from_to = SW_FROM,
			.max_depth = SW_SET_SIZE,
			.block_list = 0 };
		if (fpga_switch_chain(&c) == NO_CONN) FAIL(EINVAL);
		if (c.set.len == 0) { HERE(); FAIL(EINVAL); }
		rc = fpga_net_add_switches(&model, P46_net, c.y, c.x, &c.set);
		if (rc) FAIL(rc);
	}

	rc = test_net2(&tstate, P46_net);
	if (rc) FAIL(rc);
#endif

	printf("\n");
	printf("O Test completed.\n");
	TIME_AND_MEM();
	printf("\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
