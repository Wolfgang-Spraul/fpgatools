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
#include <sys/stat.h>

#include "helper.h"

//
// columns
//  'L' = X+L logic block
//  'M' = X+M logic block
//  'B' = block ram
//  'D' = dsp (macc)
//  'R' = registers and center IO/logic column
//
//  'n' = noio - can follow L or M to designate a logic
//        column without IO at top or bottom
//  'g' = gclk - can follow LlMmBD to designate exactly one
//        place on the left and right side of the chip where
//        the global clock is separated into left and right
//        half (on each side of the chip, for a total of 4
//        vertical clock separations).
//
// wiring on the left and right side is described with 16
// characters for each row, order is top-down
//   'W' = wired
//   'U' = unwired
//

#define XC6SLX9_ROWS	4
#define XC6SLX9_COLUMNS	"M L Bg M L D M R M Ln M L Bg M L"
#define XC6SLX9_LEFT_WIRING \
	/* row 3 */ "UWUWUWUW" "WWWWUUUU" \
	/* row 2 */ "UUUUUUUU" "WWWWWWUU" \
	/* row 1 */ "WWWUUWUU" "WUUWUUWU" \
	/* row 0 */ "UWUUWUUW" "UUWWWWUU"
#define XC6SLX9_RIGHT_WIRING \
	/* row 3 */ "UUWWUWUW" "WWWWUUUU" \
	/* row 2 */ "UUUUUUUU" "WWWWWWUU" \
	/* row 1 */ "WWWUUWUU" "WUUWUUWU" \
	/* row 0 */ "UWUUWUUW" "UUWWWWUU"

struct fpga_model
{
	int cfg_rows;
	char cfg_columns[512];
	char cfg_left_wiring[1024], cfg_right_wiring[1024];

	int x_width, y_height;
	int center_x;
	int center_y;
	// Left and right gclk separators will be located on
	// the device column (+1 or +2) of the logic or dsp/macc
	// column as indicated in the chip's cfg_columns with a 'g'.
	int left_gclk_sep_x, right_gclk_sep_x;

