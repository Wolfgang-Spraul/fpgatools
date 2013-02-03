//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include "helper.h"
#include "parts.h"

#define LEFT_SIDE_MAJOR 1

struct fpga_model
{
	int rc; // if rc != 0, all function calls will immediately return

	const struct xc_die *die;
	const struct xc6_pkg_info *pkg;

	int x_width, y_height;
	int center_x;
	int center_y;
	// Left and right gclk separators will be located on
	// the device column (+1 or +2) of the logic or dsp/macc
	// column as indicated in the chip's cfg_columns with a 'g'.
	int left_gclk_sep_x, right_gclk_sep_x;

	// x_major is an array of column indices for each x coordinate,
	// starting with column 1 for the left side, and incrementing
	// through the configuration columns. This corresponds to the
	// 'majors' in the bitstream.
	int x_major[512];

	struct xc6_routing_bitpos* sw_bitpos;
	int num_bitpos;

	struct fpga_tile* tiles;
	struct hashed_strarray str;

	int nets_array_size;
	int highest_used_net; // 1-based net_idx_t
	struct fpga_net* nets;

	// tmp_str will be allocated to hold max(x_width, y_height)
	// pointers, useful for string seeding when running wires.
	const char** tmp_str;
};

enum fpga_tile_type
{
	NA = 0,
	ROUTING, ROUTING_BRK, ROUTING_VIA,
	HCLK_ROUTING_XM, HCLK_ROUTING_XL, HCLK_LOGIC_XM, HCLK_LOGIC_XL,
	LOGIC_XM, LOGIC_XL,
	REGH_ROUTING_XM, REGH_ROUTING_XL, REGH_LOGIC_XM, REGH_LOGIC_XL,
	BRAM_ROUTING, BRAM_ROUTING_BRK,
	BRAM,
	BRAM_ROUTING_TERM_T, BRAM_ROUTING_TERM_B, BRAM_ROUTING_VIA_TERM_T, BRAM_ROUTING_VIA_TERM_B,
	BRAM_TERM_LT, BRAM_TERM_RT, BRAM_TERM_LB, BRAM_TERM_RB,
	HCLK_BRAM_ROUTING, HCLK_BRAM_ROUTING_VIA, HCLK_BRAM,
	REGH_BRAM_ROUTING, REGH_BRAM_ROUTING_VIA, REGH_BRAM_L, REGH_BRAM_R,
	MACC,
	HCLK_MACC_ROUTING, HCLK_MACC_ROUTING_VIA, HCLK_MACC,
	REGH_MACC_ROUTING, REGH_MACC_ROUTING_VIA, REGH_MACC_L,
	PLL_T, DCM_T, PLL_B, DCM_B, REG_T,
	REG_TERM_T, REG_TERM_B, REG_B,
	REGV_TERM_T, REGV_TERM_B,
	HCLK_REGV,
	REGV, REGV_BRK, REGV_T, REGV_B, REGV_MIDBUF_T, REGV_HCLKBUF_T, REGV_HCLKBUF_B, REGV_MIDBUF_B,
	REGC_ROUTING, REGC_LOGIC, REGC_CMT,
	CENTER, // unique center tile in the middle of the chip
	IO_T, IO_B, IO_TERM_T, IO_TERM_B, IO_ROUTING, IO_LOGIC_TERM_T, IO_LOGIC_TERM_B,
	IO_OUTER_T, IO_INNER_T, IO_OUTER_B, IO_INNER_B,
	IO_BUFPLL_TERM_T, IO_LOGIC_REG_TERM_T, IO_BUFPLL_TERM_B, IO_LOGIC_REG_TERM_B,
	LOGIC_ROUTING_TERM_B, LOGIC_NOIO_TERM_B,
	MACC_ROUTING_TERM_T, MACC_ROUTING_TERM_B, MACC_VIA_TERM_T,
	MACC_TERM_TL, MACC_TERM_TR, MACC_TERM_BL, MACC_TERM_BR,
	ROUTING_VIA_REGC, ROUTING_VIA_IO, ROUTING_VIA_IO_DCM, ROUTING_VIA_CARRY,
	CORNER_TERM_L, CORNER_TERM_R,
	IO_TERM_L_UPPER_TOP, IO_TERM_L_UPPER_BOT, IO_TERM_L_LOWER_TOP, IO_TERM_L_LOWER_BOT,
	IO_TERM_R_UPPER_TOP, IO_TERM_R_UPPER_BOT, IO_TERM_R_LOWER_TOP, IO_TERM_R_LOWER_BOT,
	IO_TERM_L, IO_TERM_R,
	HCLK_TERM_L, HCLK_TERM_R,
	REGH_IO_TERM_L, REGH_IO_TERM_R,
	REG_L, REG_R,
	IO_PCI_L, IO_PCI_R, IO_RDY_L, IO_RDY_R,
	IO_L, IO_R,
	IO_PCI_CONN_L, IO_PCI_CONN_R,
	CORNER_TERM_T, CORNER_TERM_B,
	ROUTING_IO_L,
	HCLK_ROUTING_IO_L, HCLK_ROUTING_IO_R, REGH_ROUTING_IO_L, REGH_ROUTING_IO_R,
	ROUTING_IO_L_BRK, ROUTING_GCLK,
	REGH_IO_L, REGH_IO_R, REGH_MCB, HCLK_MCB,
	ROUTING_IO_VIA_L, ROUTING_IO_VIA_R, ROUTING_IO_PCI_CE_L, ROUTING_IO_PCI_CE_R,
	CORNER_TL, CORNER_BL,
	CORNER_TR_UPPER, CORNER_TR_LOWER, CORNER_BR_UPPER, CORNER_BR_LOWER,
	HCLK_IO_TOP_UP_L, HCLK_IO_TOP_UP_R,
	HCLK_IO_TOP_SPLIT_L, HCLK_IO_TOP_SPLIT_R,
	HCLK_IO_TOP_DN_L, HCLK_IO_TOP_DN_R,
	HCLK_IO_BOT_UP_L, HCLK_IO_BOT_UP_R,
	HCLK_IO_BOT_SPLIT_L, HCLK_IO_BOT_SPLIT_R,
	HCLK_IO_BOT_DN_L, HCLK_IO_BOT_DN_R,
};

// Some macros to make the code more readable
#define LEFT_OUTER_COL		0
#define LEFT_INNER_COL		1
#define LEFT_IO_ROUTING		2
#define LEFT_IO_DEVS		3
#define LEFT_MCB_COL		4
#define LEFT_SIDE_WIDTH		5
#define RIGHT_SIDE_WIDTH	5
#define LEFT_LOCAL_HEIGHT	1
#define RIGHT_LOCAL_HEIGHT	2

#define TOP_IO_TILES		2
#define TOPBOT_IO_ROWS		2 // OUTER and INNER IO
// todo: maybe rename TOP_OUTER_ROW to TOP_OUTER_TERM and
//       TOP_INNER_ROW to TOP_INNER_TERM?
#define TOP_OUTER_ROW		0
#define TOP_INNER_ROW		1
#define TOP_FIRST_REGULAR	2
#define TOP_OUTER_IO		2
#define TOP_INNER_IO		3
#define HALF_ROW		8
#define HCLK_POS		8  // hclk pos in row
#define LAST_POS_IN_ROW		16 // including hclk at 8
#define LAST_REGULAR_ROW_POS	15 // without hclk
#define ROW_SIZE		(HALF_ROW+1+HALF_ROW)

