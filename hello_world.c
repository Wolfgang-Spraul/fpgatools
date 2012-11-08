//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"
#include "parts.h"

/*
   This C design corresponds to the following Verilog:
  
   module ver_and(input a, b, output y);

   // synthesis attribute LOC a "P45"
   // synthesis attribute LOC b "P46"
   // synthesis attribute LOC y "P48 | IOSTANDARD = LVCMOS33 | SLEW = SLOW | DRIVE = 12"

    assign y = a & b;

   endmodule
*/

int main(int argc, char** argv)
{
	struct fpga_model model;
	int iob_inA_y, iob_inA_x, iob_inA_type_idx;
	int iob_inB_y, iob_inB_x, iob_inB_type_idx;
	int iob_out_y, iob_out_x, iob_out_type_idx;
	int logic_y, logic_x, logic_type_idx;
	net_idx_t inA_net, inB_net, out_net;
	int rc;

	if ((rc = fpga_build_model(&model, XC6SLX9, XC6SLX9_ROWS,
		XC6SLX9_COLUMNS, XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
			FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P45", &iob_inA_y, &iob_inA_x,
		&iob_inA_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_input(&model, iob_inA_y, iob_inA_x,
		iob_inA_type_idx, IO_LVCMOS33))) FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P46", &iob_inB_y, &iob_inB_x,
		&iob_inB_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_input(&model, iob_inB_y, iob_inB_x,
		iob_inB_type_idx, IO_LVCMOS33))) FAIL(rc);

	if ((rc = fpga_find_iob(&model, "P48", &iob_out_y, &iob_out_x,
		&iob_out_type_idx))) FAIL(rc);
	if ((rc = fdev_iob_output(&model, iob_out_y, iob_out_x,
		iob_out_type_idx, IO_LVCMOS33))) FAIL(rc);

	logic_y = 68;
	logic_x = 13;
	logic_type_idx = DEV_LOG_X;
	if ((rc = fdev_logic_a2d_lut(&model, logic_y, logic_x, logic_type_idx,
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