	struct fpga_tile* tiles;
	struct hashed_strarray str;

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
#define TOP_OUTER_IO		2
#define TOP_INNER_IO		3
#define HALF_ROW		8
#define HCLK_POS		8  // hclk pos in row
#define LAST_POS_IN_ROW		16 // including hclk at 8
#define ROW_SIZE		(HALF_ROW+1+HALF_ROW)

// Some offsets that are being deducted from their origin
#define BOT_IO_TILES		2
// todo: rename BOT_OUTER_ROW to BOT_OUTER_TERM and BOT_INNER_ROW
//       to BOT_INNER_TERM?
#define BOT_OUTER_ROW		1
#define BOT_INNER_ROW		2
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
#define TF_FABRIC_LOGIC_COL		0x00000002 // only set in y==0
#define TF_FABRIC_BRAM_VIA_COL		0x00000004 // only set in y==0
#define TF_FABRIC_MACC_VIA_COL		0x00000008 // only set in y==0
#define TF_FABRIC_BRAM_COL		0x00000010 // only set in y==0
#define TF_FABRIC_MACC_COL		0x00000020 // only set in y==0
// TF_ROUTING_NO_IO is only set in y==0 - automatically for BRAM and MACC
// routing, and manually for logic routing with the noio flag in the column
// configuration string
#define TF_ROUTING_NO_IO		0x00000040
#define TF_BRAM_DEV			0x00000080
#define TF_MACC_DEV			0x00000100
#define TF_LOGIC_XL_DEV			0x00000200
#define TF_LOGIC_XM_DEV			0x00000400
#define TF_IOLOGIC_DELAY_DEV		0x00000800
#define TF_DCM_DEV			0x00001000
#define TF_PLL_DEV			0x00002000
// TF_WIRED is only set for x==0 on the left side or x==x_width-1
// on the right side.
#define TF_WIRED			0x00004000

#define Y_INNER_TOP		0x0001
#define Y_INNER_BOTTOM		0x0002
#define Y_CHIP_HORIZ_REGS	0x0004
#define Y_ROW_HORIZ_AXSYMM	0x0008
#define Y_BOTTOM_OF_ROW		0x0010
#define Y_LEFT_WIRED		0x0020
#define Y_RIGHT_WIRED		0x0040
// Y_TOPBOT_IO_RANGE checks if y points to the top or bottom outer or
// inner rows.
#define Y_TOPBOT_IO_RANGE	0x0080

// multiple checks are combined with OR logic
int is_aty(int check, struct fpga_model* model, int y);

#define X_OUTER_LEFT			0x00000001
#define X_INNER_LEFT			0x00000002
#define X_INNER_RIGHT			0x00000004
#define X_OUTER_RIGHT			0x00000008
#define X_ROUTING_COL			0x00000010 // includes routing col in left and right IO and center
#define X_ROUTING_TO_BRAM_COL		0x00000020
#define X_ROUTING_TO_MACC_COL		0x00000040
#define X_ROUTING_NO_IO			0x00000080
#define X_ROUTING_HAS_IO		0x00000100
#define X_LOGIC_COL			0x00000200 // includes the center logic col
// todo: maybe X_FABRIC_ROUTING_COL could be logic+bram+macc?
#define X_FABRIC_ROUTING_COL		0x00000400 // logic+BRAM+MACC
#define X_FABRIC_LOGIC_ROUTING_COL	0x00000800 // logic only
#define X_FABRIC_LOGIC_COL		0x00001000
// X_FABRIC_LOGIC_IO_COL is like X_FABRIC_LOGIC_COL but
// excluding those that have the no-io flag set.
#define X_FABRIC_LOGIC_IO_COL		0x00002000
#define X_FABRIC_BRAM_ROUTING_COL	0x00004000 // BRAM only
#define X_FABRIC_MACC_ROUTING_COL	0x00008000 // MACC only
#define X_FABRIC_BRAM_VIA_COL		0x00010000 // second routing col for BRAM
#define X_FABRIC_MACC_VIA_COL		0x00020000 // second routing col for MACC
#define X_FABRIC_BRAM_COL		0x00040000
#define X_FABRIC_MACC_COL		0x00080000
#define X_CENTER_ROUTING_COL		0x00100000
#define X_CENTER_LOGIC_COL		0x00200000
#define X_CENTER_CMTPLL_COL		0x00400000
#define X_CENTER_REGS_COL		0x00800000
#define X_LEFT_IO_ROUTING_COL		0x01000000
#define X_LEFT_IO_DEVS_COL		0x02000000
#define X_RIGHT_IO_ROUTING_COL		0x04000000
#define X_RIGHT_IO_DEVS_COL		0x08000000
#define X_LEFT_SIDE			0x10000000 // true for anything left of the center (not including center)
#define X_LEFT_MCB			0x20000000
#define X_RIGHT_MCB			0x40000000

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

int is_atyx(int check, struct fpga_model* model, int y, int x);

// if not in row, both return values (if given) will
// be set to -1. the row_pos is 0..7 for the upper half,
// 8 for the hclk, and 9..16 for the lower half.
void is_in_row(const struct fpga_model* model, int y,
	int* row_num, int* row_pos);

// which_row() and pos_in_row() return -1 if y is outside of a row
int which_row(int y, struct fpga_model* model);
int pos_in_row(int y, struct fpga_model* model);

const char* logicin_s(int wire, int routing_io);

enum fpgadev_type
{
	DEV_NONE = 0,