#define CENTER_X_PLUS_1		1 // routing col adjacent to center
#define CENTER_X_PLUS_2		2 // logic col adjacent to center

#define CENTER_Y_PLUS_1		1
#define CENTER_Y_PLUS_2		2
#define CENTER_Y_MINUS_1	1
#define CENTER_Y_MINUS_2	2
#define CENTER_Y_MINUS_3	3
#define CENTER_Y_MINUS_4	4

#define CENTER_TOP_IOB_O	3 // deduct from center_y
#define CENTER_BOT_IOB_O	1 // add to center_y

// Some offsets that are being deducted from their origin
#define BOT_IO_TILES		2
// todo: rename BOT_OUTER_ROW to BOT_OUTER_TERM and BOT_INNER_ROW
//       to BOT_INNER_TERM?
#define BOT_OUTER_ROW		1
#define BOT_INNER_ROW		2
#define BOT_LAST_REGULAR_O	3
#define BOT_OUTER_IO		3
#define BOT_INNER_IO		4
#define RIGHT_OUTER_O		1
#define RIGHT_INNER_O		2
#define RIGHT_MCB_O		3
#define RIGHT_IO_DEVS_O		4
#define RIGHT_IO_ROUTING_O	5
#define CENTER_CMTPLL_O		1
#define CENTER_LOGIC_O		2
#define CENTER_ROUTING_O	3

#define YX_TILE(model, y, x) (&(model)->tiles[(y)*(model)->x_width+(x)])

// tile flags

#define TF_FABRIC_ROUTING_COL		0x00000001 // only set in y==0, not for left and right IO routing or center
#define TF_FABRIC_LOGIC_XM_COL		0x00000002 // only set in y==0
#define TF_FABRIC_LOGIC_XL_COL		0x00000004 // only set in y==0
#define TF_FABRIC_BRAM_VIA_COL		0x00000008 // only set in y==0
#define TF_FABRIC_MACC_VIA_COL		0x00000010 // only set in y==0
#define TF_FABRIC_BRAM_COL		0x00000020 // only set in y==0
#define TF_FABRIC_MACC_COL		0x00000040 // only set in y==0
// TF_ROUTING_NO_IO is only set in y==0 - automatically for BRAM and MACC
// routing, and manually for logic routing with the noio flag in the column
// configuration string
#define TF_ROUTING_NO_IO		0x00000080
#define TF_BRAM_DEV			0x00000100
#define TF_MACC_DEV			0x00000200
#define TF_LOGIC_XL_DEV			0x00000400
#define TF_LOGIC_XM_DEV			0x00000800
#define TF_IOLOGIC_DELAY_DEV		0x00001000
#define TF_DCM_DEV			0x00002000
#define TF_PLL_DEV			0x00004000
// TF_WIRED is only set for x==0 on the left side or x==x_width-1
// on the right side.
#define TF_WIRED			0x00008000
#define TF_CENTER_MIDBUF		0x00010000

#define Y_OUTER_TOP		0x0001
#define Y_INNER_TOP		0x0002
#define Y_INNER_BOTTOM		0x0004
#define Y_OUTER_BOTTOM		0x0008
#define Y_CHIP_HORIZ_REGS	0x0010
#define Y_ROW_HORIZ_AXSYMM	0x0020
#define Y_BOTTOM_OF_ROW		0x0040
#define Y_LEFT_WIRED		0x0080
#define Y_RIGHT_WIRED		0x0100
// Y_TOPBOT_IO_RANGE checks if y points to the top or bottom outer or
// inner rows. todo: same as TOP_OUTER|TOP_INNER|BOT_INNER|BOT_OUTER?
#define Y_TOPBOT_IO_RANGE	0x0200
#define Y_TOP_OUTER_IO		0x0400
#define Y_TOP_INNER_IO		0x0800
#define Y_BOT_INNER_IO		0x1000
#define Y_BOT_OUTER_IO		0x2000
#define Y_TOP_FIRST_REGULAR	Y_TOP_OUTER_IO
#define Y_BOT_LAST_REGULAR	Y_BOT_OUTER_IO
#define Y_REGULAR_ROW		0x4000

// multiple checks are combined with OR logic
int is_aty(int check, struct fpga_model* model, int y);

#define X_FABRIC_LOGIC_COL		(X_FABRIC_LOGIC_XM_COL \
					 |X_FABRIC_LOGIC_XL_COL)
#define X_FABRIC_LOGIC_ROUTING_COL	(X_FABRIC_LOGIC_XM_ROUTING_COL \
					 |X_FABRIC_LOGIC_XL_ROUTING_COL)
#define X_FABRIC_ROUTING_COL		(X_FABRIC_LOGIC_XM_ROUTING_COL \
					 |X_FABRIC_LOGIC_XL_ROUTING_COL \
					 |X_FABRIC_BRAM_ROUTING_COL \
					 |X_FABRIC_MACC_ROUTING_COL)
#define X_ROUTING_COL			(X_FABRIC_ROUTING_COL \
					 |X_CENTER_ROUTING_COL \
					 |X_LEFT_IO_ROUTING_COL \
					 |X_RIGHT_IO_ROUTING_COL)
#define X_CENTER_MAJOR			(X_CENTER_ROUTING_COL \
					 |X_CENTER_LOGIC_COL \
					 |X_CENTER_CMTPLL_COL \
					 |X_CENTER_REGS_COL)

// todo and realizations:
// * is_at() which combines X_ Y_ and YX_ into 64-bit and adds a AT_AND
//   to enable AND logic. then #define is_atx() and is_aty() onto is_at()
// * maybe the many special cases for bram are better
//   tied to no-io columns?
#define X_OUTER_LEFT			0x00000001
#define X_INNER_LEFT			0x00000002
#define X_INNER_RIGHT			0x00000004
#define X_OUTER_RIGHT			0x00000008
#define X_ROUTING_NO_IO			0x00000010
#define X_FABRIC_LOGIC_XM_ROUTING_COL	0x00000020 // logic-xm only
//X_FABRIC_LOGIC_XL_ROUTING_IO
//X_FABRIC_LOGIC_XL_ROUTING_NO_IO
//X_FABRIC_LOGIC_XL_IO
//X_FABRIC_LOGIC_XL_NO_IO
#define X_FABRIC_LOGIC_XL_ROUTING_COL	0x00000040 // logic-xl only
#define X_FABRIC_LOGIC_XM_COL		0x00000080
#define X_FABRIC_LOGIC_XL_COL		0x00000100
#define X_FABRIC_BRAM_ROUTING_COL	0x00000200 // BRAM only
#define X_FABRIC_MACC_ROUTING_COL	0x00000400 // MACC only
#define X_FABRIC_BRAM_VIA_COL		0x00000800 // second routing col for BRAM
#define X_FABRIC_MACC_VIA_COL		0x00001000 // second routing col for MACC
#define X_FABRIC_BRAM_COL		0x00002000
#define X_FABRIC_MACC_COL		0x00004000
#define X_CENTER_ROUTING_COL		0x00008000
#define X_CENTER_LOGIC_COL		0x00010000
#define X_CENTER_CMTPLL_COL		0x00020000
#define X_CENTER_REGS_COL		0x00040000
#define X_LEFT_IO_ROUTING_COL		0x00080000
#define X_LEFT_IO_DEVS_COL		0x00100000
#define X_RIGHT_IO_ROUTING_COL		0x00200000
#define X_RIGHT_IO_DEVS_COL		0x00400000
#define X_LEFT_SIDE			0x00800000 // true for anything left of the center (not including center)
#define X_LEFT_MCB			0x01000000
#define X_RIGHT_MCB			0x02000000

