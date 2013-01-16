//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

// The highest 4 bits are the binary revision and not
// used when performing IDCODE verification.
// ug380, Configuration Sequence, page 78
#define IDCODE_MASK	0x0FFFFFFF

#define XC6SLX4		0x04000093
#define XC6SLX9		0x04001093
#define XC6SLX16	0x04002093
#define XC6SLX25	0x04004093
#define XC6SLX25T	0x04024093
#define XC6SLX45	0x04008093
#define XC6SLX45T	0x04028093
#define XC6SLX75	0x0400E093
#define XC6SLX75T	0x0402E093
#define XC6SLX100	0x04011093
#define XC6SLX100T	0x04031093
#define XC6SLX150	0x0401D093

#define XC_MAX_MAJORS		400
#define XC_MAX_TYPE2_ENTRIES	2000
#define XC_MAX_MUI_POS		32

#define XC_MAJ_ZERO		0x00000001
#define XC_MAJ_LEFT		0x00000002
#define XC_MAJ_CENTER		0x00000004
#define XC_MAJ_RIGHT		0x00000008
#define XC_MAJ_XM		0x00000010
#define XC_MAJ_XL		0x00000020
#define XC_MAJ_BRAM		0x00000040
#define XC_MAJ_MACC		0x00000080
#define XC_MAJ_TOP_BOT_IO	0x00000100
#define XC_MAJ_GCLK_SEP		0x00000200

struct xc_major_info
{
	int flags;
	int minors;
};

//
// major_str
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

// left_wiring and right_wiring are described with 16
// characters for each row, order is top-down
//   'W' = wired
//   'U' = unwired
//

#define XC6_NUM_GCLK_PINS 32

struct xc_t2_io_info
{
	int pair; // 0 for entries used for switches
	int pos_side; // 1 for positive, 0 for negative
	int bank;
	int y;
	int x;
	int type_idx;
};

struct xc_die
{
	int idcode;
	int num_rows;
	const char* left_wiring;
	const char* right_wiring;
	const char* major_str;
	int num_majors;
	struct xc_major_info majors[XC_MAX_MAJORS];

	int num_t2_ios;
	struct xc_t2_io_info t2_io[XC_MAX_TYPE2_ENTRIES];
	int num_gclk_pins;
	int gclk_t2_io_idx[XC6_NUM_GCLK_PINS];
	int gclk_t2_switches[XC6_NUM_GCLK_PINS]; // in 16-bit words

	int mcb_ypos;
	int num_mui;
	int mui_pos[XC_MAX_MUI_POS];
	int sel_logicin[16];
};

const struct xc_die* xc_die_info(int idcode);

int xc_die_center_major(const struct xc_die *die);

enum xc6_pkg { TQG144, FTG256, CSG324, FGG484 };
#define XC6_MAX_NUM_PINS 900 // fgg900 package

// see ug385
struct xc6_pin_info
{
	const char *name;
	int bank;
	const char *bufio2;
	const char *description;
	int pair;
	int pos_side;
};

struct xc6_pkg_info
{
	enum xc6_pkg pkg;
	int num_pins;
	struct xc6_pin_info pin[XC6_MAX_NUM_PINS];
};

const struct xc6_pkg_info *xc6_pkg_info(enum xc6_pkg pkg);

// returns 0 if description not found
const char *xc6_find_pkg_pin(const struct xc6_pkg_info *pkg_info, const char *description);

#define FRAME_SIZE		130
#define FRAMES_PER_ROW		505 // for slx4 and slx9
#define PADDING_FRAMES_PER_ROW	2
#define NUM_ROWS		4 // for slx9 and slx9

#define FRAMES_DATA_START	0
#define FRAMES_DATA_LEN		(NUM_ROWS*FRAMES_PER_ROW*FRAME_SIZE)
#define BRAM_DATA_START		FRAMES_DATA_LEN
#define BRAM_DATA_LEN		(4*144*FRAME_SIZE)
#define IOB_DATA_START		(BRAM_DATA_START + BRAM_DATA_LEN)
#define IOB_WORDS		896 // 16-bit words, for slx4 and slx9
#define IOB_DATA_LEN		(IOB_WORDS*2)
#define IOB_ENTRY_LEN		8
#define BITS_LEN		(IOB_DATA_START+IOB_DATA_LEN)

