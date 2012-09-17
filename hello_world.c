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
	int iob_inA_y, iob_inA_x, iob_inA_type_idx;
	int iob_inB_y, iob_inB_x, iob_inB_type_idx;
	int iob_out_y, iob_out_x, iob_out_type_idx;
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

	if ((rc = fdev_logic_set_lut(&model, 68, 13, DEV_LOGM,
		LUT_A, 6, "A1*A2", ZTERM))) FAIL(rc);

	// fnet_new
	// fnet_add_port
	// fnet_add_port
	// fnet_autoroute
	// fnet_autoroute
	// P45.I -> logic.A1
	// P46.I -> logic.A2
	// logic.A -> P48.O
#if 0
	struct fpga_device* P46_dev, *P48_dev, *logic_dev;
	int P46_y, P46_x, P46_dev_idx, P46_type_idx;
	int P48_y, P48_x, P48_dev_idx, P48_type_idx, logic_dev_idx, rc;
	net_idx_t P46_net;
	struct switch_to_yx switch_to;
	int iob_y, iob_x, iob_type_idx, rc;
	struct fpga_device* iob_dev;
	net_idx_t net;

	// configure net from P46.I to logic.D3
	rc = fnet_new(&model, &P46_net);
	if (rc) FAIL(rc);
	rc = fnet_add_port(&model, P46_net, P46_y, P46_x,
		P46_dev_idx, IOB_OUT_I);
	if (rc) FAIL(rc);
	rc = fnet_add_port(&model, P46_net, /*y*/ 68, /*x*/ 13,
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
	rc = fnet_add_switches(&model, P46_net, switch_to.y,
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
	rc = fnet_add_switches(&model, P46_net, switch_to.y,
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
	rc = fnet_add_switches(&model, P46_net, switch_to.y,
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