#define IS_TOP_ROW(row, model)		((row) == (model)->cfg_rows-1)
#define IS_BOTTOM_ROW(row, model)	((row) == 0)
#define IS_CENTER_Y(row, model)		((row) == (model)->center_y)
#define BOT_TERM(model)			((model)->y_height-BOT_INNER_ROW)
#define TOP_TERM(model)			(TOP_INNER_ROW)

// multiple checks are combined with OR logic
int is_atx(int check, struct fpga_model* model, int x);

// True for all tiles that are in the regular 0..15 row tiles of a routing col
#define YX_ROUTING_TILE		0x0001
#define YX_IO_ROUTING		0x0002
#define YX_ROUTING_TO_FABLOGIC	0x0004 // left of a regular fabric logic device
#define YX_DEV_ILOGIC		0x0008
#define YX_DEV_OLOGIC		0x0010
#define YX_DEV_LOGIC		0x0020
#define YX_DEV_IOB		0x0040
#define YX_CENTER_MIDBUF	0x0080
#define YX_OUTER_TERM		0x0100
#define YX_INNER_TERM		0x0200
// outside_of_routing is true for anything outside of the outer
// boundary of the regular routing area.
#define YX_OUTSIDE_OF_ROUTING	0x0400
#define YX_X_CENTER_CMTPLL	0x0800
#define YX_Y_CENTER		0x1000
#define YX_CENTER		0x2000

int is_atyx(int check, struct fpga_model* model, int y, int x);

// if not in row, both return values (if given) will
// be set to -1. the row_pos is 0..7 for the upper half,
// 8 for the hclk, and 9..16 for the lower half.
void is_in_row(const struct fpga_model* model, int y,
	int* row_num, int* row_pos);

// which_row() and pos_in_row() return -1 if y is outside of a row
int which_row(int y, struct fpga_model *model);
int pos_in_row(int y, struct fpga_model *model);
// regular_row_pos() returns the index (0..15) without hclk, or -1 if y is a hclk.
int regular_row_pos(int y, struct fpga_model *model);
int row_to_hclk(int row, struct fpga_model *model);
int y_to_hclk(int y, struct fpga_model *model);
// returns -1 if we are at TOP_FIRST_REGULAR
int regular_row_up(int y, struct fpga_model *model);

const char* logicin_s(int wire, int routing_io);

enum fpgadev_type
	{ DEV_NONE = 0,
	DEV_LOGIC, DEV_TIEOFF, DEV_MACC, DEV_IOB,
	DEV_ILOGIC, DEV_OLOGIC, DEV_IODELAY, DEV_BRAM16, DEV_BRAM8,
	DEV_BUFH, DEV_BUFIO, DEV_BUFIO_FB, DEV_BUFPLL, DEV_BUFPLL_MCB,
	DEV_BUFGMUX, DEV_BSCAN, DEV_DCM, DEV_PLL, DEV_ICAP,
	DEV_POST_CRC_INTERNAL, DEV_STARTUP, DEV_SLAVE_SPI,
	DEV_SUSPEND_SYNC, DEV_OCT_CALIBRATE, DEV_SPI_ACCESS,
	DEV_DNA, DEV_PMV, DEV_PCILOGIC_SE, DEV_MCB };
#define FPGA_DEV_STR \
	{ 0, \
	  "LOGIC", "TIEOFF", "MACC", "IOB", \
	  "ILOGIC", "OLOGIC", "IODELAY", "BRAM16", "BRAM8", \
	  "BUFH", "BUFIO", "BUFIO_FB", "BUFPLL", "BUFPLL_MCB", \
	  "BUFGMUX", "BSCAN", "DCM", "PLL", "ICAP", \
	  "POST_CRC_INTERNAL", "STARTUP", "SLAVE_SPI", \
	  "SUSPEND_SYNC", "OCT_CALIBRATE", "SPI_ACCESS", \
	  "DNA", "PMV", "PCILOGIC_SE", "MCB" }

// We use two types of device indices, one is a flat index
// into the tile->devs array (dev_idx_t), the other
// one counts up from 0 through devices of one particular
// type in the tile. The first logic device has type_idx == 0,
// the second logic device has type_idx == 1, etc, no matter
// at which index they are in the device array. The type indices
// are used in the floorplan, the flat array indices internally
// in memory.

typedef int dev_idx_t;
typedef int dev_type_idx_t;

#define NO_DEV -1
#define FPGA_DEV(model, y, x, dev_idx) \
		(((dev_idx) == NO_DEV) ? 0 : (&YX_TILE(model, y, x)->devs[dev_idx]))

//
// DEV_LOGIC
//

// M and L device is always at type index 0, X device
// is always at type index 1.
#define DEV_LOG_M_OR_L 0
#define DEV_LOG_X 1

// All device configuration is structured so that the value
// 0 is never a valid configured setting. That way all config
// data can safely be initialized to 0 meaning unconfigured.

enum { LOGIC_M = 1, LOGIC_L, LOGIC_X };

// LD1 stands for logic device 1 and can be OR'ed to the LI_A1
// or LO_A values to indicate the second logic device in a tile,
// either an M or L device.
#define LD1	0x100

// All LOGICIN_IN A..D sequences must be exactly sequential as
// here to match initialization in model_devices.c:init_logic()
// and control.c:fdev_set_required_pins().
enum {
	// input:
	LI_FIRST = 0,

	LI_A1 = LI_FIRST, LI_A2, LI_A3, LI_A4, LI_A5, LI_A6,
	LI_B1, LI_B2, LI_B3, LI_B4, LI_B5, LI_B6,
	LI_C1, LI_C2, LI_C3, LI_C4, LI_C5, LI_C6,
	LI_D1, LI_D2, LI_D3, LI_D4, LI_D5, LI_D6,
	LI_AX, LI_BX, LI_CX, LI_DX,
	LI_CLK, LI_CE, LI_SR,
	// only for L and M:
	LI_CIN,
	// only for M:
	LI_WE, LI_AI, LI_BI, LI_CI, LI_DI,

	LI_LAST = LI_DI,

	// output:
	LO_FIRST,

	LO_A = LO_FIRST, LO_B, LO_C, LO_D,
	LO_AMUX, LO_BMUX, LO_CMUX, LO_DMUX,
	LO_AQ, LO_BQ, LO_CQ, LO_DQ,
	LO_COUT, // only some L and M devs have this

