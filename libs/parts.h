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

#define XC6_HCLK_BYTES		2
#define XC6_HCLK_BITS		(XC6_HCLK_BYTES*8)

#define XC6_IOB_MASK_INSTANTIATED		0x0000000000000080
#define XC6_IOB_MASK_IO				0x00FF00FF00000000
#define XC6_IOB_MASK_O_PINW			0x0000000000000100
#define XC6_IOB_MASK_I_MUX			0x000000000000E400
#define XC6_IOB_MASK_SLEW			0x0000000000FF0000
#define XC6_IOB_MASK_SUSPEND			0x000000000000001F

#define XC6_IOB_INSTANTIATED			0x0000000000000080
#define XC6_IOB_INPUT_LVCMOS33			0x00D0024000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_2		0x00100B4000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4		0x007006C000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6		0x00300FC000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8		0x0040000000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12	0x0060088000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16	0x00980C6000000000
#define XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24	0x0088072000000000
#define XC6_IOB_O_PINW				0x0000000000000100
#define XC6_IOB_IMUX_I				0x000000000000E000
#define XC6_IOB_IMUX_I_B			0x000000000000E400
#define XC6_IOB_SLEW_SLOW			0x0000000000000000
#define XC6_IOB_SLEW_QUIETIO			0x0000000000330000
#define XC6_IOB_SLEW_FAST			0x0000000000660000
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

int get_rightside_major(int idcode);

int get_num_iobs(int idcode);
const char* get_iob_sitename(int idcode, int idx);
// returns -1 if sitename not found
int find_iob_sitename(int idcode, const char* name);

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