#define XC6_WORD_BYTES		2
#define XC6_WORD_BITS		(XC6_WORD_BYTES*8)

#define XC6_HCLK_POS		64
#define XC6_HCLK_BYTES		2
#define XC6_HCLK_BITS		(XC6_HCLK_BYTES*8)

#define XC6_HCLK_GCLK_UP_PIN	0
#define XC6_HCLK_GCLK_DOWN_PIN	1

#define XC6_NULL_MAJOR		0

#define XC6_IOB_MASK_IO				0x00FF00FFFF000000
#define XC6_IOB_MASK_IN_TYPE			0x000000000000F000
#define XC6_IOB_MASK_SLEW			0x0000000000FF0000
#define XC6_IOB_MASK_SUSPEND			0x000000000000001F

#define XC6_IOB_INSTANTIATED			0x0000000000000080
#define XC6_IOB_INPUT				0x00D0002400000000
#define XC6_IOB_INPUT_LVCMOS33_25_LVTTL		0x000000000000E000
#define XC6_IOB_INPUT_LVCMOS18_15_12		0x000000000000C000
#define XC6_IOB_INPUT_LVCMOS18_15_12_JEDEC	0x0000000000002000
#define XC6_IOB_INPUT_SSTL2_I			0x000000000000B000

#define XC6_IOB_OUTPUT_LVCMOS33_25_DRIVE_2	0x001000B400000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4		0x0070006C00000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6		0x003000FC00000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8		0x0040000000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12	0x0060008800000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16	0x009800C600000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24	0x0088007200000000

#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_4		0x00B0006C00000000
#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_6		0x004000FC00000000
#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_8		0x0000000000000000
#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_12	0x0058008800000000
#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_16	0x00B800C600000000
#define XC6_IOB_OUTPUT_LVCMOS25_DRIVE_24	0x0054007200000000

#define XC6_IOB_OUTPUT_LVTTL_DRIVE_2		0x009000B400000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_4		0x00F0006C00000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_6		0x007000FC00000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_8		0x0030000000000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_12		0x0080008800000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_16		0x006000C600000000
#define XC6_IOB_OUTPUT_LVTTL_DRIVE_24		0x0018007200000000

#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_2		0x00F000B402000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_4		0x00C000AC02000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_6		0x00E000BC02000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_8		0x00D800A002000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_12	0x003800A802000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_16	0x002800A602000000
#define XC6_IOB_OUTPUT_LVCMOS18_DRIVE_24	0x00A400A202000000

#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_2		0x00B0007402000000
#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_4		0x00E0000C02000000
#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_6		0x0098005C02000000
#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_8		0x00C8003002000000
#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_12	0x00F4001802000000
#define XC6_IOB_OUTPUT_LVCMOS15_DRIVE_16	0x002400D602000000

#define XC6_IOB_OUTPUT_LVCMOS12_DRIVE_2		0x004000B402000000
#define XC6_IOB_OUTPUT_LVCMOS12_DRIVE_4		0x0098006C02000000
#define XC6_IOB_OUTPUT_LVCMOS12_DRIVE_6		0x008800FC02000000
#define XC6_IOB_OUTPUT_LVCMOS12_DRIVE_8		0x0014000002000000
#define XC6_IOB_OUTPUT_LVCMOS12_DRIVE_12	0x00FC008802000000

#define XC6_IOB_OUTPUT_SSTL2_I			0x0040001C00000000

#define XC6_IOB_IMUX_I_B			0x0000000000000400
#define XC6_IOB_O_PINW				0x0000000000000100
#define XC6_IOB_SLEW_SLOW			0x0000000000000000
#define XC6_IOB_SLEW_FAST			0x0000000000330000
#define XC6_IOB_SLEW_QUIETIO			0x0000000000660000
#define XC6_IOB_SUSP_3STATE			0x0000000000000000
#define XC6_IOB_SUSP_3STATE_OCT_ON		0x0000000000000001
#define XC6_IOB_SUSP_3STATE_KEEPER		0x0000000000000002
#define XC6_IOB_SUSP_3STATE_PULLUP		0x0000000000000004
#define XC6_IOB_SUSP_3STATE_PULLDOWN		0x0000000000000008
#define XC6_IOB_SUSP_LAST_VAL			0x0000000000000010