	LO_LAST = LO_COUT };
#define LOGIC_PINW_STR \
	{ "A1", "A2", "A3", "A4", "A5", "A6", \
	  "B1", "B2", "B3", "B4", "B5", "B6", \
	  "C1", "C2", "C3", "C4", "C5", "C6", \
	  "D1", "D2", "D3", "D4", "D5", "D6", \
	  "AX", "BX", "CX", "DX", \
	  "CLK", "CE", "SR", \
	  "CIN", \
	  "WE", "AI", "BI", "CI", "DI", \
	  "A", "B", "C", "D", \
	  "AMUX", "BMUX", "CMUX", "DMUX", \
	  "AQ", "BQ", "CQ", "DQ", \
	  "COUT" }

enum { LUT_A = 0, LUT_B, LUT_C, LUT_D }; // offset into a2d[]
#define OUT_USED	0x0001
#define LUT6VAL_SET	0x0002
#define LUT5VAL_SET	0x0004
#define LUTMODE_ROM	0x0008
enum { FF_SRINIT0 = 1, FF_SRINIT1 };
enum { MUX_O6 = 1, MUX_O5, MUX_5Q, MUX_X, MUX_CY, MUX_XOR, MUX_F7, MUX_F8, MUX_MC31 };
enum { FF_OR2L = 1, FF_AND2L, FF_LATCH, FF_FF };
enum { CY0_X = 1, CY0_O5 };
enum { CLKINV_B = 1, CLKINV_CLK };
enum { SYNCATTR_SYNC = 1, SYNCATTR_ASYNC };
enum { WEMUX_WE = 1, WEMUX_CE };
enum { PRECYINIT_0 = 1, PRECYINIT_1, PRECYINIT_AX };
enum { DPRAM64 = 1, DPRAM32, SPRAM64, SPRAM32, SRL32, SRL16 };
enum { DIMUX_MC31 = 1, DIMUX_X, DIMUX_DX, DIMUX_BDI1 };

#define MAX_LUT_LEN 2048
#define NUM_LUTS 4

struct fpgadev_logic_a2d
{
	// LUT/RAM/ROM mode:
	// - Both the lut6 and lut5 must have the same mode of either
	//   LUT, RAM or ROM.
	// - If neither LUT6VAL_SET nor LUT5VAL_SET are on, the mode is LUT.
	// - If either LUT6VAL_SET or LUT5VAL_SET are on, lut6_str and
	//   lut5_str are ignored, and the mode is RAM, or ROM if
	//   LUTMODE_ROM is on.

	int flags;	// OUT_USED, LUT6VAL_SET, LUT5VAL_SET, LUTMODE_ROM
	char* lut6_str;
	char* lut5_str;
	int ff_mux;	// O6, O5, X, F7(a/c), F8(b), MC31(d), CY, XOR
	int ff_srinit;	// SRINIT0, SRINIT1 
	int ff5_srinit; // SRINIT0, SRINIT1
	int out_mux;	// O6, O5, 5Q, F7(a/c), F8(b), MC31(d), CY, XOR
	int ff;		// OR2L, AND2L, LATCH, FF
	int cy0;	// X, O5

	// distributed memory related - if either LUT6VAL_SET or LUT5VAL_SET are on:
	uint64_t lut6_val;
	uint32_t lut5_val;
	// ram_mode is only valid if LUTMODE_ROM is not set. If both
	// lut6_val and lut5_val are used, they must use the same ram_mode.
	int ram_mode; // DPRAM64, DPRAM32, SPRAM64, SPRAM32, SRL32, SRL16
	int di_mux; // only for A-C
		// DIMUX_MC31, DIMUX_X, DIMUX_DX (b/c), DIMUX_BDI1 (a)
};

struct fpgadev_logic
{
	struct fpgadev_logic_a2d a2d[NUM_LUTS];
	int clk_inv;	// CLKINV_B, CLKINV_CLK
	int sync_attr;	// SYNCATTR_SYNC, SYNCATTR_ASYNC
	int ce_used;
	int sr_used;
	int we_mux;	// WEMUX_WE, WEMUX_CE
	int cout_used;
	int precyinit;	// PRECYINIT_0, PRECYINIT_1, PRECYINIT_AX
};

//
// DEV_IOB
//

enum { IOBM = 1, IOBS };
typedef char IOSTANDARD[32];
#define IO_LVTTL		"LVTTL"
#define IO_LVCMOS33		"LVCMOS33"
#define IO_LVCMOS25		"LVCMOS25"
#define IO_LVCMOS18		"LVCMOS18"
#define IO_LVCMOS18_JEDEC	"LVCMOS18_JEDEC"
#define IO_LVCMOS15		"LVCMOS15"
#define IO_LVCMOS15_JEDEC	"LVCMOS15_JEDEC"
#define IO_LVCMOS12		"LVCMOS12"
#define IO_LVCMOS12_JEDEC	"LVCMOS12_JEDEC"
#define IO_SSTL2_I		"SSTL2_I" // TODO: sstl not fully supported
enum { BYPASS_MUX_I = 1, BYPASS_MUX_O, BYPASS_MUX_T };
enum { IMUX_I_B = 1, IMUX_I };
enum { SLEW_SLOW = 1, SLEW_FAST, SLEW_QUIETIO };
enum { SUSP_LAST_VAL = 1, SUSP_3STATE, SUSP_3STATE_PULLUP,
	SUSP_3STATE_PULLDOWN, SUSP_3STATE_KEEPER, SUSP_3STATE_OCT_ON };
enum { ITERM_NONE = 1, ITERM_UNTUNED_25, ITERM_UNTUNED_50,
	ITERM_UNTUNED_75 };
enum { OTERM_NONE = 1, OTERM_UNTUNED_25, OTERM_UNTUNED_50,
	OTERM_UNTUNED_75 };

enum { // input:
	IOB_IN_O = 0, IOB_IN_T, IOB_IN_DIFFI_IN, IOB_IN_DIFFO_IN,
	// output:
	IOB_OUT_I, IOB_OUT_PADOUT, IOB_OUT_PCI_RDY, IOB_OUT_DIFFO_OUT };
#define IOB_LAST_INPUT_PINW	IOB_IN_DIFFO_IN
#define IOB_LAST_OUTPUT_PINW	IOB_OUT_DIFFO_OUT
#define IOB_PINW_STR \
	{ "O", "T", "DIFFI_IN", "DIFFO_IN", \
	  "I", "PADOUT", "PCI_RDY", "DIFFO_OUT" }

struct fpgadev_iob
{
	IOSTANDARD istandard;
	IOSTANDARD ostandard;
	int bypass_mux;
	int I_mux;
	int drive_strength; // supports 2,4,6,8,12,16 and 24
	int slew;
	int O_used;
	int suspend;
	int in_term;
	int out_term;
};

//
// DEV_BUFGMUX
//

enum { BUFG_CLK_ASYNC = 1, BUFG_CLK_SYNC };
enum { BUFG_DISATTR_LOW = 1, BUFG_DISATTR_HIGH };
enum { BUFG_SINV_N = 1, BUFG_SINV_Y };

