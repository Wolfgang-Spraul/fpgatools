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
	int param_bits;
	const char *param_clock_pin, *param_led_pin;
	int iob_clk_y, iob_clk_x, iob_clk_type_idx;
	int iob_led_y, iob_led_x, iob_led_type_idx;
	int logic_y, logic_x, logic_type_idx;
	struct fpgadev_logic logic_cfg;
	net_idx_t net;

	if (cmdline_help(argc, argv)) {
		printf( "       %*s [-Dbits=14|23]\n"
			"       %*s [-Dclock_pin=IO_L30N_GCLK0_USERCCLK_2|...]\n"
			"       %*s [-Dled_pin=IO_L48P_D7_2|...]\n"
			"\n", (int) strlen(*argv), "",
			(int) strlen(*argv), "", (int) strlen(*argv), "");
		return 0;
	}
	if (!(param_bits = cmdline_intvar(argc, argv, "bits")))
		param_bits = 14;

	if (!(param_clock_pin = cmdline_strvar(argc, argv, "clock_pin")))
		param_clock_pin = "IO_L30N_GCLK0_USERCCLK_2";
	if (!(param_led_pin = cmdline_strvar(argc, argv, "led_pin")))
		param_led_pin = "IO_L48P_D7_2";

	fpga_build_model(&model, cmdline_part(argc, argv),
		cmdline_package(argc, argv));

	fpga_find_iob(&model, xc6_find_pkg_pin(model.pkg, param_clock_pin),
		&iob_clk_y, &iob_clk_x, &iob_clk_type_idx);
	fdev_iob_input(&model, iob_clk_y, iob_clk_x, iob_clk_type_idx,
		IO_LVCMOS33);

	fpga_find_iob(&model, xc6_find_pkg_pin(model.pkg, param_led_pin),
		&iob_led_y, &iob_led_x, &iob_led_type_idx);
	fdev_iob_output(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		IO_LVCMOS25);
	fdev_iob_slew(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		SLEW_QUIETIO);
	fdev_iob_drive(&model, iob_led_y, iob_led_x, iob_led_type_idx,
		8);

	logic_y = 58;
	logic_x = 13;
	logic_type_idx = DEV_LOG_M_OR_L;

	CLEAR(logic_cfg);
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
	CLEAR(logic_cfg.a2d[LUT_D]);
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
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_C6);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_D6);
	fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);

	// carry chain
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_route(&model, net);

	fnet_new(&model, &net);
	fnet_add_port(&model, net, 56, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_route(&model, net);

	fnet_new(&model, &net);
	fnet_add_port(&model, net, 57, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_CIN);
	fnet_add_port(&model, net, 58, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_COUT);
	fnet_route(&model, net);

	// bit chain
	{
		int out_pin[] = {LO_AQ, LO_BQ, LO_CQ, LO_DQ};
		int in_pin[] = {LI_A5, LI_B5, LI_C5, LI_D5};
		int cur_y, i;

		for (cur_y = 58; cur_y >= 55; cur_y--) {
			for (i = 0; i < 4; i++) {
				if (cur_y == 55 && i >= 2)
					break;
				fnet_new(&model, &net);
				fnet_add_port(&model, net, cur_y, 13, DEV_LOGIC, DEV_LOG_M_OR_L, out_pin[i]);
				fnet_add_port(&model, net, cur_y, 13, DEV_LOGIC, DEV_LOG_M_OR_L, in_pin[i]);
				fnet_route(&model, net);
			}
		}
	}
	fnet_new(&model, &net);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LO_CQ);
	fnet_add_port(&model, net, 55, 13, DEV_LOGIC, DEV_LOG_M_OR_L, LI_C5);
	fnet_add_port(&model, net, iob_led_y, iob_led_x, DEV_IOB,
		iob_led_type_idx, IOB_IN_O);
	fnet_route(&model, net);

	write_floorplan(stdout, &model, FP_DEFAULT);
	return fpga_free_model(&model);
}