int get_major_minors(int idcode, int major);

enum major_type { MAJ_ZERO, MAJ_LEFT, MAJ_RIGHT, MAJ_CENTER,
	MAJ_LOGIC_XM, MAJ_LOGIC_XL, MAJ_BRAM, MAJ_MACC };
enum major_type get_major_type(int idcode, int major);

#define XC6_ZERO_MAJOR 0
#define XC6_LEFTSIDE_MAJOR 1
#define XC6_SLX9_RIGHTMOST_MAJOR 17

#define XC6_SLX9_TOTAL_TILE_ROWS 73
#define XC6_SLX9_TOTAL_TILE_COLS 45

int get_rightside_major(int idcode);
int get_major_framestart(int idcode, int major);
int get_frames_per_row(int idcode);

// The routing bitpos is relative to a tile, i.e. major (x)
// and row/v64_i (y) are defined outside.
struct xc6_routing_bitpos
{
	// from and to are enum extra_wires values from model.h
	int from;
	int to;
	int bidir;

	// minors 0-19 are minor pairs, minor will be set
	// to the even beginning of the pair, two_bits_o and
	// one_bit_o are within 2*64 bits of the two minors.
	// Even bit offsets are from the first minor, odd bit
	// offsets from the second minor.
	// minor 20 is a regular single 64-bit minor.

	int minor; // 0,2,4,..18 for pairs, 20 for single-minor
	int two_bits_o; // 0-126 for pairs (even only), 0-62 for single-minor
	int two_bits_val; // 0-3
	int one_bit_o; // 1-6 positions up or down from two-bit pos
};

int get_xc6_routing_bitpos(struct xc6_routing_bitpos** bitpos, int* num_bitpos);
void free_xc6_routing_bitpos(struct xc6_routing_bitpos* bitpos);

#define XC6_LMAP_XM_M_A 0
#define XC6_LMAP_XM_M_B 1
#define XC6_LMAP_XM_M_C 0
#define XC6_LMAP_XM_M_D 1
#define XC6_LMAP_XM_X_A 2
#define XC6_LMAP_XM_X_B 2
#define XC6_LMAP_XM_X_C 3
#define XC6_LMAP_XM_X_D 3
#define XC6_LMAP_XL_L_A 3
#define XC6_LMAP_XL_L_B 2
#define XC6_LMAP_XL_L_C 3
#define XC6_LMAP_XL_L_D 2
#define XC6_LMAP_XL_X_A 2
#define XC6_LMAP_XL_X_B 2
#define XC6_LMAP_XL_X_C 3
#define XC6_LMAP_XL_X_D 3

// num_bits must be 32 or 64. If it is 32, the lower
// 32 entries of map contain the bit positions for lut5,
// the upper 32 entries of map the ones for lut6.
// In either case 64 entries are written to map.
void xc6_lut_bitmap(int lut_pos, int (*map)[64], int num_bits);

//
// logic configuration
//

//
// Some things noteworthy for *not* having bits set:
//  cout_used, a_used-d_used, srinit=0, non-inverted clock,
//  async attribute, precyinit=0, ffmux=O6, cy0=X, enabling
//  5Q-ff in X devices
//
// All offsets into vertical range of 64 bits per logic row.
//

// minor 20 (only bits 24-39 for logic config, 0-23 and 40-63
// are for routing switches):
#define XC6_MI20_LOGIC_MASK 0x000000FFFF000000ULL

#define XC6_ML_D5_FFSRINIT_1	24
#define XC6_X_D5_FFSRINIT_1	25
#define XC6_ML_C5_FFSRINIT_1	29
#define XC6_X_C_FFSRINIT_1	30
#define XC6_X_C5_FFSRINIT_1	31
#define XC6_ML_B5_FFSRINIT_1	32
#define XC6_X_B5_FFSRINIT_1	34
#define XC6_M_A_FFSRINIT_1	37 // M-device only
#define XC6_X_A5_FFSRINIT_1	38
#define XC6_ML_A5_FFSRINIT_1	39