struct fpgadev_bufgmux
{
	int clk;
	int disable_attr;
	int s_inv;
};

//
// DEV_BUFIO
//

enum { BUFIO_DIVIDEBP_N = 1, BUFIO_DIVIDEBP_Y };
enum { BUFIO_IINV_N = 1, BUFIO_IINV_Y };

struct fpgadev_bufio
{
	int divide;
	int divide_bypass;
	int i_inv;
};

//
// DEV_BSCAN
//

enum { BSCAN_JTAG_TEST_N = 1, BSCAN_JTAG_TEST_Y };

struct fpgadev_bscan
{
	int jtag_chain; // 1-4
	int jtag_test;
};

//
// DEV_BRAM
//

// B8_0 or B8_1 can be or'ed into the BI/BO values
// to designate the first and second BRAM8 device,
// instead of the default BRAM16 device.
#define B8_0 0x100
#define B8_1 0x200
#define BW_FLAGS (B8_0|B8_1)

enum {
	// input:
	BI_FIRST = 0,

	// port A
	BI_ADDRA0 = BI_FIRST, BI_ADDRA1, BI_ADDRA2, BI_ADDRA3, BI_ADDRA4, BI_ADDRA5,
	BI_ADDRA6, BI_ADDRA7, BI_ADDRA8, BI_ADDRA9, BI_ADDRA10, BI_ADDRA11,
	BI_ADDRA12, BI_ADDRA13,

	BI_DIA0, BI_DIA1, BI_DIA2, BI_DIA3, BI_DIA4, BI_DIA5,
	BI_DIA6, BI_DIA7, BI_DIA8, BI_DIA9, BI_DIA10, BI_DIA11,
	BI_DIA12, BI_DIA13, BI_DIA14, BI_DIA15, BI_DIA16, BI_DIA17,
	BI_DIA18, BI_DIA19, BI_DIA20, BI_DIA21, BI_DIA22, BI_DIA23,
	BI_DIA24, BI_DIA25, BI_DIA26, BI_DIA27, BI_DIA28, BI_DIA29,
	BI_DIA30, BI_DIA31,

	BI_DIPA0, BI_DIPA1, BI_DIPA2, BI_DIPA3,
	BI_WEA0, BI_WEA1, BI_WEA2, BI_WEA3,
	BI_REGCEA,
	BI_ENA,

	// port B
	BI_ADDRB0, BI_ADDRB1, BI_ADDRB2, BI_ADDRB3, BI_ADDRB4, BI_ADDRB5,
	BI_ADDRB6, BI_ADDRB7, BI_ADDRB8, BI_ADDRB9, BI_ADDRB10, BI_ADDRB11,
	BI_ADDRB12, BI_ADDRB13,

	BI_DIB0, BI_DIB1, BI_DIB2, BI_DIB3, BI_DIB4, BI_DIB5,
	BI_DIB6, BI_DIB7, BI_DIB8, BI_DIB9, BI_DIB10, BI_DIB11,
	BI_DIB12, BI_DIB13, BI_DIB14, BI_DIB15, BI_DIB16, BI_DIB17,
	BI_DIB18, BI_DIB19, BI_DIB20, BI_DIB21, BI_DIB22, BI_DIB23,
	BI_DIB24, BI_DIB25, BI_DIB26, BI_DIB27, BI_DIB28, BI_DIB29,
	BI_DIB30, BI_DIB31,

	BI_DIPB0, BI_DIPB1, BI_DIPB2, BI_DIPB3,
	BI_WEB0, BI_WEB1, BI_WEB2, BI_WEB3,
	BI_REGCEB,
	BI_ENB,

	BI_LAST = BI_ENB,

	// output:
	BO_FIRST,

	// port A
	BO_DOA0 = BO_FIRST, BO_DOA1, BO_DOA2, BO_DOA3, BO_DOA4, BO_DOA5,
	BO_DOA6, BO_DOA7, BO_DOA8, BO_DOA9, BO_DOA10, BO_DOA11,
	BO_DOA12, BO_DOA13, BO_DOA14, BO_DOA15, BO_DOA16, BO_DOA17,
	BO_DOA18, BO_DOA19, BO_DOA20, BO_DOA21, BO_DOA22, BO_DOA23,
	BO_DOA24, BO_DOA25, BO_DOA26, BO_DOA27, BO_DOA28, BO_DOA29,
	BO_DOA30, BO_DOA31,

	BO_DOPA0, BO_DOPA1, BO_DOPA2, BO_DOPA3,

	// port B
	BO_DOB0, BO_DOB1, BO_DOB2, BO_DOB3, BO_DOB4, BO_DOB5,
	BO_DOB6, BO_DOB7, BO_DOB8, BO_DOB9, BO_DOB10, BO_DOB11,
	BO_DOB12, BO_DOB13, BO_DOB14, BO_DOB15, BO_DOB16, BO_DOB17,
	BO_DOB18, BO_DOB19, BO_DOB20, BO_DOB21, BO_DOB22, BO_DOB23,
	BO_DOB24, BO_DOB25, BO_DOB26, BO_DOB27, BO_DOB28, BO_DOB29,
	BO_DOB30, BO_DOB31,

	BO_DOPB0, BO_DOPB1, BO_DOPB2, BO_DOPB3,

	BO_LAST = BO_DOPB3
};

//
// DEV_MACC
//

enum {
	// input:
	MI_FIRST = 0,

	MI_CEA = MI_FIRST, MI_CEB, MI_CEC, MI_CED, MI_CEM, MI_CEP,
	MI_CE_OPMODE, MI_CE_CARRYIN,
	MI_OPMODE0, MI_OPMODE1, MI_OPMODE2, MI_OPMODE3,
	MI_OPMODE4, MI_OPMODE5, MI_OPMODE6, MI_OPMODE7,
	MI_A0, MI_A1, MI_A2, MI_A3, MI_A4, MI_A5, MI_A6, MI_A7,
	MI_A8, MI_A9, MI_A10, MI_A11, MI_A12, MI_A13, MI_A14, MI_A15,
	MI_A16, MI_A17,
	MI_B0, MI_B1, MI_B2, MI_B3, MI_B4, MI_B5, MI_B6, MI_B7,
	MI_B8, MI_B9, MI_B10, MI_B11, MI_B12, MI_B13, MI_B14, MI_B15,
	MI_B16, MI_B17,
	MI_C0, MI_C1, MI_C2, MI_C3, MI_C4, MI_C5, MI_C6, MI_C7,
	MI_C8, MI_C9, MI_C10, MI_C11, MI_C12, MI_C13, MI_C14, MI_C15,
	MI_C16, MI_C17, MI_C18, MI_C19, MI_C20, MI_C21, MI_C22, MI_C23,
	MI_C24, MI_C25, MI_C26, MI_C27, MI_C28, MI_C29, MI_C30, MI_C31,
	MI_C32, MI_C33, MI_C34, MI_C35, MI_C36, MI_C37, MI_C38, MI_C39,
	MI_C40, MI_C41, MI_C42, MI_C43, MI_C44, MI_C45, MI_C46, MI_C47,
	MI_D0, MI_D1, MI_D2, MI_D3, MI_D4, MI_D5, MI_D6, MI_D7,
	MI_D8, MI_D9, MI_D10, MI_D11, MI_D12, MI_D13, MI_D14, MI_D15,
	MI_D16, MI_D17,

