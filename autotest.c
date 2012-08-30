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
	rc = printf_nets(dest_f, tstate->model);
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

static void cut_sw_from_end(struct fpga_net* net, int cut_o)
{
	if (cut_o <= 2) return;
	memmove(&net->el[2], &net->el[cut_o],
		(net->len-cut_o)*sizeof(net->el[0]));
	net->len = 2 + net->len-cut_o;
}

int test_net(struct test_state* tstate, net_idx_t net_i);
int test_net(struct test_state* tstate, net_idx_t net_i)
{
	struct fpga_net* net;
	struct fpga_net copy_net;
	swidx_t same_sw[64];
	int same_len;
	int i, j, rc;

	net = fpga_net_get(tstate->model, net_i);
	if (!net) FAIL(EINVAL);
	copy_net = *net;

	for (i = net->len-1; i >= 2; i--) {
		printf("round i %i\n", i);
		*net = copy_net;
		cut_sw_from_end(net, i);
		if (net->len <= 2) {
			rc = diff_printf(tstate);
			if (rc) FAIL(rc);
			continue;
		}
		// what other switches go to SW_TO(el[2])?
		printf_swchain(tstate->model,
			net->el[2].y, net->el[2].x,
			fpga_switch_str_i(tstate->model,
				net->el[2].y, net->el[2].x,
				net->el[2].idx, SW_TO),
				SW_TO, 1, 0, 0);
		same_len = sizeof(same_sw)/sizeof(*same_sw);
		rc = fpga_switch_same_fromto(tstate->model,
			net->el[2].y, net->el[2].x,
			net->el[2].idx, SW_TO, same_sw, &same_len);
		if (rc) FAIL(rc);
		printf("same_len at this level %i\n", same_len);
		for (j = 0; j < same_len; j++) {
			net->el[2].idx = same_sw[j];
			rc = diff_printf(tstate);
			if (rc) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

int test_net2(struct test_state* tstate, net_idx_t net_i);
int test_net2(struct test_state* tstate, net_idx_t net_i)
{
	struct fpga_net* net;
	struct fpga_net copy_net;
	swidx_t same_sw[64];
	int same_len;
	int i, j, rc;

	net = fpga_net_get(tstate->model, net_i);
	if (!net) FAIL(EINVAL);
	copy_net = *net;

	for (i = 3; i <= net->len-1; i++) {
		printf("round i %i\n", i);
		net->len = i;
		// what other switches go from SW_FROM(el[i-1])?
		printf_swchain(tstate->model,
			net->el[i-1].y, net->el[i-1].x,
			fpga_switch_str_i(tstate->model,
				net->el[i-1].y, net->el[i-1].x,
				net->el[i-1].idx, SW_FROM),
				SW_FROM, 1, 0, 0);
		same_len = sizeof(same_sw)/sizeof(*same_sw);
		rc = fpga_switch_same_fromto(tstate->model,
			net->el[i-1].y, net->el[i-1].x,
			net->el[i-1].idx, SW_FROM, same_sw, &same_len);
		if (rc) FAIL(rc);
		printf("same_len at this level %i\n", same_len);
		for (j = 0; j < same_len; j++) {
			net->el[i-1].idx = same_sw[j];
			rc = diff_printf(tstate);
			if (rc) FAIL(rc);
		}
		*net = copy_net;
	}
	return 0;
fail:
	return rc;
}

// goal: configure logic devices in all supported variations
int test_logic_config(struct test_state* tstate);
int test_logic_config(struct test_state* tstate)
{
	int a_to_d[] = { A6_LUT, B6_LUT, C6_LUT, D6_LUT };
	int idx_enum[] = { DEV_LOGM, DEV_LOGX };
	int y, x, i, j, k, rc;

        y = 68;
	x = 13;

	for (i = 0; i < sizeof(idx_enum)/sizeof(*idx_enum); i++) {
		for (j = 0; j < sizeof(a_to_d)/sizeof(*a_to_d); j++) {
			for (k = '1'; k <= '6'; k++) {
				rc = fdev_logic_set_lut(tstate->model, y, x,
					idx_enum[i], a_to_d[j], pf("A%c", k), ZTERM);
				if (rc) FAIL(rc);

				rc = diff_printf(tstate);
				if (rc) FAIL(rc);

				fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int test_device_fingers(struct test_state* tstate, int y, int x,
	int type, int type_idx, swidx_t* block_list, int* block_list_len)
{
	struct fpga_device* dev;
	struct switch_to_yx switch_to;
	struct sw_chain chain;
	net_idx_t net_idx;
	int i, from_to, rc;

	rc = fdev_set_required_pins(tstate->model, y, x, type, type_idx);
	if (rc) FAIL(rc);
// fdev_print_required_pins(tstate->model, y, x, type, type_idx);

	dev = fdev_p(tstate->model, y, x, type, type_idx);
	if (!dev) FAIL(EINVAL);
	for (i = 0; i < dev->pinw_req_total; i++) {
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
// printf_switch_to_result(&switch_to);

		rc = construct_sw_chain(&chain, tstate->model, switch_to.dest_y,
			switch_to.dest_x, switch_to.dest_connpt, from_to,
			/*max_depth*/ 1, block_list, *block_list_len);
		if (rc) FAIL(rc);
		while (fpga_switch_chain(&chain) != NO_SWITCH) {
			rc = fpga_net_new(tstate->model, &net_idx);
			if (rc) FAIL(rc);

			// add port
			rc = fpga_net_add_port(tstate->model, net_idx,
				y, x, fpga_dev_idx(tstate->model, y, x,
				type, type_idx), dev->pinw_req_for_cfg[i]);
			if (rc) FAIL(rc);
			// add (one) switch in logic tile
			rc = fpga_net_add_switches(tstate->model, net_idx,
				y, x, &switch_to.set);
			if (rc) FAIL(rc);
			// add switches in routing tile
			rc = fpga_net_add_switches(tstate->model, net_idx,
				switch_to.dest_y, switch_to.dest_x, &chain.set);
			if (rc) FAIL(rc);

// fprintf_net(stdout, tstate->model, net_idx);
// printf("set %s\n", fmt_swset(tstate->model, switch_to.dest_y, switch_to.dest_x, &chain.set, from_to));

#if 1
			rc = diff_printf(tstate);
			if (rc) FAIL(rc);
#endif
			fpga_net_delete(tstate->model, net_idx);
		}
		*block_list_len = chain.block_list_len;
// printf("block_list_len now %i\n", *block_list_len);
		destruct_sw_chain(&chain);
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
	int rc;

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

#if 0
	printf("lnet %s %s\n", fpga_switch_print(tstate->model, routing_y,
		routing_x, routing_sw1), routing_sw2 == NO_SWITCH ? ""
		: fpga_switch_print(tstate->model, routing_y, routing_x, routing_sw2));
#endif
#if 1
	rc = diff_printf(tstate);
	if (rc) FAIL(rc);
#endif
	fpga_net_delete(tstate->model, net_idx);
	return 0;
fail:
	return rc;
}

static int test_logic_fingers(struct test_state* tstate, int y, int x,
	int type, int type_idx, str16_t* done_pinw_list, int* done_pinw_len,
	swidx_t* l2_done_list, int* l2_done_len)
{
	struct fpga_device* dev;
	struct switch_to_yx switch_to;
	struct sw_chain chain;
	net_idx_t net_idx;
	int i, j, k, l, from_to, direction, rc;
	struct sw_set set_l1, set_l2;
	struct fpga_tile* switch_tile;

	rc = fdev_set_required_pins(tstate->model, y, x, type, type_idx);
	if (rc) FAIL(rc);
// fdev_print_required_pins(tstate->model, y, x, type, type_idx);

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
// printf_switch_to_result(&switch_to);

		switch_tile = YX_TILE(tstate->model, switch_to.dest_y, switch_to.dest_x);
		rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
			switch_to.dest_x, switch_to.dest_connpt, from_to, &set_l1);
		if (rc) FAIL(rc);

//		if (1||(i < dev->pinw_req_in)) {
//		if (i >= dev->pinw_req_in) {
		if (1) {
// fpga_swset_print(tstate->model, switch_to.dest_y, switch_to.dest_x, &set_l1, from_to);
			for (j = 0; j < set_l1.len; j++) {
				rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
					&switch_to.set, switch_to.dest_y, switch_to.dest_x,
					set_l1.sw[j], NO_SWITCH);
				if (rc) FAIL(rc);
				// level one switches themselves cannot become duplicates
				// because we check for unique pinwires already. but we still
				// need to add them to the done_list to compare against
				// future l2 switches.
// todo: rename l2_done_list to just done_list
				l2_done_list[(*l2_done_len)++] = set_l1.sw[j];

				rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
					switch_to.dest_x, CONNPT_STR16(switch_tile,
					SW_I(switch_tile->switches[set_l1.sw[j]], !from_to)),
					i < dev->pinw_req_in ? from_to : !from_to, &set_l2);
				if (rc) FAIL(rc);

				for (k = 0; k < set_l2.len; k++) {
					// on an out pin, we are reversing direction and looking
					// for other switches pointing *to* the output, but we
					// can skip ourselves...
					if (set_l2.sw[k] == set_l1.sw[j])
						continue;

					// The level 1 switches cannot duplicate because we
					// already track unique pinwires. But level 2 switches
					// must be checked for duplicates.
					for (l = 0; l < *l2_done_len; l++) {
						if (l2_done_list[l] == set_l2.sw[k])
							break;
					}
					if (l < *l2_done_len)
						continue;
					l2_done_list[(*l2_done_len)++] = set_l2.sw[k];

					rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
						&switch_to.set, switch_to.dest_y, switch_to.dest_x,
						set_l1.sw[j], set_l2.sw[k]);
					if (rc) FAIL(rc);
				}
			}
		}


#if 0


		// loop twice, first continue in the same direction as
		// the in/out pin, second loop in reverse direction
		for (direction = 0; direction <= 1; direction++) {
//printf("direction %i\n", direction);
			for (j = 0; j < set_l1.len; j++) {

//printf("j %i\n", j);
				rc = fpga_swset_fromto(tstate->model, switch_to.dest_y,
					switch_to.dest_x, CONNPT_STR16(switch_tile,
					SW_I(switch_tile->switches[set_l1.sw[j]], !from_to)),
					direction ? !from_to : from_to, &set_l2);
				if (rc) FAIL(rc);

#if 0
				rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
					&switch_to.set, switch_to.dest_y, switch_to.dest_x,
					set_l1.sw[j], NO_SWITCH);
				if (rc) FAIL(rc);
#endif

				for (k = 0; k < set_l2.len; k++) {
#if 0
					rc = test_logic_net(tstate, y, x, type_idx, dev->pinw_req_for_cfg[i],
						&switch_to.set, switch_to.dest_y, switch_to.dest_x,
						set_l1.sw[j], set_l2.sw[k]);
					if (rc) FAIL(rc);
#endif
				}
// fpga_swset_print(tstate->model, switch_to.dest_y, switch_to.dest_x, &set_l2, direction ? !from_to: from_to);
			}
		}
#endif
	}
	return 0;
fail:
	return rc;
}

// goal: use all switches in a routing switchbox
int test_logic_routing_switches(struct test_state* tstate);
int test_logic_routing_switches(struct test_state* tstate)
{
	int a_to_d[] = { A6_LUT, B6_LUT, C6_LUT, D6_LUT };
	int idx_enum[] = { DEV_LOGM, DEV_LOGX };
	int y, x, i, j, k, rc;
	swidx_t l2_done_list[MAX_SWITCHBOX_SIZE];
	int l2_done_len;
	str16_t done_pinw_list[2000];
	int done_pinw_len;
	
        y = 68;
	x = 13;

	done_pinw_len = 0;
	l2_done_len = 0;
	for (i = 0; i < sizeof(idx_enum)/sizeof(*idx_enum); i++) {
		for (j = 0; j < sizeof(a_to_d)/sizeof(*a_to_d); j++) {
			for (k = '1'; k <= '6'; k++) {
				rc = fdev_logic_set_lut(tstate->model, y, x,
					idx_enum[i], a_to_d[j], pf("A%c", k), ZTERM);
				if (rc) FAIL(rc);

				rc = test_logic_fingers(tstate, y, x, DEV_LOGIC,
					idx_enum[i], done_pinw_list, &done_pinw_len,
					l2_done_list, &l2_done_len);
				if (rc) FAIL(rc);
				fdev_delete(tstate->model, y, x, DEV_LOGIC, idx_enum[i]);
			}
		}
	}
#if 0
for (i = 0; i < done_list_len; i++) {
	printf("done %i %s\n", i, fpga_switch_print(tstate->model, 68, 12, done_list[i]));
}
#endif
	return 0;
fail:
	return rc;
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	struct fpga_device* P46_dev, *P48_dev, *logic_dev;
	int P46_y, P46_x, P46_dev_idx, P46_type_idx;
	int P48_y, P48_x, P48_dev_idx, P48_type_idx, logic_dev_idx, rc;
	struct test_state tstate;
	struct switch_to_yx switch_to;
	net_idx_t P46_net;

	// flush after every line is better for the autotest
	// output, tee, etc.
	// for example: ./autotest 2>&1 | tee autotest.log
	setvbuf(stdout, /*buf*/ 0, _IOLBF, /*size*/ 0);

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

#if 0
	rc = test_logic_config(&tstate);
	if (rc) FAIL(rc);
#endif

#if 1
	rc = test_logic_routing_switches(&tstate);
	if (rc) FAIL(rc);
#endif
	
	// test_iob_config
	// test_iologic_routing_switches

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
		logic_dev_idx, LOGIC_IN_D3);
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
			.start_switch = logic_dev->pinw[LOGIC_IN_D3],
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
	printf("O Test suite completed.\n");
	TIME_AND_MEM();
	printf("\n");
	return EXIT_SUCCESS;
fail:
	return rc;
}
