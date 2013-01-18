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

     // COUNTER_SIZE tested as 14 (32K crystal) and 23 (20M crystal)
     `define COUNTER_SIZE 23

     reg [`COUNTER_SIZE:0] counter;
     always @(posedge clk) counter <= counter + 1;
     assign led = counter[`COUNTER_SIZE];

   endmodule
*/

int main(int argc, char** argv)
{
	struct fpga_model model;
	int param_highest_bit;
	const char *param_clock_pin, *param_led_pin;
	int iob_clk_y, iob_clk_x, iob_clk_type_idx;
	int iob_led_y, iob_led_x, iob_led_type_idx;
	int logic_x, logic_type_idx, cur_bit;
	int cur_y, next_y, i;
	struct fpgadev_logic logic_cfg;
	net_idx_t clock_net, net;

	// todo: test with clock=C10/IO_L37N_GCLK12_0, led=M16/IO_L46N_FOE_B_M1DQ3_1
	//	 currently: T8/R5
	// todo: we could support more ways to specify a pin:
	//       "bpp:0,30,1" - die-specific bank/pair/positive
	//       "name:T8" - package-specific pin (including unbonded ones)
	//       "desc:IO_L30N_2" - package-specific description
	if (cmdline_help(argc, argv)) {
		printf( "       %*s [-Dhighest_bit=14|23]\n"
			"       %*s [-Dclock_pin=desc:IO_L30N_GCLK0_USERCCLK_2|...]\n"
			"       %*s [-Dled_pin=desc:IO_L48P_D7_2|...]\n"
			"\n", (int) strlen(*argv), "",
			(int) strlen(*argv), "", (int) strlen(*argv), "");
		return 0;
	}
	if (!(param_highest_bit = cmdline_intvar(argc, argv, "highest_bit")))
		param_highest_bit = 14;

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

	// todo: temporary because our routing is so fragile...
	if (param_highest_bit == 14)
		cur_y = 58;
	else
		cur_y = 52;
	logic_x = 13;
	logic_type_idx = DEV_LOG_M_OR_L;

	// clock net
	fnet_new(&model, &clock_net);
	fnet_add_port(&model, clock_net, iob_clk_y, iob_clk_x, DEV_IOB,
		iob_clk_type_idx, IOB_OUT_I);

	for (cur_bit = 0; cur_bit <= param_highest_bit; cur_bit++) {
		RC_ASSERT(&model, cur_y != -1);
		if (!(cur_bit % 4)) { // beginning of slice
			CLEAR(logic_cfg);
			logic_cfg.clk_inv = CLKINV_CLK;
			logic_cfg.sync_attr = SYNCATTR_ASYNC;
		}
		if (!cur_bit) { // first bit
			logic_cfg.precyinit = PRECYINIT_0;
			logic_cfg.a2d[LUT_A].lut6 = "(A6+~A6)*(~A5)";
			logic_cfg.a2d[LUT_A].lut5 = "1";
			logic_cfg.a2d[LUT_A].cy0 = CY0_O5;
			logic_cfg.a2d[LUT_A].ff = FF_FF;
			logic_cfg.a2d[LUT_A].ff_mux = MUX_XOR;
			logic_cfg.a2d[LUT_A].ff_srinit = FF_SRINIT0;
		} else if (cur_bit == param_highest_bit) {
			logic_cfg.a2d[cur_bit%4].lut6 = "A5";
			logic_cfg.a2d[cur_bit%4].ff = FF_FF;
			logic_cfg.a2d[cur_bit%4].ff_mux = MUX_XOR;
			logic_cfg.a2d[cur_bit%4].ff_srinit = FF_SRINIT0;
		} else {
			logic_cfg.a2d[cur_bit%4].lut6 = "(A6+~A6)*(A5)";
			logic_cfg.a2d[cur_bit%4].lut5 = "0";
			logic_cfg.a2d[cur_bit%4].cy0 = CY0_O5;
			logic_cfg.a2d[cur_bit%4].ff = FF_FF;
			logic_cfg.a2d[cur_bit%4].ff_mux = MUX_XOR;
			logic_cfg.a2d[cur_bit%4].ff_srinit = FF_SRINIT0;
		}
		if (cur_bit%4 == 3 || cur_bit == param_highest_bit) {
			static const int out_pin[] = {LO_AQ, LO_BQ, LO_CQ, LO_DQ};
			static const int in_pin[] = {LI_A5, LI_B5, LI_C5, LI_D5};

			next_y = regular_row_up(cur_y, &model);
			if (cur_bit < param_highest_bit) {
				RC_ASSERT(&model, next_y != -1);
				logic_cfg.cout_used = 1;
				// carry chain
				fnet_new(&model, &net);
				fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LO_COUT);
				fnet_add_port(&model, net, next_y, logic_x, DEV_LOGIC, logic_type_idx, LI_CIN);
				fnet_route(&model, net);
			}
			fdev_logic_setconf(&model, cur_y, logic_x, logic_type_idx, &logic_cfg);

			// clock net
			fnet_add_port(&model, clock_net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LI_CLK);

			// lut5 net (drive vcc into A6 to enable lut5)
			if (logic_cfg.a2d[LUT_A].lut5
			    || logic_cfg.a2d[LUT_B].lut5
			    || logic_cfg.a2d[LUT_C].lut5
			    || logic_cfg.a2d[LUT_D].lut5) {
				fnet_new(&model, &net);
				if (logic_cfg.a2d[LUT_A].lut5)
					fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LI_A6);
				if (logic_cfg.a2d[LUT_B].lut5)
					fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LI_B6);
				if (logic_cfg.a2d[LUT_C].lut5)
					fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LI_C6);
				if (logic_cfg.a2d[LUT_D].lut5)
					fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, LI_D6);
				fnet_vcc_gnd(&model, net, /*is_vcc*/ 1);
			}
			for (i = 0; i <= cur_bit%4; i++) {
				fnet_new(&model, &net);
				fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, out_pin[i]);
				fnet_add_port(&model, net, cur_y, logic_x, DEV_LOGIC, logic_type_idx, in_pin[i]);
				if (cur_bit - cur_bit%4 + i == param_highest_bit)
					fnet_add_port(&model, net, iob_led_y, iob_led_x, DEV_IOB,
						iob_led_type_idx, IOB_IN_O);
				fnet_route(&model, net);
			}
			cur_y = next_y;
		}
	}
	fnet_route(&model, clock_net);
	write_floorplan(stdout, &model, FP_DEFAULT);
	return fpga_free_model(&model);
}