	MI_LAST = MI_D17,

	// output:
	MO_FIRST,

	MO_CARRYOUT = MO_FIRST,
	MO_P0, MO_P1, MO_P2, MO_P3, MO_P4, MO_P5, MO_P6, MO_P7,
	MO_P8, MO_P9, MO_P10, MO_P11, MO_P12, MO_P13, MO_P14, MO_P15,
	MO_P16, MO_P17, MO_P18, MO_P19, MO_P20, MO_P21, MO_P22, MO_P23,
	MO_P24, MO_P25, MO_P26, MO_P27, MO_P28, MO_P29, MO_P30, MO_P31,
	MO_P32, MO_P33, MO_P34, MO_P35, MO_P36, MO_P37, MO_P38, MO_P39,
	MO_P40, MO_P41, MO_P42, MO_P43, MO_P44, MO_P45, MO_P46, MO_P47,

	MO_M0, MO_M1, MO_M2, MO_M3, MO_M4, MO_M5, MO_M6, MO_M7,
	MO_M8, MO_M9, MO_M10, MO_M11, MO_M12, MO_M13, MO_M14, MO_M15,
	MO_M16, MO_M17, MO_M18, MO_M19, MO_M20, MO_M21, MO_M22, MO_M23,
	MO_M24, MO_M25, MO_M26, MO_M27, MO_M28, MO_M29, MO_M30, MO_M31,
	MO_M32, MO_M33, MO_M34, MO_M35,

	MO_LAST = MO_M35
};

//
// fpga_device
//

typedef int pinw_idx_t; // index into pinw array

// A bram dev has about 190 pinwires (input and output
// combined), macc about 350, mcb about 1200.
#define MAX_NUM_PINW	2048

struct fpga_device
{
	enum fpgadev_type type;
	// subtypes:
	// IOB:   IOBM, IOBS
	// LOGIC: LOGIC_M, LOGIC_L, LOGIC_X
	int subtype;
	int instantiated;

	int num_pinw_total, num_pinw_in;
	// The array holds first the input wires, then the output wires.
	// Unused members are set to STRIDX_NO_ENTRY.
	str16_t* pinw;

	// required pinwires depend on the given config and will
	// be deleted/invalidated on any config change.
	int pinw_req_total, pinw_req_in;
	pinw_idx_t* pinw_req_for_cfg;
	
	// the rest will be memset to 0 on any device removal/uninstantiation
	union {
		struct fpgadev_logic logic;
		struct fpgadev_iob iob;
		struct fpgadev_bufgmux bufgmux;
		struct fpgadev_bufio bufio;
		struct fpgadev_bscan bscan;
	} u;
};

#define SWITCH_USED		0x80000000
#define SWITCH_BIDIRECTIONAL	0x40000000
#define SWITCH_MAX_CONNPT_O	0x7FFF // 15 bits
#define SW_FROM_I(u32)		(((u32) >> 15) & SWITCH_MAX_CONNPT_O)
#define SW_TO_I(u32)		((u32) & SWITCH_MAX_CONNPT_O)

#define SW_I(u32, from_to)	((from_to) ? SW_FROM_I(u32) : SW_TO_I(u32))
// SW_FROM and SW_TO values are chosen such that ! inverts them,
// and swf() assumes that SW_FROM is positive.
#define SW_FROM			1
#define SW_TO			0

#define NO_SWITCH	-1
// FIRST_SW must be high enough to be above switch indices or
// connpt or str16.
#define FIRST_SW	0x80000
#define NO_CONN		-1

typedef int connpt_t; // index into conn_point_names (not yet *2)
#define CONNPT_STR16(tile, connpt)	((tile)->conn_point_names[(connpt)*2+1])

struct fpga_tile
{
	enum fpga_tile_type type;
	int flags;

	// expect up to 64 devices per tile
	int num_devs;
	struct fpga_device* devs;

	// expect up to 5k connection point names per tile
	// 2*16 bit per entry
	//   - index into conn_point_dests (not multiplied by 3) (16bit)
	//   - hashed string array index (16 bit)
	// each conn point name exists only once in the array
	int num_conn_point_names; // conn_point_names is 2*num_conn_point_names 16-bit words
	uint16_t* conn_point_names; // num_conn_point_names*2 16-bit-words: 16(conn)-16(str)

	// expect up to 28k connection point destinations to other tiles per tile
	// 3*16 bit per destination:
	//   - x coordinate of other tile (16bit)
	//   - y coordinate of other tile (16bit)
	//   - hashed string array index for conn_point_names name in other tile (16bit)
	int num_conn_point_dests; // conn_point_dests array is 3*num_conn_point_dests 16-bit words
	uint16_t* conn_point_dests; // num_conn_point_dests*3 16-bit words: 16(x)-16(y)-16(conn_name)

	// expect up to 4k switches per tile
	// 32bit: 31    off: no connection   on: connected
	//        30    off: unidirectional  on: bidirectional
	//        29:15 from, index into conn_point_names (not yet *2)
	//        14:0  to, index into conn_point_names (not yet *2)
	int num_switches;
	uint32_t* switches;
};

int fpga_build_model(struct fpga_model* model, int idcode, enum xc6_pkg pkg);
// returns model->rc (model itself will be memset to 0)
int fpga_free_model(struct fpga_model* model);

const char* fpga_tiletype_str(enum fpga_tile_type type);

int init_tiles(struct fpga_model* model);

int init_devices(struct fpga_model* model);
void free_devices(struct fpga_model* model);

int init_ports(struct fpga_model* model, int dup_warn);
int init_conns(struct fpga_model* model);

int init_switches(struct fpga_model* model, int routing_sw);
// replicate_routing_switches() is a high-speed optimized way to
// initialize the routing switches, will only work before ports,
// connections or other switches.
int replicate_routing_switches(struct fpga_model* model);

const char* pf(const char* fmt, ...);
const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name);
char next_non_whitespace(const char* s);
char last_major(const char* str, int cur_o);
int has_connpt(struct fpga_model* model, int y, int x, const char* name);
// add_connpt_name(): name_i and conn_point_o can be 0
int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o);

// has_device() and has_device_type() return the number of devices
// for the given type or type/subtype.
int has_device(struct fpga_model* model, int y, int x, int dev);
int has_device_type(struct fpga_model* model, int y, int x, int dev, int subtype);

int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2,
	int dup_warn);

