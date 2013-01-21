//
// Author (C version): Wolfgang Spraul
// Author (Verilog version): Xiangfu Liu
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "floorplan.h"
#include "control.h"

/*
   This C design corresponds to the following Verilog:
TODO
*/

int main(int argc, char** argv)
{
	struct fpga_model model;
	const char *param_clock_pin, *param_led_pin;
	int iob_clk_y, iob_clk_x, iob_clk_type_idx;
	int iob_led_y, iob_led_x, iob_led_type_idx;

	if (cmdline_help(argc, argv)) {
		printf("\n");
		return 0;
	}
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

	write_floorplan(stdout, &model, FP_DEFAULT);
	return fpga_free_model(&model);
}
