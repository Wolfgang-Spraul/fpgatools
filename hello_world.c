//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"

// Searches a connection in search_y/search_x that connects to
// target_y/target_x/target_pt.
int fpga_find_conn(struct fpga_model* model, int search_y, int search_x,
	str16_t* pt, int target_y, int target_x, str16_t target_pt)
{
	struct fpga_tile* tile;
	int i, j, dests_end;

	tile = YX_TILE(model, search_y, search_x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		dests_end = (i < tile->num_conn_point_names-1)
			? tile->conn_point_names[(i+1)*2]
			: tile->num_conn_point_dests;
		for (j = tile->conn_point_names[i*2]; j < dests_end; j++) {
			if (tile->conn_point_dests[j*3] == target_x
			    && tile->conn_point_dests[j*3+1] == target_y
			    && tile->conn_point_dests[j*3+2] == target_pt) {
				*pt = tile->conn_point_names[i*2+1];
				return 0;
			}
		}
	}
	*pt = STRIDX_NO_ENTRY;
	return 0;

}

static int froute_direct(struct fpga_model* model, int start_y, int start_x,
	str16_t start_pt, int end_y, int end_x, str16_t end_pt,
	struct sw_set* start_set, struct sw_set* end_set)
{
	struct sw_conns conns;
	struct sw_set end_switches;
	int i, rc;

printf("start %02i %02i %s end %02i %02i %s\n", start_y, start_x, strarray_lookup(&model->str, start_pt),
	end_y, end_x, strarray_lookup(&model->str, end_pt));

	rc = fpga_swset_fromto(model, end_y, end_x, end_pt, SW_TO, &end_switches);
	if (rc) FAIL(rc);
fpga_swset_print(model, end_y, end_x, &end_switches, SW_TO);
	if (!end_switches.len) FAIL(EINVAL);

// TODO: WR1 is not in model, so cannot connect...
	if (construct_sw_conns(&conns, model, start_y, start_x, start_pt, SW_FROM, /*max_depth*/ 1))
		{ HERE(); return; }
	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (conns.dest_y != end_y
		    || conns.dest_x != end_x) continue;
		for (i = 0; i < end_switches.len; i++) {
			if (conns.dest_str_i == fpga_switch_str_i(model, end_y, end_x, end_switches.sw[i], SW_FROM)) {
				*start_set = conns.chain.set;
				end_set->len = 1;
				end_set->sw[0] = end_switches.sw[i];
				return 0;
			}
		}
		printf("sw %s conn y%02i x%02i %s\n", fmt_swset(model, start_y, start_x,
			&conns.chain.set, SW_FROM),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&model->str, conns.dest_str_i));
	}
	printf("sw -\n");
	destruct_sw_conns(&conns);
	start_set->len = 0;
	end_set->len = 0;
	return 0;
fail:
	return rc;
}