typedef int (*add_conn_f)(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
#define NOPREF_BI_F	add_conn_bi
#define PREF_BI_F	add_conn_bi_pref

int add_conn_bi(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_bi_pref(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func,
	int y1, int x1, const char* name1, int start1, int last1,
	int y2, int x2, const char* name2, int start2);

//
// switches
//

int add_switch(struct fpga_model* model, int y, int x, const char* from,
	const char* to, int is_bidirectional);
int add_switch_set(struct fpga_model* model, int y, int x, const char* prefix,
	const char** pairs, int suffix_inc);

// This will replicate the entire conn_point_names and switches arrays
// from one tile to another, assuming that all of conn_point_names,
// switches and conn_point_dests in the destination tile are empty.
int replicate_switches_and_names(struct fpga_model* model,
	int y_from, int x_from, int y_to, int x_to);

struct seed_data
{
	int flags;
	const char* str;
};

void seed_strx(struct fpga_model *model, const struct seed_data *data);
void seed_stry(struct fpga_model *model, const struct seed_data *data);

#define MAX_WIRENAME_LEN 64

// The LWF flags are OR'ed into the logic_wire enum
#define LWF_SOUTH0		0x0100
#define LWF_NORTH3		0x0200
#define LWF_BIDIR		0x0400
#define LWF_FAN_B		0x0800
#define LWF_WIRE_MASK		0x00FF // namespace for the enums

// ordered to match the LOGICIN_B?? enumeration
// todo: both enums logicin_wire and logicout_wire are not really
//       ideal for supporting L_ and XX_ variants, maybe use pinwires
//       and LD1 instead?
enum logicin_wire {
    /*  0 */	X_A1 = 0,
		      X_A2, X_A3, X_A4, X_A5, X_A6, X_AX,
    /*  7 */	X_B1, X_B2, X_B3, X_B4, X_B5, X_B6, X_BX,
    /* 14 */	X_C1, X_C2, X_C3, X_C4, X_C5, X_C6, X_CE, X_CX,
    /* 22 */	X_D1, X_D2, X_D3, X_D4, X_D5, X_D6, X_DX,
    /* 29 */	M_A1, M_A2, M_A3, M_A4, M_A5, M_A6, M_AX, M_AI,
    /* 37 */	M_B1, M_B2, M_B3, M_B4, M_B5, M_B6, M_BX, M_BI,
    /* 45 */	M_C1, M_C2, M_C3, M_C4, M_C5, M_C6, M_CE, M_CX, M_CI,
    /* 54 */	M_D1, M_D2, M_D3, M_D4, M_D5, M_D6, M_DX, M_DI,
    /* 62 */	M_WE
};

// ordered to match the LOGICOUT_B?? enumeration
enum logicout_wire {
    /*  0 */	X_A = 0,
		     X_AMUX, X_AQ, X_B, X_BMUX, X_BQ,
    /*  6 */	X_C, X_CMUX, X_CQ, X_D, X_DMUX, X_DQ,
    /* 12 */	M_A, M_AMUX, M_AQ, M_B, M_BMUX, M_BQ,
    /* 18 */	M_C, M_CMUX, M_CQ, M_D, M_DMUX, M_DQ
};

const char* logicin_str(enum logicin_wire w);
const char* logicout_str(enum logicout_wire w);

// The wires are ordered clockwise. Order is important for
// wire_to_NESW4().
enum wire_type
{
	FIRST_LEN1 = 1,
	W_NL1 = FIRST_LEN1,
	W_NR1,
	W_EL1,
	W_ER1,
	W_SL1,
	W_SR1,
	W_WL1,
	W_WR1,
	LAST_LEN1 = W_WR1,

	FIRST_LEN2,
	W_NN2 = FIRST_LEN2,
	W_NE2,
	W_EE2,
	W_SE2,
	W_SS2,
	W_SW2,
	W_WW2,
	W_NW2,
	LAST_LEN2 = W_NW2,

	FIRST_LEN4,
	W_NN4 = FIRST_LEN4,
	W_NE4,
	W_EE4,
	W_SE4,
	W_SS4,
	W_SW4,
	W_WW4,
	W_NW4,
	LAST_LEN4 = W_NW4
};

#define W_CLOCKWISE(w)			rotate_wire((w), 1)
#define W_CLOCKWISE_2(w)		rotate_wire((w), 2)
#define W_COUNTER_CLOCKWISE(w)		rotate_wire((w), -1)
#define W_COUNTER_CLOCKWISE_2(w)	rotate_wire((w), -2)

#define W_IS_LEN1(w)			((w) >= FIRST_LEN1 && (w) <= LAST_LEN1)
#define W_IS_LEN2(w)			((w) >= FIRST_LEN2 && (w) <= LAST_LEN2)
#define W_IS_LEN4(w)			((w) >= FIRST_LEN4 && (w) <= LAST_LEN4)

#define W_TO_LEN1(w)			wire_to_len(w, FIRST_LEN1)
#define W_TO_LEN2(w)			wire_to_len(w, FIRST_LEN2)
#define W_TO_LEN4(w)			wire_to_len(w, FIRST_LEN4)

const char* wire_base(enum wire_type w);
enum wire_type base2wire(const char* str);
enum wire_type rotate_wire(enum wire_type cur, int off);
enum wire_type wire_to_len(enum wire_type w, int first_len);

// These three flags can be OR'ed into the DW..DW_LAST range.
// DIR_BEG signals a 'B' line - the default is 'E' endpoint.
// DIR_S0 turns 0 into _S0
// DIR_N3 turns 3 into _N3.
// First flag must be higher than LAST_LEN4 (25) * 4 + 3 = 103
#define DIR_BEG 0x80
#define DIR_S0	0x100
#define DIR_N3	0x200
#define DIR_FLAGS (DIR_BEG|DIR_S0|DIR_N3)

// some more direction-related macros mostly to make code
// more readable and not directly related to enum extra_wires.
#define DIR_IN	0
#define DIR_OUT	1

#define DIR_POS		+1
#define DIR_NEG		-1
#define DIR_LEFT	DIR_NEG
#define DIR_RIGHT	DIR_POS
#define DIR_UP		DIR_NEG
#define DIR_DOWN	DIR_POS

enum { DIR_NORTH = 0, DIR_EAST, DIR_SOUTH, DIR_WEST };

#define LOGICOUT_HIGHEST	23
#define LOGICIN_HIGHEST		62

// The extra wires must not overlap with logicin_wire or logicout_wire
// namespaces so that they can be combined with either of them.
enum extra_wires {
	// NO_WIRE is not compatible with the old X_A1/M_A1 system, but
	// compatible with the new LW + LI_A1 system.
	NO_WIRE = 0,

	UNDEF = 100, // use UNDEF with old system, can be removed after
		     // old system is gone
	FAN_B,
	GFAN0,
	GFAN1,
	CLK0, // == clka for bram
	CLK1, // == clkb for bram
	SR0, // == rsta for bram
	SR1, // == rstb for bram
	LOGICIN20,
	LOGICIN21,
	LOGICIN44,
	LOGICIN52,
	LOGICIN_N21,
	LOGICIN_N28,
	LOGICIN_N52,
	LOGICIN_N60,
	LOGICIN_S20,
	LOGICIN_S36,
	LOGICIN_S44,
	LOGICIN_S62,
	IOCE,
	IOCLK,
	PLLCE,
	PLLCLK,
	CKPIN,
	CLK_FEEDBACK,
	CLK_INDIRECT,
	CFB0, CFB1, CFB2, CFB3, CFB4, CFB5, CFB6, CFB7,
		CFB8, CFB9, CFB10, CFB11, CFB12, CFB13, CFB14, CFB15,
	DFB0, DFB1, DFB2, DFB3, DFB4, DFB5, DFB6, DFB7,
	CLKPIN0, CLKPIN1, CLKPIN2, CLKPIN3, CLKPIN4, CLKPIN5, CLKPIN6, CLKPIN7,
	DQSN0, DQSN1, DQSN2, DQSN3,
	DQSP0, DQSP1, DQSP2, DQSP3,
	VCC_WIRE,
	GND_WIRE,
	GCLK0, GCLK1, GCLK2, GCLK3, GCLK4, GCLK5, GCLK6, GCLK7,
		GCLK8, GCLK9, GCLK10, GCLK11, GCLK12, GCLK13, GCLK14, GCLK15,
	LOGICOUT_B0, LOGICOUT_B1, LOGICOUT_B2, LOGICOUT_B3,
		LOGICOUT_B4, LOGICOUT_B5, LOGICOUT_B6, LOGICOUT_B7,
		LOGICOUT_B8, LOGICOUT_B9, LOGICOUT_B10, LOGICOUT_B11,
		LOGICOUT_B12, LOGICOUT_B13, LOGICOUT_B14, LOGICOUT_B15,
		LOGICOUT_B16, LOGICOUT_B17, LOGICOUT_B18, LOGICOUT_B19,
		LOGICOUT_B20, LOGICOUT_B21, LOGICOUT_B22, LOGICOUT_B23,
	LOGICIN_B0, LOGICIN_B1, LOGICIN_B2, LOGICIN_B3,
		LOGICIN_B4, LOGICIN_B5, LOGICIN_B6, LOGICIN_B7,
		LOGICIN_B8, LOGICIN_B9, LOGICIN_B10, LOGICIN_B11,
		LOGICIN_B12, LOGICIN_B13, LOGICIN_B14, LOGICIN_B15,
		LOGICIN_B16, LOGICIN_B17, LOGICIN_B18, LOGICIN_B19,
		LOGICIN_B20, LOGICIN_B21, LOGICIN_B22, LOGICIN_B23,
		LOGICIN_B24, LOGICIN_B25, LOGICIN_B26, LOGICIN_B27,
		LOGICIN_B28, LOGICIN_B29, LOGICIN_B30, LOGICIN_B31,
		LOGICIN_B32, LOGICIN_B33, LOGICIN_B34, LOGICIN_B35,
		LOGICIN_B36, LOGICIN_B37, LOGICIN_B38, LOGICIN_B39,
		LOGICIN_B40, LOGICIN_B41, LOGICIN_B42, LOGICIN_B43,
		LOGICIN_B44, LOGICIN_B45, LOGICIN_B46, LOGICIN_B47,
		LOGICIN_B48, LOGICIN_B49, LOGICIN_B50, LOGICIN_B51,
		LOGICIN_B52, LOGICIN_B53, LOGICIN_B54, LOGICIN_B55,
		LOGICIN_B56, LOGICIN_B57, LOGICIN_B58, LOGICIN_B59,
		LOGICIN_B60, LOGICIN_B61, LOGICIN_B62,

	// direction wires
	DW = 500,
	// dirwires can be encoded times-4, for example
	// NL1E2 = DW + W_NL1*4 + 2
	// DIR_BEG and DIR_S0N3 can be OR'ed into this range.
	DW_LAST = 1499,
	
	// logic wires
	LW,
	// logic wires are encoded here as LOGIC_BEG+LI_A1. LD1 (0x100)
	// can be OR'ed to the LI or LO value.
	LW_LAST = 1999,

	// bram wires are BW + (BI_/BO_, ORed with B8_0(0x100) or B8_1(0x200))
	BW,
	BW_LAST = 2999,

	// macc wires are MW + MI_/MO_
	MW,
	MW_LAST = 3499,
};

enum iologic_wire
{
	IOLOGIC_NO_WIRE = 0,
	IBUF0, IBUF1,
	D_ILOGIC_SITE, D_ILOGIC_SITE_S,
	D_ILOGIC_IDATAIN_IODELAY, D_ILOGIC_IDATAIN_IODELAY_S,
	DFB_ILOGIC_SITE, DFB_ILOGIC_SITE_S,
	FABRICOUT_ILOGIC_SITE, FABRICOUT_ILOGIC_SITE_S,
	O0, O1,
	OQ_OLOGIC_SITE, OQ_OLOGIC_SITE_S,
	D1_OLOGIC_SITE, D1_OLOGIC_SITE_S,
	IOI_LOGICOUT0, IOI_INTER_LOGICOUT0,
	IOI_LOGICOUT7, IOI_INTER_LOGICOUT7,
	IOI_LOGICIN_B31
};

const char *fpga_connpt_str(struct fpga_model *model, enum extra_wires wire,
	int y, int x, int dest_y, int dest_x);
str16_t fpga_wire2str_yx(struct fpga_model *model, enum extra_wires wire,
	int y, int x);
str16_t fpga_iologic_wire2str_yx(struct fpga_model *model,
	enum iologic_wire wire, int y, int x);
const char* fpga_wire2str(enum extra_wires wire);
str16_t fpga_wire2str_i(struct fpga_model* model, enum extra_wires wire);
enum extra_wires fpga_str2wire(const char* str);
int fdev_logic_inbit(pinw_idx_t idx);
int fdev_logic_outbit(pinw_idx_t idx);

// physically, tile3 is at the top, tile0 at the bottom
// errors are returned as -1 in tile0_to_3
void fdev_bram_inbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_62);
void fdev_bram_outbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_23);
int fdev_is_bram8_inwire(int bi_wire); // direct BI_ value
int fdev_is_bram8_outwire(int bo_wire); // direct BO_ value

