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

int get_major_minors(int idcode, int major);

enum major_type { MAJ_ZERO, MAJ_LEFT, MAJ_RIGHT, MAJ_CENTER,
	MAJ_LOGIC_XM, MAJ_LOGIC_XL, MAJ_BRAM, MAJ_MACC };
enum major_type get_major_type(int idcode, int major);

int get_num_iobs(int idcode);
const char* get_iob_sitename(int idcode, int idx);

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
