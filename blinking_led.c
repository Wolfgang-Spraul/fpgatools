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
  
   module blinking(input clk, output led);

   // synthesis attribute LOC clk "P55 | IOSTANDARD = LVCMOS33"
   // synthesis attribute LOC led "P48 | SLEW = QUIETIO | DRIVE = 8"

     reg [14:0] counter;
     always @(posedge clk) counter <= counter + 1;
     assign led = counter[14];

   endmodule
*/

int main(int argc, char** argv)
{
	struct fpga_model model;
	int iob_clk_y, iob_clk_x, iob_clk_type_idx;
	int iob_led_y, iob_led_x, iob_led_type_idx;
	int logic_y, logic_x, logic_type_idx;
	struct fpgadev_logic logic_cfg;
	net_idx_t inA_net, inB_net, out_net;

	fpga_build_model(&model, XC6SLX9);

	fpga_find_iob(&model, "P55", &iob_clk_y, &iob_clk_x, &iob_clk_type_idx);
	fdev_iob_input(&model, iob_clk_y, iob_clk_x, iob_clk_type_idx,
		IO_LVCMOS33);

	fpga_find_iob(&model, "P48", &iob_led_y, &iob_led_x, &iob_led_type_idx);
	fdev_iob_output(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		IO_LVCMOS25);
	fdev_iob_slew(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		SLEW_QUIETIO);
	fdev_iob_drive(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		8);

#if 0
	logic_y = 55-58
	logic_x = 13;
	logic_type_idx = DEV_LOG_X;
	if ((rc = fdev_logic_a2d_lut(&model, logic_y, logic_x, logic_type_idx,
		LUT_D, 6, "A3*A5", ZTERM))) FAIL(rc);

			memset(&logic_cfg, 0, sizeof(logic_cfg));
			logic_cfg.a2d[LUT_A].lut6 = "(A6+~A6)*(~A5)";
			logic_cfg.a2d[LUT_A].lut5 = "1";
			logic_cfg.a2d[LUT_A].cy0 = CY0_O5;
			logic_cfg.a2d[LUT_A].ff = FF_FF;
			logic_cfg.a2d[LUT_A].ff_mux = MUX_XOR;
			logic_cfg.a2d[LUT_A].ff_srinit = FF_SRINIT0;
			logic_cfg.a2d[LUT_B].lut6 = "(A6+~A6)*(A5)";
			logic_cfg.a2d[LUT_B].lut5 = "1";
			logic_cfg.a2d[LUT_B].cy0 = CY0_O5;
			logic_cfg.a2d[LUT_B].ff = FF_FF;
			logic_cfg.a2d[LUT_B].ff_mux = MUX_XOR;
			logic_cfg.a2d[LUT_B].ff_srinit = FF_SRINIT0;
			logic_cfg.a2d[LUT_C].lut6 = "(A6+~A6)*(A5)";
			logic_cfg.a2d[LUT_C].lut5 = "1";
			logic_cfg.a2d[LUT_C].cy0 = CY0_O5;
			logic_cfg.a2d[LUT_C].ff = FF_FF;
			logic_cfg.a2d[LUT_C].ff_mux = MUX_XOR;
			logic_cfg.a2d[LUT_C].ff_srinit = FF_SRINIT0;
			logic_cfg.a2d[LUT_D].lut6 = "(A6+~A6)*(A5)";
			logic_cfg.a2d[LUT_D].lut5 = "1";
			logic_cfg.a2d[LUT_D].cy0 = CY0_O5;
			logic_cfg.a2d[LUT_D].ff = FF_FF;
			logic_cfg.a2d[LUT_D].ff_mux = MUX_XOR;
			logic_cfg.a2d[LUT_D].ff_srinit = FF_SRINIT0;

			logic_cfg.clk_inv = CLKINV_CLK;
			logic_cfg.sync_attr = SYNCATTR_ASYNC;
			logic_cfg.precyinit = PRECYINIT_0;
			logic_cfg.cout_used = 1;
	logic_setconf(tstate->model, y, x, type_idx, logic_cfg);


	fnet_new(&model, &inA_net);
	fnet_add_port(&model, inA_net, iob_inA_y, iob_inA_x,
		DEV_IOB, iob_inA_type_idx, IOB_OUT_I);
	fnet_add_port(&model, inA_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_D3);
	fnet_autoroute(&model, inA_net);

	fnet_new(&model, &inB_net);
	fnet_add_port(&model, inB_net, iob_inB_y, iob_inB_x,
		DEV_IOB, iob_inB_type_idx, IOB_OUT_I);
	fnet_add_port(&model, inB_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LI_D5);
	fnet_autoroute(&model, inB_net);

	fnet_new(&model, &out_net);
	fnet_add_port(&model, out_net, logic_y, logic_x, DEV_LOGIC,
		logic_type_idx, LO_D);
	fnet_add_port(&model, out_net, iob_out_y, iob_out_x,
		DEV_IOB, iob_out_type_idx, IOB_IN_O);
	fnet_autoroute(&model, out_net);
#endif

	write_floorplan(stdout, &model, FP_DEFAULT);
	return fpga_free_model(&model);
}