void fdev_macc_inbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_62);
void fdev_macc_outbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_23);

//
// integer-based net (w_net_i)
//

struct w_yx { int y, x; };
#define MAX_NET_I_YX	128

struct w_net_i
{
	enum extra_wires wire;
	int wire_inc; // 0 = no-inc, 1 = wire+0 and wire+1, etc.
	int num_yx;
	struct w_yx yx[MAX_NET_I_YX];
};

int add_conn_net_i(struct fpga_model *model, const struct w_net_i *net);

//
// string-based net (w_net)
//

// COUNT_DOWN can be OR'ed to start_count to make
// the enumerated wires count from start_count down.
#define COUNT_DOWN		0x100
#define COUNT_MASK		0xFF

struct w_point // wire point
{
	const char* name;
	int start_count; // if there is a %i in the name, this is the start number
	int y, x;
};

#define NO_INCREMENT 0
#define MAX_NET_POINTS	128

struct w_net
{
	// if !last_inc, no incrementing will happen (NO_INCREMENT)
	// if last_inc > 0, incrementing will happen to
	// the %i in the name from pt.start_count:last_inc
	int last_inc;
	int num_pts;
	struct w_point pt[MAX_NET_POINTS];
};

#define NO_PREF 0
#define ADD_PREF 1

int add_conn_net(struct fpga_model* model, int add_pref, const struct w_net *net);