// minor 26 in XM, 25 in XL columns:

// ML_D_CY0=DX			 -
#define XC6_ML_D_CY0_O5		 0	// implies lut5 on ML-D

// X_D_OUTMUX=5Q 		 -      // implies lut5 on X-D
#define XC6_X_D_OUTMUX_O5	 1	// default-set, does not imply lut5
// X_C_OUTMUX=5Q		 -	// implies lut5 on X-C
#define XC6_X_C_OUTMUX_O5	 2	// default-set, does not imply lut5

#define XC6_ML_D_FFSRINIT_1	 3
#define XC6_ML_C_FFSRINIT_1	 4
#define XC6_X_D_FFSRINIT_1	 5

// ML_C_CY0=CX			 -
#define XC6_ML_C_CY0_O5		 6	// implies lut5 on ML-C

// X_B_OUTMUX=5Q		 -	// implies lut5 on X-B
#define XC6_X_B_OUTMUX_O5	 7	// default-set, does not imply lut5

#define XC6_ML_D_OUTMUX_MASK	0x0000000000000F00ULL
#define XC6_ML_D_OUTMUX_O	 8
#define XC6_ML_D_OUTMUX_O6		 1ULL // 0001
#define XC6_ML_D_OUTMUX_XOR		 2ULL // 0010
#define XC6_ML_D_OUTMUX_O5		 5ULL // 0101, implies lut5 on ML-D
#define XC6_ML_D_OUTMUX_CY		 6ULL // 0110
#define XC6_ML_D_OUTMUX_5Q		 8ULL // 1000, implies lut5 on ML-D

#define XC6_ML_D_FFMUX_MASK	0x000000000000F000ULL
#define XC6_ML_D_FFMUX_O	12
#define XC6_ML_D_FFMUX_O6		 0ULL // 0000
#define XC6_ML_D_FFMUX_O5		 1ULL // 0001, implies lut5 on ML-D
#define XC6_ML_D_FFMUX_X		10ULL // 1010
#define XC6_ML_D_FFMUX_XOR		12ULL // 1100
#define XC6_ML_D_FFMUX_CY		13ULL // 1101

#define XC6_X_CLK_B		16
#define XC6_ML_ALL_LATCH	17
#define XC6_ML_SR_USED		18
#define XC6_ML_SYNC		19
#define XC6_ML_CE_USED		20
// X_D_FFMUX=O6			 -
#define XC6_X_D_FFMUX_X		21	// default-set, does not imply lut5
// X_C_FFMUX=O6			 -
#define XC6_X_C_FFMUX_X		22	// default-set, does not imply lut5
#define XC6_X_CE_USED		23

#define XC6_ML_C_OUTMUX_MASK	0x000000000F000000ULL
#define XC6_ML_C_OUTMUX_O	24
#define XC6_ML_C_OUTMUX_XOR		 1ULL // 0001
#define XC6_ML_C_OUTMUX_O6		 2ULL // 0010
#define XC6_ML_C_OUTMUX_5Q		 4ULL // 0100, implies lut5 on ML-C
#define XC6_ML_C_OUTMUX_CY		 9ULL // 1001
#define XC6_ML_C_OUTMUX_O5		10ULL // 1010, implies lut5 on ML-C
#define XC6_ML_C_OUTMUX_F7		12ULL // 1100

#define XC6_ML_C_FFMUX_MASK	0x00000000F0000000ULL
#define XC6_ML_C_FFMUX_O	28
#define XC6_ML_C_FFMUX_O6		 0ULL // 0000
#define XC6_ML_C_FFMUX_O5		 2ULL // 0010, implies lut5 on ML-C
#define XC6_ML_C_FFMUX_X		 5ULL // 0101
#define XC6_ML_C_FFMUX_F7		 7ULL // 0111
#define XC6_ML_C_FFMUX_XOR		12ULL // 1100
#define XC6_ML_C_FFMUX_CY		14ULL // 1110

