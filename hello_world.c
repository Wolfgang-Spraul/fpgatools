//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"

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

	rc = fpga_swset_fromto(model, end_y, end_x, end_pt, SW_TO, &end_switches);
	if (rc) FAIL(rc);
	if (!end_switches.len) FAIL(EINVAL);

	rc = construct_sw_conns(&conns, model, start_y, start_x, start_pt,
		SW_FROM, /*max_depth*/ 1);
	if (rc) FAIL(rc);

	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (conns.dest_y != end_y
		    || conns.dest_x != end_x) continue;
		for (i = 0; i < end_switches.len; i++) {
			if (conns.dest_str_i == fpga_switch_str_i(model, end_y,
			    end_x, end_switches.sw[i], SW_FROM)) {
				*start_set = conns.chain.set;
				end_set->len = 1;
				end_set->sw[0] = end_switches.sw[i];
				return 0;
			}
		}
	}
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
	struct sw_set logic_route_set, iologic_route_set;
	struct switch_to_rel switch_to_rel;
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

		// switch from logic tile to logic routing tile
		switch_to_rel.model = model;
		switch_to_rel.start_y = net_p->el[in_i].y;
		switch_to_rel.start_x = net_p->el[in_i].x;
		switch_to_rel.start_switch =
			in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK];
		switch_to_rel.from_to = SW_TO;
		switch_to_rel.rel_y = 0;
		switch_to_rel.rel_x = -1;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		rc = fpga_switch_to_rel(&switch_to_rel);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.start_y,
			switch_to_rel.start_x, switch_to_rel.set.sw,
			switch_to_rel.set.len);
		if (rc) FAIL(rc);

		// route from ilogic routing to logic routing
		rc = froute_direct(model, switch_to.dest_y, switch_to.dest_x,
			switch_to.dest_connpt,
			switch_to_rel.dest_y, switch_to_rel.dest_x,
			switch_to_rel.dest_connpt,
			&iologic_route_set, &logic_route_set);
		if (rc) FAIL(rc);
		if (!iologic_route_set.len || !logic_route_set.len) FAIL(EINVAL);
		rc = fnet_add_sw(model, net_i, switch_to.dest_y, switch_to.dest_x,
			iologic_route_set.sw, iologic_route_set.len);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, net_p->el[in_i].y, net_p->el[in_i].x-1,
			logic_route_set.sw, logic_route_set.len);
		if (rc) FAIL(rc);

		return 0;
	}
	if (out_dev->type == DEV_LOGIC) {
		if (in_dev->type != DEV_IOB)
			FAIL(ENOTSUP);

		// switch from ologic to iob
		switch_to.yx_req = YX_DEV_OLOGIC;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = net_p->el[in_i].y;
		switch_to.x = net_p->el[in_i].x;
		switch_to.start_switch = in_dev->pinw[net_p->el[in_i].idx
			& NET_IDX_MASK];
		switch_to.from_to = SW_TO;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switches inside ologic to ologic routing
		switch_to.yx_req = YX_ROUTING_TILE;
		switch_to.flags = SWTO_YX_DEF;
		switch_to.model = model;
		switch_to.y = switch_to.dest_y;
		switch_to.x = switch_to.dest_x;
		switch_to.start_switch = switch_to.dest_connpt;
		switch_to.from_to = SW_TO;
		rc = fpga_switch_to_yx(&switch_to);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
			switch_to.set.sw, switch_to.set.len);
		if (rc) FAIL(rc);

		// switch inside logic tile
		switch_to_rel.model = model;
		switch_to_rel.start_y = net_p->el[out_i].y;
		switch_to_rel.start_x = net_p->el[out_i].x;
		switch_to_rel.start_switch =
			out_dev->pinw[net_p->el[out_i].idx & NET_IDX_MASK];
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.rel_y = 0;
		switch_to_rel.rel_x = -1;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		rc = fpga_switch_to_rel(&switch_to_rel);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.start_y,
			switch_to_rel.start_x, switch_to_rel.set.sw,
			switch_to_rel.set.len);
		if (rc) FAIL(rc);

		// route from logic routing to ilogic routing
		rc = froute_direct(model, switch_to_rel.dest_y,
			switch_to_rel.dest_x, switch_to_rel.dest_connpt,
			switch_to.dest_y, switch_to.dest_x, switch_to.dest_connpt,
			&logic_route_set, &iologic_route_set);
		if (rc) FAIL(rc);
		if (!iologic_route_set.len || !logic_route_set.len) FAIL(EINVAL);
		rc = fnet_add_sw(model, net_i, switch_to.dest_y, switch_to.dest_x,
			iologic_route_set.sw, iologic_route_set.len);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_i, switch_to_rel.dest_y,
			switch_to_rel.dest_x, logic_route_set.sw,
			logic_route_set.len);
		if (rc) FAIL(rc);

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
	logic_type_idx = DEV_LOGX;
	if ((rc = fdev_logic_set_lut(&model, logic_y, logic_x, logic_type_idx,
		LUT_D, 6, "A3*A5", ZTERM))) FAIL(rc);

	if ((rc = fnet_new(&model, &inA_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inA_net, iob_inA_y, iob_inA_x,
		DEV_IOB, iob_inA_type_idx, IOB_OUT_I))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inA_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_D3))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, inA_net))) FAIL(rc);

	if ((rc = fnet_new(&model, &inB_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inB_net, iob_inB_y, iob_inB_x,
		DEV_IOB, iob_inB_type_idx, IOB_OUT_I))) FAIL(rc);
	if ((rc = fnet_add_port(&model, inB_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_D5))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, inB_net))) FAIL(rc);

	if ((rc = fnet_new(&model, &out_net))) FAIL(rc);
	if ((rc = fnet_add_port(&model, out_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LO_D))) FAIL(rc);
	if ((rc = fnet_add_port(&model, out_net, iob_out_y, iob_out_x,
		DEV_IOB, iob_out_type_idx, IOB_IN_O))) FAIL(rc);
	if ((rc = fnet_autoroute(&model, out_net))) FAIL(rc);

	if ((rc = write_floorplan(stdout, &model, FP_DEFAULT))) FAIL(rc);
	return EXIT_SUCCESS;
fail:
	return rc;
}