static int fnet_autoroute(struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net_p;
	struct fpga_device* dev_p, *out_dev, *in_dev;
	struct switch_to_yx switch_to;
	struct sw_chain ch;
	struct sw_set logic_set, logic_route_set, iologic_route_set;
	str16_t route_pt;
	int i, out_i, in_i, rc;

	// todo: gnd and vcc nets are special and have no outpin
	//       but lots of inpins

	net_p = fnet_get(model, net_i);
	if (!net_p) FAIL(EINVAL);
	out_i = -1;
	in_i = -1;
	for (i = 0; i < net_p->len; i++) {
		if (!(net_p->el[i].idx & NET_IDX_IS_PINW)) {
			// net already routed?
			FAIL(EINVAL);
		}
		dev_p = FPGA_DEV(model, net_p->el[i].y,
			net_p->el[i].x, net_p->el[i].dev_idx);
		if ((net_p->el[i].idx & NET_IDX_MASK) < dev_p->num_pinw_in) {
			// todo: right now we only support 1 inpin
			if (in_i != -1) FAIL(ENOTSUP);
			in_i = i;
			continue;
		}
		if (out_i != -1) FAIL(EINVAL); // at most 1 outpin
		out_i = i;
	}
	// todo: vcc and gnd have no outpin?
	if (out_i == -1 || in_i == -1)
		FAIL(EINVAL);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y,
			net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y,
			net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	if (out_dev->type == DEV_IOB) {
		if (in_dev->type != DEV_LOGIC)
			FAIL(ENOTSUP);

		// switch to ilogic
		switch_to.yx_req = YX_DEV_ILOGIC;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = net_p->el[out_i].y;
		switch_to.x = net_p->el[out_i].x;
		switch_to.start_switch = out_dev->pinw[net_p->el[out_i].idx
			& NET_IDX_MASK];
		switch_to.from_to = SW_FROM;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch to ilogic routing
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_FROM;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch inside logic tile
		rc = fpga_swset_fromto(model, net_p->el[in_i].y,
			net_p->el[in_i].x,
			in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK],
			SW_TO, &logic_set);
		if (rc) FAIL(rc);
		if (logic_set.len != 1) FAIL(EINVAL);

		// switch in routing tile connecting to logic tile
		rc = fpga_find_conn(model,
			net_p->el[in_i].y, net_p->el[in_i].x-1, &route_pt,
			net_p->el[in_i].y, net_p->el[in_i].x,
			fpga_switch_str_i(model, net_p->el[in_i].y,
				net_p->el[in_i].x, logic_set.sw[0], SW_FROM));
		if (rc) FAIL(rc);
printf("%s\n", strarray_lookup(&model->str, route_pt));

		// route from ilogic routing to logic routing
		rc = froute_direct(model, switch_to.dest_y, switch_to.dest_x,
			switch_to.dest_connpt,
			net_p->el[in_i].y, net_p->el[in_i].x-1, route_pt,
			&iologic_route_set, &logic_route_set);
		if (rc) FAIL(rc);
#if 0
static int froute_direct(struct fpga_model* model, int start_y, int start_x,
	str16_t start_pt, int end_y, int end_x, str16_t end_pt,
	int from_to, struct sw_set* sw)

		switch_to.yx_req = YX_ROUTING_TO_FABLOGIC;
		switch_to.flags = SWTO_YX_CLOSEST;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_FROM;
printf("from1 y%02i x%02i %s\n", switch_to.dest_y, switch_to.dest_x,
	strarray_lookup(&model->str, switch_to.dest_connpt));
67/12/LOGICOUT7
 -> WR1B1
 -> 68/12 WR1E1
 -> LOGICIN_B30
params: 67/12/LOGICOUT7
        dest: 68/12/LOGICIN_B30

func:
   SW_TO LOGICIN_B30
   go through, call switch_to with target_connpt

		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		switch_to.yx_req = YX_DEV_LOGIC;
		switch_to.flags = SWTO_YX_TARGET_CONNPT|SWTO_YX_MAX_SWITCH_DEPTH;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_FROM;
		switch_to.max_switch_depth = 1;
printf("from y%02i x%02i %s\n", switch_to.dest_y, switch_to.dest_x,
	strarray_lookup(&model->str, switch_to.dest_connpt));

		rc = construct_sw_chain(&ch, model, switch_to.dest_y,
			switch_to.dest_x+1, in_dev->pinw[net_p->el[in_i].idx
			& NET_IDX_MASK], SW_TO, /*max_depth*/ -1,
			/*block*/ 0, 0);
		if (rc) FAIL(rc);
		if (fpga_switch_chain(&ch) == NO_CONN) FAIL(EINVAL);
		if (!ch.set.len) { HERE(); FAIL(EINVAL); }
		switch_to.target_connpt = fpga_switch_str_i(model,
			switch_to.dest_y, switch_to.dest_x+1,
			ch.set.sw[ch.set.len-1], SW_FROM);
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
printf_switch_to_yx_result(&switch_to);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);
#endif

		return 0;
	}
	if (out_dev->type == DEV_LOGIC) {
		if (in_dev->type != DEV_IOB)
			FAIL(ENOTSUP);
		return 0;
	}
	FAIL(ENOTSUP);
fail:
	return rc;
}

int main(int argc, char** argv)
{
	struct fpga_model model;
	int iob_inA_y, iob_inA_x, iob_inA_type_idx;
	int iob_inB_y, iob_inB_x, iob_inB_type_idx;
	int iob_out_y, iob_out_x, iob_out_type_idx;
	int logic_y, logic_x, logic_type_idx;
	net_idx_t inA_net, inB_net, out_net;
	int rc;

	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
		XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING))) FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P45", &iob_inA_y, &iob_inA_x,
		&iob_inA_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_input(&model, iob_inA_y, iob_inA_x,
		iob_inA_type_idx))) FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P46", &iob_inB_y, &iob_inB_x,
		&iob_inB_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_input(&model, iob_inB_y, iob_inB_x,
		iob_inB_type_idx))) FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P48", &iob_out_y, &iob_out_x,
		&iob_out_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_output(&model, iob_out_y, iob_out_x,
		iob_out_type_idx))) FAIL(rc);

	logic_y = 68;
	logic_x = 13;
	logic_type_idx = DEV_LOGM;
	if ((rc = fdev_logic_set_lut(&model, logic_y, logic_x, logic_type_idx,
		LUT_A, 6, "A1*A2", ZTERM))) FAIL(rc);

	if ((rc = fnet_new(&model, &inA_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inA_net, iob_inA_y, iob_inA_x,
		DEV_IOB, iob_inA_type_idx, IOB_OUT_I))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inA_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_A1))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, inA_net))) FAIL(rc);

	if ((rc = fnet_new(&model, &inB_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inB_net, iob_inB_y, iob_inB_x,
		DEV_IOB, iob_inB_type_idx, IOB_OUT_I))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inB_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_A2))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, inB_net))) FAIL(rc);

	if ((rc = fnet_new(&model, &out_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, out_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LO_A))) FAIL(rc);
	if ((rc = fnet_add_port(&model, out_net, iob_out_y, iob_out_x,
		DEV_IOB, iob_out_type_idx, IOB_IN_O))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, out_net))) FAIL(rc);
#if 0
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
	rc = fnet_add_switches(&model, P46_net, switch_to.y,
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
		rc = fnet_add_switches(&model, P46_net, c.y, c.x, &c.set);
		if (rc) FAIL(rc);
	}
#endif
	if ((rc = write_floorplan(stdout, &model, FP_DEFAULT))) FAIL(rc);
	return EXIT_SUCCESS;
fail:
	return rc;
}