#define XC6_ML_B_OUTMUX_MASK	0x0000000F00000000ULL
#define XC6_ML_B_OUTMUX_O	32
#define XC6_ML_B_OUTMUX_5Q		 2ULL // 0010, implies lut5 on ML-B
#define XC6_ML_B_OUTMUX_F8		 3ULL // 0011
#define XC6_ML_B_OUTMUX_XOR		 4ULL // 0100
#define XC6_ML_B_OUTMUX_CY		 5ULL // 0101
#define XC6_ML_B_OUTMUX_O6		 8ULL // 1000
#define XC6_ML_B_OUTMUX_O5		 9ULL // 1001, implies lut5 on ML-B

// X_B_FFMUX=O6			 -
#define XC6_X_B_FFMUX_X		36	// default-set, does not imply lut5
// X_A_FFMUX=O6			 -
#define XC6_X_A_FFMUX_X		37	// default-set, does not imply lut5
#define XC6_X_B_FFSRINIT_1	38
// X_A_OUTMUX=5Q		 -	// implies lut5 on X-A
#define XC6_X_A_OUTMUX_O5	39	// default-set, does not imply lut5
#define XC6_X_SR_USED		40
#define XC6_X_SYNC		41
#define XC6_X_ALL_LATCH		42
#define XC6_ML_CLK_B		43

#define XC6_ML_B_FFMUX_MASK	0x0000F00000000000ULL
#define XC6_ML_B_FFMUX_O	44
#define XC6_ML_B_FFMUX_O6		 0ULL // 0000
#define XC6_ML_B_FFMUX_XOR		 3ULL // 0011
#define XC6_ML_B_FFMUX_O5		 4ULL // 0100, implies lut5 on ML-B
#define XC6_ML_B_FFMUX_CY		 7ULL // 0111
#define XC6_ML_B_FFMUX_X		10ULL // 1010
#define XC6_ML_B_FFMUX_F8		14ULL // 1110

#define XC6_ML_A_FFMUX_MASK	0x000F000000000000ULL
#define XC6_ML_A_FFMUX_O	48
#define XC6_ML_A_FFMUX_O6		 0ULL // 0000
#define XC6_ML_A_FFMUX_XOR 		 3ULL // 0011
#define XC6_ML_A_FFMUX_X 		 5ULL // 0101
#define XC6_ML_A_FFMUX_O5		 8ULL // 1000, implies lut5 on ML-A
#define XC6_ML_A_FFMUX_CY		11ULL // 1011
#define XC6_ML_A_FFMUX_F7		13ULL // 1101

#define XC6_ML_A_OUTMUX_MASK	0x00F0000000000000ULL
#define XC6_ML_A_OUTMUX_O	52
#define XC6_ML_A_OUTMUX_5Q		 1ULL // 0001, implies lut5 on ML-A
#define XC6_ML_A_OUTMUX_F7		 3ULL // 0011
#define XC6_ML_A_OUTMUX_XOR		 4ULL // 0100
#define XC6_ML_A_OUTMUX_CY		 6ULL // 0110
#define XC6_ML_A_OUTMUX_O6		 8ULL // 1000
#define XC6_ML_A_OUTMUX_O5		10ULL // 1010, implies lut5 on ML-A

// ML_B_CY0=BX			 -
#define XC6_ML_B_CY0_O5		56	// implies lut5 on ML-B
#define XC6_ML_PRECYINIT_AX	57
#define XC6_X_A_FFSRINIT_1	58
// CIN_USED best corresponds to the cout->cout_n switch in the
// next lower logic device (y+1).
#define XC6_ML_CIN_USED		59
// ML_PRECYINIT=0		 -
#define XC6_ML_PRECYINIT_1	60
#define XC6_ML_B_FFSRINIT_1	61
// ML_A_CY0=AX			 -
#define XC6_ML_A_CY0_O5		62	// implies lut5 on ML-A

#define XC6_L_A_FFSRINIT_1	63	// L-device only

#define XC6_TYPE2_GCLK_REG_SW	 2 // bit 2 in 1st word
#define XC6_CENTER_GCLK_MINOR	25
