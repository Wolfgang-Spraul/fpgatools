//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"

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
	net_idx_t net;

	fpga_build_model(&model, XC6SLX9, TQG144);

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

	logic_y = 58;
	logic_x = 13;
	logic_type_idx = DEV_LOG_M_OR_L;

	memset(&logic_cfg, 0, sizeof(logic_cfg));
	logic_cfg.a2d[LUT_A].lut6 = "(A6+~A6)*(~A5)";
	logic_cfg.a2d[LUT_A].lut5 = "1";
	logic_cfg.a2d[LUT_A].cy0 = CY0_O5;
	logic_cfg.a2d[LUT_A].ff = FF_FF;
	logic_cfg.a2d[LUT_A].ff_mux = MUX_XOR;
	logic_cfg.a2d[LUT_A].ff_srinit = FF_SRINIT0;
	logic_cfg.a2d[LUT_B].lut6 = "(A6+~A6)*(A5)";
	logic_cfg.a2d[LUT_B].lut5 = "0";
	logic_cfg.a2d[LUT_B].cy0 = CY0_O5;
	logic_cfg.a2d[LUT_B].ff = FF_FF;
	logic_cfg.a2d[LUT_B].ff_mux = MUX_XOR;
	logic_cfg.a2d[LUT_B].ff_srinit = FF_SRINIT0;
	logic_cfg.a2d[LUT_C].lut6 = "(A6+~A6)*(A5)";
	logic_cfg.a2d[LUT_C].lut5 = "0";
	logic_cfg.a2d[LUT_C].cy0 = CY0_O5;
	logic_cfg.a2d[LUT_C].ff = FF_FF;
	logic_cfg.a2d[LUT_C].ff_mux = MUX_XOR;
	logic_cfg.a2d[LUT_C].ff_srinit = FF_SRINIT0;
	logic_cfg.a2d[LUT_D].lut6 = "(A6+~A6)*(A5)";
	logic_cfg.a2d[LUT_D].lut5 = "0";
	logic_cfg.a2d[LUT_D].cy0 = CY0_O5;
	logic_cfg.a2d[LUT_D].ff = FF_FF;
	logic_cfg.a2d[LUT_D].ff_mux = MUX_XOR;
	logic_cfg.a2d[LUT_D].ff_srinit = FF_SRINIT0;

	logic_cfg.clk_inv = CLKINV_CLK;
	logic_cfg.sync_attr = SYNCATTR_ASYNC;
	logic_cfg.precyinit = PRECYINIT_0;
	logic_cfg.cout_used = 1;
	fdev_logic_setconf(&model, logic_y, logic_x, logic_type_idx, &logic_cfg);

	logic_y = 57;
	logic_cfg.precyinit = 0;
	logic_cfg.a2d[LUT_A].lut6 = "(A6+~A6)*(A5)";
	logic_cfg.a2d[LUT_A].lut5 = "0";
	fdev_logic_setconf(&model, logic_y, logic_x, logic_type_idx, &logic_cfg);

	logic_y = 56;
	fdev_logic_setconf(&model, logic_y, logic_x, logic_type_idx, &logic_cfg);

	logic_y = 55;
	logic_cfg.cout_used = 0;
	logic_cfg.a2d[LUT_C].lut6 = "A5";
	logic_cfg.a2d[LUT_C].lut5 = 0;
	logic_cfg.a2d[LUT_C].cy0 = 0;
	logic_cfg.a2d[LUT_C].ff = FF_FF;
	logic_cfg.a2d[LUT_C].ff_mux = MUX_XOR;
	logic_cfg.a2d[LUT_C].ff_srinit = FF_SRINIT0;
	memset(&logic_cfg.a2d[LUT_D], 0, sizeof(logic_cfg.a2d[LUT_D]));
	fdev_logic_setconf(&model, logic_y, logic_x, logic_type_idx, &logic_cfg);

	// clock to logic devs
	fnet_new(&model, &net);
	fnet_add_port(&model, net, iob_clk_y, iob_clk_x, DEV_IOB,
		iob_clk_type_idx, IOB_OUT_I);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CLK);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CLK);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CLK);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CLK);
	fnet_route(&model, net);

	// vcc to logic devs
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_A6);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_B6);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_C6);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_D6);
	fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_A6);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_B6);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_C6);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_D6);
	fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_A6);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_B6);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_C6);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_D6);
	fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_A6);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_B6);
	fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);

	// carry chain
#if 0
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_route(&model, net);

	fnet_new(&model, &net);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_route(&model, net);

	fnet_new(&model, &net);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_route(&model, net);
#endif

	write_floorplan(stdout, &model, FP_DEFAULT);
	return fpga_free_model(&model);
}
