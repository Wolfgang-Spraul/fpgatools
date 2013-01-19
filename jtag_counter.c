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
  
// There are 5 registers
//  0x0 R  Firmware version
//  0x1 RW Counter enable/disable, enable: 1, disable: 0
//  0x2 RW Counter write enable,  enable: 1, disable: 0
//  0x3 RW Counter write Value
//  0x4 R  Counter current value

// LED1, on: stop counting, off: counting
// LED2, on: counter > 0, off: counter == 0

module counter(clk, led1, led2);
   input clk;
   output led1;
   output led2;

   // synthesis attribute LOC clk "T8 | IOSTANDARD = LVCMOS33"
   // synthesis attribute LOC led "R5 | SLEW = QUIETIO | DRIVE = 8"

   reg 	  counter_start = 1'b0;
   reg 	  counter_we = 1'b0;
   reg [31:0] counter = 32'd0;
   reg [31:0] counter_set = 32'd0;
   always @(posedge clk)
     begin 
	if (counter_start == 1'b1)
	  counter <= counter + 1;
	if (counter_we == 1'b1)
	  counter <= counter_set;
     end

   assign led1 = ~counter_start;
   assign led2 = counter;
   
   wire  jt_capture, jt_drck, jt_reset, jt_sel;
   wire  jt_shift, jt_tck, jt_tdi, jt_update, jt_tdo;
   
   BSCAN_SPARTAN6 # (.JTAG_CHAIN(1)) jtag_blk (
					       .CAPTURE(jt_capture),
					       .DRCK(jt_drck),
					       .RESET(jt_reset),
					       .RUNTEST(),
					       .SEL(jt_sel),
					       .SHIFT(jt_shift),
					       .TCK(jt_tck),
					       .TDI(jt_tdi),
					       .TDO(jt_tdo),
					       .TMS(),
					       .UPDATE(jt_update)
					       );

   reg [37:0] dr;
   reg [3:0]  addr = 4'hF;
   reg 	      checksum;
   wire       checksum_valid = ~checksum;
   wire       jtag_we = dr[36];
   wire [3:0] jtag_addr = dr[35:32];

   assign jt_tdo = dr[0];

   always @ (posedge jt_tck)
     begin
	if (jt_reset == 1'b1)
	  begin
	     dr <= 38'd0;
	  end
	else if (jt_capture == 1'b1)
	  begin
	     checksum <= 1'b1;
	     dr[37:32] <= 6'd0;
	     addr <= 4'hF;
	     case (addr)
	       4'h0: dr[31:0] <= 32'h20120911;
	       4'h1: dr[0] <= counter_start;
	       4'h2: dr[0] <= counter_we;
	       4'h3: dr[31:0] <= counter_set;
	       4'h4: dr[31:0] <= counter;

	       default: dr[31:0] <= 32'habcdef01;
	     endcase
	  end
	else if (jt_shift == 1'b1)
	  begin
	     dr <= {jt_tdi, dr[37:1]};
	     checksum <= checksum ^ jt_tdi;
	  end
	else if (jt_update & checksum_valid)
	  begin
	     addr <= jtag_addr;
	     if (jtag_we)
	       begin
		  case (jtag_addr)
		    4'h1: counter_start <= dr[0];
		    4'h2: counter_we <= dr[0];
		    4'h3: counter_set <= dr[31:0];
		  endcase
	       end
	  end
     end
endmodule
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