	DEV_LOGIC,
	DEV_TIEOFF,
	DEV_MACC,
	DEV_IOB,
	DEV_ILOGIC,
	DEV_OLOGIC,
	DEV_IODELAY,
	DEV_BRAM16,
	DEV_BRAM8,
	DEV_BUFH,
	DEV_BUFIO,
	DEV_BUFIO_FB,
	DEV_BUFPLL,
	DEV_BUFPLL_MCB,
	DEV_BUFGMUX,
	DEV_BSCAN,
	DEV_DCM,
	DEV_PLL,
	DEV_ICAP,
	DEV_POST_CRC_INTERNAL,
	DEV_STARTUP,
	DEV_SLAVE_SPI,
	DEV_SUSPEND_SYNC,
	DEV_OCT_CALIBRATE,
	DEV_SPI_ACCESS
};

// All device configuration is structured so that the value
// 0 is never a valid configured setting. That way all config
// data can safely be initialized to 0 meaning unconfigured.

enum { LOGIC_M = 1, LOGIC_L, LOGIC_X };

struct fpgadev_logic
{
	int subtype; // LOGIC_M, LOGIC_L or LOGIC_X
	int A_used, B_used, C_used, D_used;
	char* A6_lut, *B6_lut, *C6_lut, *D6_lut;
};

enum { IOBM = 1, IOBS };
typedef char IOSTANDARD[32];
#define IO_LVCMOS33	"LVCMOS33"
enum { BYPASS_MUX_I = 1, BYPASS_MUX_O, BYPASS_MUX_T };
enum { IMUX_I_B = 1, IMUX_I };
enum { SLEW_SLOW = 1, SLEW_FAST, SLEW_QUIETIO };
enum { SUSP_LAST_VAL = 1, SUSP_3STATE, SUSP_3STATE_PULLUP,
	SUSP_3STATE_PULLDOWN, SUSP_3STATE_KEEPER, SUSP_3STATE_OCT_ON };
enum { ITERM_NONE = 1, ITERM_UNTUNED_25, ITERM_UNTUNED_50,
	ITERM_UNTUNED_75 };
enum { OTERM_NONE = 1, OTERM_UNTUNED_25, OTERM_UNTUNED_50,
	OTERM_UNTUNED_75 };

struct fpgadev_iob
{
	int subtype; // IOBM or IOBS
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

struct fpga_device
{
	enum fpgadev_type type;
	int instantiated;
	union {
		struct fpgadev_logic logic;
		struct fpgadev_iob iob;
	};
};

#define SWITCH_ON			0x80000000
#define SWITCH_BIDIRECTIONAL		0x40000000
#define SWITCH_MAX_CONNPT_O		0x7FFF // 15 bits

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
	//        29:15 from, index into conn_point_names
	//        14:0  to, index into conn_point_names
	int num_switches;
	uint32_t* switches;
};

int fpga_build_model(struct fpga_model* model,
	int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring);
void fpga_free_model(struct fpga_model* model);

const char* fpga_tiletype_str(enum fpga_tile_type type);

int init_tiles(struct fpga_model* model);
int init_conns(struct fpga_model* model);
int init_ports(struct fpga_model* model);
int init_devices(struct fpga_model* model);
void free_devices(struct fpga_model* model);
int init_switches(struct fpga_model* model);

const char* pf(const char* fmt, ...);
const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name);
char next_non_whitespace(const char* s);
char last_major(const char* str, int cur_o);
int has_connpt(struct fpga_model* model, int y, int x, const char* name);
int add_connpt_name(struct fpga_model* model, int y, int x, const char* connpt_name);

int has_device(struct fpga_model* model, int y, int x, int dev);
int has_device_type(struct fpga_model* model, int y, int x, int dev, int subtype);
int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2);

typedef int (*add_conn_f)(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
#define NOPREF_BI_F	add_conn_bi
#define PREF_BI_F	add_conn_bi_pref
#define NOPREF_UNI_F	add_conn_uni
#define PREF_UNI_F	add_conn_uni_pref

int add_conn_uni(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_uni_pref(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_bi(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_bi_pref(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2);
int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func,
	int y1, int x1, const char* name1, int start1, int last1,
	int y2, int x2, const char* name2, int start2);

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

struct w_net
{
	// if !last_inc, no incrementing will happen (NO_INCREMENT)
	// if last_inc > 0, incrementing will happen to
	// the %i in the name from 0:last_inc, for a total
	// of last_inc+1 wires.
	int last_inc;
	struct w_point pts[40];
};

int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net);

int add_switch(struct fpga_model* model, int y, int x, const char* from,
	const char* to, int is_bidirectional);
int add_switch_set(struct fpga_model* model, int y, int x, const char* prefix,
	const char** pairs, int suffix_inc);

struct seed_data
{
	int x_flags;
	const char* str;
};

void seed_strx(struct fpga_model* model, struct seed_data* data);

// The LWF flags are OR'ed into the logic_wire enum
#define LWF_SOUTH0		0x0100
#define LWF_NORTH3		0x0200
#define LWF_BIDIR		0x0400
#define LWF_FAN_B		0x0800
#define LWF_WIRE_MASK		0x00FF // namespace for the enums

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

enum logicout_wire {
    /*  0 */	X_A = 0,
		     X_AMUX, X_AQ, X_B, X_BMUX, X_BQ,
    /*  6 */	X_C, X_CMUX, X_CQ, X_D, X_DMUX, X_DQ,
    /* 12 */	M_A, M_AMUX, M_AQ, M_B, M_BMUX, M_BQ,
    /* 18 */	M_C, M_CMUX, M_CQ, M_D, M_DMUX, M_DQ
};

const char* logicin_str(enum logicin_wire w);
const char* logicout_str(enum logicout_wire w);

// The extra wires must not overlap with logicin_wire or logicout_wire
// namespaces so that they can be combined with either of them.
enum extra_wires {
	UNDEF = 100,
	FAN_B,
	GFAN0,
	GFAN1,
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
	LOGICIN_S62
};
