//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "helper.h"

// 120 MB max bitstream size is enough for now
#define BITSTREAM_READ_PAGESIZE		4096
#define BITSTREAM_READ_MAXPAGES		30000	// 120 MB max bitstream

//
// xc6 configuration registers, documentation in ug380, page90
//

enum {
	CRC = 0, FAR_MAJ, FAR_MIN, FDRI, FDRO, CMD, CTL, MASK, STAT, LOUT, COR1,
	COR2, PWRDN_REG, FLR, IDCODE, CWDT, HC_OPT_REG, CSBO = 18,
	GENERAL1, GENERAL2, GENERAL3, GENERAL4, GENERAL5, MODE_REG, PU_GWE,
	PU_GTS, MFWR, CCLK_FREQ, SEU_OPT, EXP_SIGN, RDBK_SIGN, BOOTSTS,
	EYE_MASK, CBC_REG
};

// The highest 4 bits are the binary revision and not
// used when performing IDCODE verification.
// ug380, Configuration Sequence, page 78
typedef struct
{
	char* name;
	uint32_t code;
} IDCODE_S;

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

const IDCODE_S idcodes[] =
{
	{"XC6SLX4",	XC6SLX4},
	{"XC6SLX9",	XC6SLX9},
	{"XC6SLX16",	XC6SLX16},
	{"XC6SLX25",	XC6SLX25},
	{"XC6SLX25T",	XC6SLX25T},
	{"XC6SLX45",	XC6SLX45},
	{"XC6SLX45T",	XC6SLX45T},
	{"XC6SLX75",	XC6SLX75},
	{"XC6SLX75T",	XC6SLX75T},
	{"XC6SLX100",	XC6SLX100},
	{"XC6SLX100T",	XC6SLX100T},
	{"XC6SLX150",	XC6SLX150}
};

enum {
	CMD_NULL = 0, CMD_WCFG, CMD_MFW, CMD_LFRM, CMD_RCFG, CMD_START,
	CMD_RCRC = 7, CMD_AGHIGH, CMD_GRESTORE = 10, CMD_SHUTDOWN,
	CMD_DESYNC = 13, CMD_IPROG
};

const char* cmds[] =
{
	[CMD_NULL] = "NULL",
	[CMD_WCFG] = "WCFG",
	[CMD_MFW] = "MFW",
	[CMD_LFRM] = "LFRM",
	[CMD_RCFG] = "RCFG",
	[CMD_START] = "START",
	[CMD_RCRC] = "RCRC",
	[CMD_AGHIGH] = "AGHIGH",
	[CMD_GRESTORE] = "GRESTORE",
	[CMD_SHUTDOWN] = "SHUTDOWN",
	[CMD_DESYNC] = "DESYNC",
	[CMD_IPROG] = "IPROG"
};

typedef enum {
	MAJ_EXTRA = -1,
	MAJ_CLB, MAJ_BRAM
} major_t;

typedef struct
{
	char* name;
	int minors;
	major_t type;
} MAJOR;

const MAJOR majors[] =
{
	/*  0 */ { 0,		 4, MAJ_EXTRA }, // clock?
	/*  1 */ { 0,		30, MAJ_EXTRA },
	/*  2 */ { "clb",	31, MAJ_CLB },
	/*  3 */ { 0,		30, MAJ_EXTRA },
	/*  4 */ { "bram",	25, MAJ_BRAM },
	/*  5 */ { "clb",	31, MAJ_CLB },
	/*  6 */ { 0,		30, MAJ_EXTRA },
	/*  7 */ { 0,		24, MAJ_EXTRA }, // dsp?
	/*  8 */ { "clb",	31, MAJ_CLB },
	/*  9 */ { 0,		31, MAJ_EXTRA },
	/* 10 */ { "clb",	31, MAJ_CLB },
	/* 11 */ { 0,		30, MAJ_EXTRA },
	/* 12 */ { 0,		31, MAJ_EXTRA },
	/* 13 */ { 0,		30, MAJ_EXTRA },
	/* 14 */ { 0,		25, MAJ_EXTRA },
	/* 15 */ { "clb",	31, MAJ_CLB },
	/* 16 */ { 0,		30, MAJ_EXTRA },
	/* 17 */ { 0,		30, MAJ_EXTRA }
};

typedef struct ramb16_cfg
{
	uint8_t byte[64];
} __attribute((packed)) ramb16_cfg_t;

static const cfg_atom_t ramb16_instance =
{
	{-1}, {12,13, 274,275,276,277,316,317,318,319,
	       420,421,422,423,-1}, "default_bits"
};

static cfg_atom_t ramb16_atoms[] =
{
  // data_width_a
  {{264,265,260,261,256,257,-1},{                        -1},"data_width_a 1"},
  {{264,265,260,261,        -1},{                256,257,-1},"data_width_a 2"},
  {{264,265,        256,257,-1},{        260,261,        -1},"data_width_a 4"},
  {{264,265,                -1},{        260,261,256,257,-1},"data_width_a 9"},
  {{        260,261,256,257,-1},{264,265,                -1},"data_width_a 18"},
  {{        260,261,        -1},{264,265,        256,257,-1},"data_width_a 36"},
  {{                        -1},{264,265,260,261,256,257,-1},"data_width_a 0"},

  // data_width_b
  {{262,263,286,287,258,259,-1},{                        -1},"data_width_b 1"},
  {{262,263,286,287,        -1},{                258,259,-1},"data_width_b 2"},
  {{262,263,        258,259,-1},{        286,287,        -1},"data_width_b 4"},
  {{262,263,                -1},{        286,287,258,259,-1},"data_width_b 9"},
  {{        286,287,258,259,-1},{262,263,                -1},"data_width_b 18"},
  {{        286,287,        -1},{262,263,        258,259,-1},"data_width_b 36"},
  {{                        -1},{262,263,286,287,258,259,-1},"data_width_b 0"},

	// required
	{ {           -1}, {266, 267, -1}, "RST_PRIORITY_B:CE"  },
	{ {266, 267,  -1}, {          -1}, "RST_PRIORITY_B:SR"  },
	{ {           -1}, {268, 269, -1}, "RST_PRIORITY_A:CE"  },
	{ {268, 269,  -1}, {          -1}, "RST_PRIORITY_A:SR"  },
	{ {           -1}, {290, 291, -1}, "EN_RSTRAM_A:TRUE"  },
	{ {290, 291,  -1}, {          -1}, "EN_RSTRAM_A:FALSE"  },
	{ {           -1}, {444, 445, -1}, "EN_RSTRAM_B:TRUE" },
	{ {444, 445,  -1}, {          -1}, "EN_RSTRAM_B:FALSE" },

	// optional
	{ {           -1}, { 26,  27, -1}, "CLKAINV:CLKA" },
	{ { 26,  27,  -1}, {          -1}, "CLKAINV:CLKA_B" }, // def
	{ {           -1}, { 30,  31, -1}, "CLKBINV:CLKB"  },
	{ { 30,  31,  -1}, {          -1}, "CLKBINV:CLKB_B" }, // def
	{ {           -1}, {270, 271, -1}, "RSTTYPE:ASYNC"  },
	{ {270, 271,  -1}, {          -1}, "RSTTYPE:SYNC"  }, // def
	{ {           -1}, {278, 279, -1}, "WRITE_MODE_B:READ_FIRST"  },
	{ {           -1}, {280, 281, -1}, "WRITE_MODE_A:READ_FIRST"  },
	{ {           -1}, {282, 283, -1}, "WRITE_MODE_B:NO_CHANGE"  },
	{ {           -1}, {284, 285, -1}, "WRITE_MODE_A:NO_CHANGE"  },
	{ {278, 279, 282, 283, -1},  {-1}, "WRITE_MODE_B:WRITE_FIRST"  }, //def
	{ {280, 281, 284, 285, -1},  {-1}, "WRITE_MODE_A:WRITE_FIRST"  }, //def
	{ {           -1}, {306, 307, -1}, "DOB_REG:1"  },
	{ {306, 306,  -1}, {          -1}, "DOB_REG:0"  }, // def
	{ {           -1}, {308, 309, -1}, "DOA_REG:1"  },
	{ {308, 309,  -1}, {          -1}, "DOA_REG:0"  }, // def
	{ {431, 467,  -1}, {430, 466, -1}, "ENAINV:ENA" }, // def
	{ {430, 431, 466, 467, -1},  {-1}, "ENAINV:ENA_B" },
	{ {465, 469,  -1}, {464, 468, -1}, "ENBINV:ENB" }, // def
	{ {464, 465, 468, 469, -1},  {-1}, "ENBINV:ENB_B" },
	{ {           -1}, { 20,  21, -1}, "REGCEAINV:REGCEA" }, // def
	{ { 20,  21,  -1}, {          -1}, "REGCEAINV:REGCEA_B" },
	{ {           -1}, {  8,   9, -1}, "REGCEBINV:REGCEB" },
	{ {  8,   9,  -1}, {          -1}, "REGCEBINV:REGCEB_B" }, // def
	{ { 24,  25,  -1}, {          -1}, "RSTAINV:RSTA" }, // def
	{ {           -1}, { 24,  25, -1}, "RSTAINV:RSTA_B" },
	{ {           -1}, {  4,   5, -1}, "RSTBINV:RSTB" }, // def
	{ {  4,   5,  -1}, {          -1}, "RSTBINV:RSTB_B" },
	{ {           -1}, {      19, -1}, "WEA0INV:WEA0" }, // def
	{ { 19,       -1}, {          -1}, "WEA0INV:WEA0_B" },
	{ {           -1}, {      23, -1}, "WEA2INV:WEA1" }, // def
	{ { 23,       -1}, {          -1}, "WEA2INV:WEA1_B" },
	{ {           -1}, {      18, -1}, "WEA2INV:WEA2" }, // def
	{ { 18,       -1}, {          -1}, "WEA2INV:WEA2_B" },
	{ {           -1}, {      22, -1}, "WEA2INV:WEA3" }, // def
	{ { 22,       -1}, {          -1}, "WEA2INV:WEA3_B" },
	{ {           -1}, {       7, -1}, "WEB0INV:WEB0" }, // def
	{ {  7,       -1}, {          -1}, "WEB0INV:WEB0_B" },
	{ {           -1}, {       3, -1}, "WEB1INV:WEB1" }, // def
	{ {  3,       -1}, {          -1}, "WEB1INV:WEB1_B" },
	{ {           -1}, {       6, -1}, "WEB2INV:WEB2" }, // def
	{ {  6,       -1}, {          -1}, "WEB2INV:WEB2_B" },
	{ {           -1}, {       2, -1}, "WEB3INV:WEB3" }, // def
	{ {  2,       -1}, {          -1}, "WEB3INV:WEB3_B" },
};

int g_cmd_frames = 0;
int g_cmd_info = 0; // whether to print #I info messages (offsets and others)

void print_ramb16_cfg(ramb16_cfg_t* cfg)
{
	char bits[512];
	uint8_t u8;
	int i, first_extra;

	for (i = 0; i < 32; i++) {
		u8 = cfg->byte[i*2];
		cfg->byte[i*2] = cfg->byte[i*2+1];
		cfg->byte[i*2+1] = u8;
	}
	for (i = 0; i < 64; i++) {
		u8 = 0;
		if (cfg->byte[i] & 0x01) u8 |= 0x80;
		if (cfg->byte[i] & 0x02) u8 |= 0x40;
		if (cfg->byte[i] & 0x04) u8 |= 0x20;
		if (cfg->byte[i] & 0x08) u8 |= 0x10;
		if (cfg->byte[i] & 0x10) u8 |= 0x08;
		if (cfg->byte[i] & 0x20) u8 |= 0x04;
		if (cfg->byte[i] & 0x40) u8 |= 0x02;
		if (cfg->byte[i] & 0x80) u8 |= 0x01;
		cfg->byte[i] = u8;
	}

	//
	// Bits 0..255 come from minor 23, Bits 256..511 from minor 24.
	// Each set of 256 bits is divided into two halfs of 128 bits
	// that are swept forward and backward to form 2-bit pairs,
	// pairs 0..127 are formed out of bits 0..127 and 255..128,
	// p128..p255 are formed out of b256..b383 and b511..b384.
	// Since so much bit twiddling is already happening, we are sorting
	// the bits so that pairs are next to each other.
	// The notation for a pair is "p8=01".

	// minor 23
	for (i = 0; i < 128; i++) {
		bits[i*2] = (cfg->byte[i/8] & (1<<(i%8))) != 0;
		bits[i*2+1] = (cfg->byte[(255-i)/8]
			& (1<<(7-(i%8)))) != 0;
	}
	// minor 24
	for (i = 0; i < 128; i++) {
		bits[256+i*2] = (cfg->byte[32+i/8] & (1<<(i%8))) != 0;
		bits[256+i*2+1] = (cfg->byte[32+(255-i)/8]
			& (1<<(7-(i%8)))) != 0;
	}

	printf("{\n");
	// hexdump(1 /* indent */, &cfg->byte[0], 64 /* len */);
	for (i = 0; i < sizeof(ramb16_atoms)/sizeof(ramb16_atoms[0]); i++) {
		if (atom_found(bits, &ramb16_atoms[i])
		    && ramb16_atoms[i].must_1[0] != -1) {
			printf("  %s\n", ramb16_atoms[i].str);
			ramb16_atoms[i].flag = 1;
		} else
			ramb16_atoms[i].flag = 0;
	}
	for (i = 0; i < sizeof(ramb16_atoms)/sizeof(ramb16_atoms[0]); i++) {
		if (ramb16_atoms[i].flag)
			atom_remove(bits, &ramb16_atoms[i]);
	}
	// instantiation bits
	if (ramb16_instance.must_1[0] != -1) {
		if (atom_found(bits, &ramb16_instance)) {
			for (i = 0; ramb16_instance.must_1[i] != -1; i++)
				printf("  b%i\n", ramb16_instance.must_1[i]);
			atom_remove(bits, &ramb16_instance);
		} else
			printf("  #W Not all instantiation bits set.\n");
	}
	// extra bits
	first_extra = 1;
	for (i = 0; i < 512; i++) {
		if (bits[i]) {
			if (first_extra) {
				printf("  #W Extra bits set.\n");
				first_extra = 0;
			}
			printf("  b%i\n", i);
		}
	}
	printf("}\n");
}

int FAR_pos(int FAR_row, int FAR_major, int FAR_minor)
{
	int result, i;

	if (FAR_row < 0 || FAR_major < 0 || FAR_minor < 0)
		return -1;
	if (FAR_row > 3 || FAR_major > 17
	    || FAR_minor >= majors[FAR_major].minors)
		return -1;
	result = FAR_row * 505*130;
	for (i = 0; i < FAR_major; i++)
		result += majors[i].minors*130;
	return result + FAR_minor*130;
}

int full_map(uint8_t* bit_file, int bf_len, int first_FAR_off,
	uint8_t** bits, int* bits_len, int idcode, int FLR_len, int* outdelta)
{
	int src_off, packet_hdr_type, packet_hdr_opcode;
	int packet_hdr_register, packet_hdr_wordcount;
	int FAR_block, FAR_row, FAR_major, FAR_minor, i, j, MFW_src_off;
	int offset_in_bits, block0_words, padding_frames;
	uint16_t u16;
	uint32_t u32;

	*bits = 0;
	if (idcode != XC6SLX4) goto fail;
	if (FLR_len != 896) goto fail;

	*bits_len = (4*505 + 4*144) * 130 + 896*2;
	*bits = calloc(*bits_len, 1 /* elsize */);
	if (!(*bits)) {
		fprintf(stderr, "#E Cannot allocate %i bytes for bits.\n",
			*bits_len);
		goto fail;
	}
	FAR_block = -1;
	FAR_row = -1;
	FAR_major = -1;
	FAR_minor = -1;
	MFW_src_off = -1;
	// Go through bit_file from first_FAR_off until last byte of
	// IOB was read, plus padding, plus CRC verification.
	src_off = first_FAR_off;
	while (src_off < bf_len) {
		if (src_off + 2 > bf_len) goto fail;
		u16 = __be16_to_cpu(*(uint16_t*)&bit_file[src_off]);
		src_off += 2;

		// 3 bits: 001 = Type 1; 010 = Type 2
		packet_hdr_type = (u16 & 0xE000) >> 13;
		if (packet_hdr_type != 1 && packet_hdr_type != 2)
			goto fail;

		// 2 bits: 00 = noop; 01 = read; 10 = write; 11 = reserved
		packet_hdr_opcode = (u16 & 0x1800) >> 11;
		if (packet_hdr_opcode == 3) goto fail;

		if (packet_hdr_opcode == 0) { // noop
			if (packet_hdr_type != 1 || u16 & 0x07FF) goto fail;
			continue;
		}

		// Now we must look at a Type 1 command
		packet_hdr_register = (u16 & 0x07E0) >> 5;
		packet_hdr_wordcount = u16 & 0x001F;
		if (src_off + packet_hdr_wordcount*2 > bf_len) goto fail;

		if (packet_hdr_type == 1) {
			if (packet_hdr_register == CMD) {
				if (packet_hdr_wordcount != 1) goto fail;
				u16 = __be16_to_cpu(
					*(uint16_t*)&bit_file[src_off]);
				if (u16 == CMD_GRESTORE || u16 == CMD_LFRM) {
					src_off -= 2;
					goto success;
				}
				if (u16 != CMD_MFW && u16 != CMD_WCFG)
					goto fail;
				if (u16 == CMD_MFW) {
					if (FAR_block != 0) goto fail;
					MFW_src_off = FAR_pos(FAR_row, FAR_major, FAR_minor);
					if (MFW_src_off == -1) goto fail;
				}
				src_off += 2;
				continue;
			}
			if (packet_hdr_register == FAR_MAJ) {
				uint16_t maj, min;

				if (packet_hdr_wordcount != 2) goto fail;
				maj = __be16_to_cpu(*(uint16_t*)
					&bit_file[src_off]);
				min = __be16_to_cpu(*(uint16_t*)
					&bit_file[src_off+2]);

				FAR_block = (maj & 0xF000) >> 12;
				if (FAR_block > 7) goto fail;
				FAR_row = (maj & 0x0F00) >> 8;
				FAR_major = maj & 0x00FF;
				FAR_minor = min & 0x03FF;
				src_off += 4;
				continue;
			}
			if (packet_hdr_register == MFWR) {
				uint32_t first_dword, second_dword;

				if (packet_hdr_wordcount != 4) goto fail;
				first_dword = __be32_to_cpu(
					*(uint32_t*)&bit_file[src_off]);
				second_dword = __be32_to_cpu(
					*(uint32_t*)&bit_file[src_off+4]);
				if (first_dword || second_dword) goto fail;
				// The first MFWR will overwrite itself, so
				// use memmove().
				if (FAR_block != 0) goto fail;
				offset_in_bits = FAR_pos(FAR_row, FAR_major, FAR_minor);
				if (offset_in_bits == -1) goto fail;
				memmove(&(*bits)[offset_in_bits], &(*bits)[MFW_src_off], 130);
				   
				src_off += 8;
				continue;
			}
			goto fail;
		}

		// packet type must be 2 here
		if (packet_hdr_wordcount != 0) goto fail;
		if (packet_hdr_register != FDRI) goto fail;

		if (src_off + 4 > bf_len) goto fail;
		u32 = __be32_to_cpu(*(uint32_t*)&bit_file[src_off]);
		src_off += 4;
		if (src_off+2*u32 > bf_len) goto fail;
		if (2*u32 < 130) goto fail;

		// fdri words u32
		if (FAR_block == -1 || FAR_block > 1 || FAR_row == -1
		    || FAR_major == -1 || FAR_minor == -1)
			goto fail;

		block0_words = 0;
		if (!FAR_block) {

			offset_in_bits = FAR_pos(FAR_row, FAR_major, FAR_minor);
			if (offset_in_bits == -1) goto fail;
			if (!FAR_row && !FAR_major && !FAR_minor
			    && u32 > 4*(505+2)*65)
				block0_words = 4*(505+2)*65;
			else {
				block0_words = u32;
				if (block0_words % 65) goto fail;
			}
			padding_frames = 0;
			for (i = 0; i < block0_words/65; i++) {
				if (i && i+1 == block0_words/65) {
					for (j = 0; j < 130; j++) {
						if (bit_file[src_off+i*130+j]
						    != 0xFF) break;
					}
					// Not sure about the exact logic to
					// determine a padding frame. Maybe
					// first word all 1? For now we skip
					// the frame as a padding frame when
					// it's the last frame of a block and
					// all-1.
					if (j >= 130)
						break;
				}
				if (!FAR_major && !FAR_minor
				    && (i%507 == 505)) {
					for (j = 0; j < 2*130; j++) {
						if (bit_file[src_off+i*130+j]
						    != 0xFF) goto fail;
					}
					i++;
					padding_frames += 2;
					continue;
				}
				memcpy(&(*bits)[offset_in_bits
					+ (i-padding_frames)*130],
					&bit_file[src_off
					+ i*130], 130);
			}
		}
		if (u32 - block0_words > 0) {
			int bram_data_words = 4*144*65 + 896;
			if (u32 - block0_words != bram_data_words + 1) goto fail;
			offset_in_bits = 4*505*130;
			memcpy(&(*bits)[offset_in_bits],
				&bit_file[src_off+block0_words*2],
				bram_data_words*2);
			u16 = __be16_to_cpu(*(uint16_t*)&bit_file[
			  (src_off+block0_words+bram_data_words)*2]);
			if (u16) goto fail;
		}
		src_off += 2*u32;
		// two CRC words
		u32 = __be32_to_cpu(*(uint32_t*)&bit_file[src_off]);
		src_off += 4;
	}
fail:
	free(*bits);
	*bits = 0;
	return -1;
success:
	*outdelta = src_off - first_FAR_off;
	return 0;
}

void printf_clb(uint8_t* maj_bits, int row, int major)
{
	int i, j, start, max_idx, frame_off;
	const char* lut_str;
	uint64_t lut64;

	// the first two slots on top and bottom row are not used for clb
	if (!row) {
		start = 2;
		max_idx = 16;
	} else if (row == 2) {
		start = 0;
		max_idx = 14;
	} else {
		start = 0;
		max_idx = 16;
	}

	for (i = start; i < max_idx; i++) {
		if (clb_empty(maj_bits, i))
			continue;
		frame_off = i*64;
		if (i >= 8)
			frame_off += 16; // skip clock bits for idx >= 8
		printf("r%i ma%i clb i%i\n", row, major, i-start);
		printf("{\n");

		// LUTs
		lut64 = read_lut64(&maj_bits[24*130], frame_off+32);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s0_A6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[21*130], frame_off+32);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s0_B6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[24*130], frame_off);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s0_C6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[21*130], frame_off);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s0_D6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[27*130], frame_off+32);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s1_A6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[29*130], frame_off+32);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s1_B6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[27*130], frame_off);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s1_C6LUT \"%s\"\n", lut_str);

		lut64 = read_lut64(&maj_bits[29*130], frame_off);
		lut_str = lut2bool(lut64, 64);
		if (*lut_str)
			printf(" s1_D6LUT \"%s\"\n", lut_str);

		// bits
		for (j = 0; j < 64; j++) {
			if (bit_set(&maj_bits[20*130], frame_off + j))
				printf(" mi20 b%i\n", j); 
		}
		for (j = 0; j < 64; j++) {
			if (bit_set(&maj_bits[23*130], frame_off + j))
				printf(" mi23 b%i\n", j); 
		}
		for (j = 0; j < 64; j++) {
			if (bit_set(&maj_bits[26*130], frame_off + j))
				printf(" mi26 b%i\n", j); 
		}
		printf("}\n");
	}
}

void printf_bits(uint8_t* bits, int bits_len, int idcode)
{
	int row, major, minor, i, j, off, bram_data_start;
	int offset_in_frame, newline;

	// type0
	off = 0;
	printf("\n");
	for (row = 0; row < 4; row++) {
		for (major = 0; major < 18; major++) {
			if (majors[major].type == MAJ_CLB) {
				minor = 0;
				while (minor < 20) {
					minor += printf_frames(&bits[off
					  +minor*130], 31 - minor, row,
					  major, minor, g_cmd_info);
				}

				// clock
				for (minor = 20; minor < 31; minor++)
					printf_clock(&bits[off+minor*130],
						row, major, minor);
				// extra bits at bottom of row0 and top of row2
				if (row == 2)
					printf_singlebits(&bits[off], 20, 11,
						0, 128, row, major);
				else if (!row)
					printf_singlebits(&bits[off], 20, 11,
						14*64 + 16, 128, row, major);

				// clbs
				printf_clb(&bits[off], row, major);
			} else if (majors[major].type == MAJ_BRAM) {
				ramb16_cfg_t ramb16_cfg[4];

				// minors 0..22
				minor = 0;
				while (minor < 23) {
					minor += printf_frames(&bits[off
					  +minor*130], 23 - minor, row,
					  major, minor, g_cmd_info);
				}

				// minors 23&24
				printf_clock(&bits[off+23*130], row, major, 23);
				printf_clock(&bits[off+24*130], row, major, 24);
				for (i = 0; i < 4; i++) {
					offset_in_frame = i*32;
					if (offset_in_frame >= 64)
						offset_in_frame += 2;
					for (j = 0; j < 32; j++) {
						ramb16_cfg[i].byte[j] = bits[off+23*130+offset_in_frame+j];
						ramb16_cfg[i].byte[j+32] = bits[off+24*130+offset_in_frame+j];
					}
				}
				for (i = 0; i < 4; i++) {
					for (j = 0; j < 64; j++) {
						if (ramb16_cfg[i].byte[j])
							break;
					}
					if (j >= 64)
						continue;
					printf("r%i m%i i%i ramb16\n", row, major, i);
					print_ramb16_cfg(&ramb16_cfg[i]);
				}
			} else {
				minor = 0;
				while (minor < majors[major].minors) {
					minor += printf_frames(&bits[off
					  +minor*130], majors[major].minors
					  - minor, row, major, minor, g_cmd_info);
				}
			}
			off += majors[major].minors * 130;
		}
	}

	// bram
	bram_data_start = 4*505*130;
	newline = 0;
	for (row = 0; row < 4; row++) {
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 18*130; j++) {
				if (bits[bram_data_start + row*144*130
					  + i*18*130 + j])
					break;
			}
			if (j >= 18*130)
				continue;
			if (!newline) {
				newline = 1;
				printf("\n");
			}
			printf("br%i i%i ramb16 data\n", row, i);
			printf("{\n");
			off = bram_data_start + row*144*130 + i*18*130;
			printf_ramb16_data(bits, off);
			printf("}\n");
		}
	}

	// iob
	printf("\n");
	if (printf_iob(bits, bits_len, bram_data_start + 4*144*130, 896*2/8))
		printf("\n");
}

int main(int argc, char** argv)
{
	uint8_t* bit_data = 0; // file contents
	uint8_t* bits = 0; // bits in chip layout
	FILE* bitf = 0;
	int bit_cur, try_full_map, first_FAR_off, bits_len;
	uint32_t bit_eof, cmd_len, u32, u16_off;
	uint16_t u16, packet_hdr_type, packet_hdr_opcode;
	uint16_t packet_hdr_register, packet_hdr_wordcount;
	char* bit_path = 0;
	int i, num_frames, times;

	// state machine driven from file input
	int m_FLR_value = -1;
	int m_idcode = -1; // offset into idcodes

	//
	// parse command line
	//

	if (argc < 2) {
		printf_help();
		return EXIT_SUCCESS;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help")) {
			printf_help();
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[i], "--version")) {
			printf("%s\n", PROGRAM_REVISION);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[i], "--info"))
			g_cmd_info = 1;
		else if (!strcmp(argv[i], "--frames"))
			g_cmd_frames = 1;
		else {
			bit_path = argv[i];
			if (argc > i+1) { // only 1 path supported
				printf_help();
				return EXIT_FAILURE;
			}
		}
	}
	if (!bit_path) { // shouldn't get here, just in case
		printf_help();
		return EXIT_FAILURE;
	}

	//
	// read .bit into memory
	//

	bit_data = malloc(BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE);
	if (!bit_data) {
		fprintf(stderr, "#E Cannot allocate %i bytes for filebuf.\n",
		BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE);
		goto fail;
	}
	if (!(bitf = fopen(bit_path, "rb"))) {
		fprintf(stderr, "#E Error opening %s.\n", bit_path);
		goto fail;
        }
	bit_eof = 0;
	while (bit_eof < BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE) {
		size_t num_read = fread(&bit_data[bit_eof], sizeof(uint8_t),
			BITSTREAM_READ_PAGESIZE, bitf);
		bit_eof += num_read;
		if (num_read != BITSTREAM_READ_PAGESIZE)
			break;
	}
	fclose(bitf);
	if (bit_eof >= BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE) {
		fprintf(stderr, "#E Bitstream size above maximum of "
			"%i bytes.\n", BITSTREAM_READ_MAXPAGES *
			BITSTREAM_READ_PAGESIZE);
		goto fail;
	}

	//
	// header
	//

	printf("bit2txt_format 1\n");

	if (printf_header(bit_data, bit_eof, 0 /* inpos */, &bit_cur))
		goto fail;

	//
	// commands
	//

	if (bit_cur + 5 > bit_eof) goto fail_eof;
	if (bit_data[bit_cur] != 'e') {
		fprintf(stderr, "#E Expected string code 'e', got '%c'.\n",
			bit_data[bit_cur]);
		goto fail;
	}
	cmd_len = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur + 1]);
	bit_cur += 5;
	if (bit_cur + cmd_len > bit_eof) goto fail_eof;
	if (bit_cur + cmd_len < bit_eof) {
		printf("#W Unexpected continuation after offset "
			"%i.\n", bit_cur + 5 + cmd_len);
	}

	// hex-dump everything until 0xAA (sync word: 0xAA995566)
	if (bit_cur >= bit_eof) goto fail_eof;
	if (bit_data[bit_cur] != 0xAA) {
		printf("hex");
		while (bit_cur < bit_eof && bit_data[bit_cur] != 0xAA) {
			printf(" %.02x", bit_data[bit_cur]);
			bit_cur++; if (bit_cur >= bit_eof) goto fail_eof;
		}
		printf("\n");
	}
	if (bit_cur + 4 > bit_eof) goto fail_eof;
	if (g_cmd_info) printf("#I sync word at offset 0x%x.\n", bit_cur);
	u32 = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur]);
	bit_cur += 4;
	if (u32 != 0xAA995566) {
		fprintf(stderr, "#E Unexpected sync word 0x%x.\n", u32);
		goto fail;
	}
	printf("sync_word\n");

	try_full_map = !g_cmd_frames;
	first_FAR_off = -1;
	while (bit_cur < bit_eof) {
		// packet header: ug380, Configuration Packets (p88)
		if (g_cmd_info) printf("#I Packet header at off 0x%x.\n", bit_cur);

		if (bit_cur + 2 > bit_eof) goto fail_eof;
		u16 = __be16_to_cpu(*(uint16_t*)&bit_data[bit_cur]);
		u16_off = bit_cur; bit_cur += 2;

		// 3 bits: 001 = Type 1; 010 = Type 2
		packet_hdr_type = (u16 & 0xE000) >> 13;

		if (packet_hdr_type != 1 && packet_hdr_type != 2) {
			fprintf(stderr, "#E 0x%x=0x%x Unexpected packet type "
				"%u.\n", u16_off, u16, packet_hdr_type);
			goto fail;
		}

		// 2 bits: 00 = noop; 01 = read; 10 = write; 11 = reserved
		packet_hdr_opcode = (u16 & 0x1800) >> 11;
		if (packet_hdr_opcode == 3) {
			fprintf(stderr, "#E 0x%x=0x%x Unexpected packet opcode "
				"3.\n", u16_off, u16);
			goto fail;
		}
		if (packet_hdr_opcode == 0) { // noop
			if (packet_hdr_type != 1)
				printf("#W 0x%x=0x%x Unexpected packet"
					" type %u noop.\n", u16_off,
					u16, packet_hdr_type);
			if (u16 & 0x07FF)
				printf("#W 0x%x=0x%x Unexpected noop "
					"header.\n", u16_off, u16);
			if (packet_hdr_type != 1 || (u16&0x07FF))
				times = 1;
			else { // lookahead for more good noops
				i = bit_cur;
				while (i+1 < bit_eof) {
					u16 = __be16_to_cpu(*(uint16_t*)&bit_data[i]);
					if (((u16 & 0xE000) >> 13) != 1
					    || ((u16 & 0x1800) >> 11)
					    || (u16 & 0x7FF))
						break;
					i += 2;
				}
				times = 1 + (i - bit_cur)/2;
				if (times > 1)
					bit_cur += (times-1)*2;
			}
   			if (times > 1)
				printf("noop times %i\n", times);
			else
		 		printf("noop\n");
			continue;
		}

		// Now we must look at a Type 1 read or write command
		packet_hdr_register = (u16 & 0x07E0) >> 5;
		packet_hdr_wordcount = u16 & 0x001F;
		if (bit_cur + packet_hdr_wordcount*2 > bit_eof) goto fail_eof;
		bit_cur += packet_hdr_wordcount*2;

		if (packet_hdr_type == 1) {
			if (packet_hdr_register == IDCODE) {
				if (packet_hdr_wordcount != 2) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected IDCODE"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u32 = __be32_to_cpu(*(uint32_t*)&bit_data[u16_off+2]);
				for (i = 0; i < sizeof(idcodes)/sizeof(idcodes[0]); i++) {
					if ((u32 & 0x0FFFFFFF) == idcodes[i].code) {
						printf("T1 IDCODE %s\n", idcodes[i].name);
						m_idcode = i;
						break;
					}
				}
				if (i >= sizeof(idcodes)/sizeof(idcodes[0]))
					printf("#W Unknown IDCODE 0x%x.\n", u32);
				else if (u32 & 0xF0000000)
					printf("#W Unexpected revision bits in IDCODE 0x%x.\n", u32);
				if (idcodes[m_idcode].code == XC6SLX4
				    && m_FLR_value != 896)
					printf("#W Unexpected FLR value %i on "
					  "idcode %s.\n", m_FLR_value,
					  idcodes[m_idcode].name);
				continue;
			}
			if (packet_hdr_register == CMD) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected CMD"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				if (u16 >= sizeof(cmds) / sizeof(cmds[0])
				    || cmds[u16][0] == 0)
					printf("#W 0x%x=0x%x Unknown CMD.\n",
						u16_off, u16);
				else
					printf("T1 CMD %s\n", cmds[u16]);
				continue;
			}
			if (packet_hdr_register == FLR) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected FLR"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 FLR %u\n", u16);
				m_FLR_value = u16;
				if ((m_FLR_value*2) % 8)
					printf("#W FLR*2 should be multiple of "
					"8, but modulo 8 is %i\n", (m_FLR_value*2) % 8);
				// First come the type 0 frames (clb, bram
				// config, dsp, etc). Then type 1 (bram data),
				// then type 2, the IOB config data block.
				// FLR is counted in 16-bit words, and there is
				// 1 extra dummy 0x0000 after that.
				continue;
			}
			if (packet_hdr_register == COR1) {
				int unexpected_clk11 = 0;

				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected COR1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 COR1");
				if (u16 & 0x8000) {
					printf(" DRIVE_AWAKE");
					u16 &= ~0x8000;
				}
				if (u16 & 0x0010) {
					printf(" CRC_BYPASS");
					u16 &= ~0x0010;
				}
				if (u16 & 0x0008) {
					printf(" DONE_PIPE");
					u16 &= ~0x0008;
				}
				if (u16 & 0x0004) {
					printf(" DRIVE_DONE");
					u16 &= ~0x0004;
				}
				if (u16 & 0x0003) {
					if (u16 & 0x0002) {
						if (u16 & 0x0001)
							unexpected_clk11 = 1;
						printf(" SSCLKSRC=TCK");
					} else
						printf(" SSCLKSRC=UserClk");
					u16 &= ~0x0003;
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				if (unexpected_clk11)
					printf("#W Unexpected SSCLKSRC 11.\n");
				// Reserved bits 14:5 should be 0110111000
				// according to documentation.
				if (u16 != 0x3700)
					printf("#W Expected reserved 0x%x, got 0x%x.\n", 0x3700, u16);

				continue;
			}
			if (packet_hdr_register == COR2) {
				int unexpected_done_cycle = 0;
				int unexpected_lck_cycle = 0;
				unsigned cycle;

				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected COR2"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 COR2");
				if (u16 & 0x8000) {
					printf(" RESET_ON_ERROR");
					u16 &= ~0x8000;
				}

				// DONE_CYCLE
				cycle = (u16 & 0x0E00) >> 9;
				printf(" DONE_CYCLE=%s", bitstr(cycle, 3));
				if (!cycle || cycle == 7)
					unexpected_done_cycle = 1;
				u16 &= ~0x0E00;

				// LCK_CYCLE
				cycle = (u16 & 0x01C0) >> 6;
				printf(" LCK_CYCLE=%s", bitstr(cycle, 3));
				if (!cycle)
					unexpected_lck_cycle = 1;
				u16 &= ~0x01C0;

				// GTS_CYCLE
				cycle = (u16 & 0x0038) >> 3;
				printf(" GTS_CYCLE=%s", bitstr(cycle, 3));
				u16 &= ~0x0038;

				// GWE_CYCLE
				cycle = u16 & 0x0007;
				printf(" GWE_CYCLE=%s", bitstr(cycle, 3));
				u16 &= ~0x0007;

				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				if (unexpected_done_cycle)
					printf("#W Unexpected DONE_CYCLE %s.\n",
						bitstr((u16 & 0x01C0) >> 6, 3));
				if (unexpected_lck_cycle)
					printf("#W Unexpected LCK_CYCLE 0b000.\n");
				// Reserved bits 14:12 should be 000
				// according to documentation.
				if (u16)
					printf("#W Expected reserved 0, got 0x%x.\n", u16);
				continue;
			}
			if (packet_hdr_register == FAR_MAJ
			    && packet_hdr_wordcount == 2) {
				uint16_t maj, min;
				int unexpected_blk_bit4 = 0;

				if (first_FAR_off == -1)
					first_FAR_off = u16_off;

				maj = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				min = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+4]);
				printf("T1 FAR_MAJ");

				// BLK
				u16 = (maj & 0xF000) >> 12;
				printf(" BLK=%u", u16);
				if (u16 > 7)
					unexpected_blk_bit4 = 1;
				// ROW
				u16 = (maj & 0x0F00) >> 8;
				printf(" ROW=%u", u16);
				// MAJOR
				u16 = maj & 0x00FF;
				printf(" MAJOR=%u", u16);
				// Block RAM
				u16 = (min & 0xC000) >> 14;
				printf(" BRAM=%u", u16);
				// MINOR
				u16 = min & 0x03FF;
				printf(" MINOR=%u", u16);

				if (min & 0x3C00)
					printf(" 0x%x", min & 0x3C00);
				printf("\n");

				if (unexpected_blk_bit4)
					printf("#W Unexpected BLK bit 4 set.\n");
				// Reserved min bits 13:10 should be 000.
				if (min & 0x3C00)
					printf("#W Expected reserved 0, got 0x%x.\n", (min & 0x3C00) > 10);
				continue;
			}
			if (packet_hdr_register == MFWR) {
				uint32_t first_dword, second_dword;

				if (packet_hdr_wordcount != 4) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected "
						"MFWR wordcount %u.\n", u16_off,
						u16, packet_hdr_wordcount);
					goto fail;
				}
				first_dword = __be32_to_cpu(*(uint32_t*)&bit_data[u16_off+2]);
				second_dword = __be32_to_cpu(*(uint32_t*)&bit_data[u16_off+6]);
				if (first_dword || second_dword) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected "
						"MFWR data 0x%x 0x%x.\n", u16_off,
						u16, first_dword, second_dword);
					goto fail;
				}
				printf("T1 MFWR\n");
				continue;
			}
			if (packet_hdr_register == CTL) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected CTL"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 CTL");
				if (u16 & 0x0040) {
					printf(" DECRYPT");
					u16 &= ~0x0040;
				}
				if (u16 & 0x0020) {
					if (u16 & 0x0010)
						printf(" SBITS=NO_RW");
					else
						printf(" SBITS=NO_READ");
					u16 &= ~0x0030;
				} else if (u16 & 0x0010) {
					printf(" SBITS=ICAP_READ");
					u16 &= ~0x0010;
				}
				if (u16 & 0x0008) {
					printf(" PERSIST");
					u16 &= ~0x0008;
				}
				if (u16 & 0x0004) {
					printf(" USE_EFUSE_KEY");
					u16 &= ~0x0004;
				}
				if (u16 & 0x0002) {
					printf(" CRC_EXTSTAT_DISABLE");
					u16 &= ~0x0002;
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				// bit0 is reserved as 1, and we have seen
				// bit7 on as well.
				if (u16 != 0x81)
					printf("#W Expected reserved 0x%x, got 0x%x.\n", 0x0081, u16);
				continue;
			}
			if (packet_hdr_register == MASK) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected MASK"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 MASK");
				if (u16 & 0x0040) {
					printf(" DECRYPT");
					u16 &= ~0x0040;
				}
				if ((u16 & 0x0030) == 0x0030) {
					printf(" SECURITY");
					u16 &= ~0x0030;
				}
				if (u16 & 0x0008) {
					printf(" PERSIST");
					u16 &= ~0x0008;
				}
				if (u16 & 0x0004) {
					printf(" USE_EFUSE_KEY");
					u16 &= ~0x0004;
				}
				if (u16 & 0x0002) {
					printf(" CRC_EXTSTAT_DISABLE");
					u16 &= ~0x0002;
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				// It seems bit7 and bit0 are always masked in.
				if (u16 != 0x81)
					printf("#W Expected reserved 0x%x, got 0x%x.\n", 0x0081, u16);
				continue;
			}
			if (packet_hdr_register == PWRDN_REG) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected PWRDN_REG"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 PWRDN_REG");
				if (u16 & 0x4000) {
					printf(" EN_EYES");
					u16 &= ~0x4000;
				}
				if (u16 & 0x0020) {
					printf(" FILTER_B");
					u16 &= ~0x0020;
				}
				if (u16 & 0x0010) {
					printf(" EN_PGSR");
					u16 &= ~0x0010;
				}
				if (u16 & 0x0004) {
					printf(" EN_PWRDN");
					u16 &= ~0x0004;
				}
				if (u16 & 0x0001) {
					printf(" KEEP_SCLK");
					u16 &= ~0x0001;
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				// Reserved bits 13:6 should be 00100010
				// according to documentation.
				if (u16 != 0x0880)
					printf("#W Expected reserved 0x%x, got 0x%x.\n", 0x0880, u16);
				continue;
			}
			if (packet_hdr_register == HC_OPT_REG) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected HC_OPT_REG"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 HC_OPT_REG");
				if (u16 & 0x0040) {
					printf(" INIT_SKIP");
					u16 &= ~0x0040;
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				// Reserved bits 5:0 should be 011111
				// according to documentation.
				if (u16 != 0x001F)
					printf("#W Expected reserved 0x%x, got 0x%x.\n", 0x001F, u16);
				continue;
			}
			if (packet_hdr_register == PU_GWE) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected PU_GWE"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 PU_GWE 0x%03X\n", u16);
				continue;
			}
			if (packet_hdr_register == PU_GTS) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected PU_GTS"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 PU_GTS 0x%03X\n", u16);
				continue;
			}
			if (packet_hdr_register == CWDT) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected CWDT"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 CWDT 0x%X\n", u16);
				if (u16 < 0x0201)
					printf("#W Watchdog timer clock below"
					  " minimum value of 0x0201.\n");
				continue;
			}
			if (packet_hdr_register == MODE_REG) {
				int unexpected_buswidth = 0;

				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected MODE_REG"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 MODE_REG");
				if (u16 & (1<<13)) {
					printf(" NEW_MODE=BITSTREAM");
					u16 &= ~(1<<13);
				}
				if ((u16 & (1<<12))
				   && (u16 & (1<<11)))
					unexpected_buswidth = 1;
				else if (u16 & (1<<12)) {
					printf(" BUSWIDTH=4");
					u16 &= ~(1<<12);
				} else if (u16 & (1<<11)) {
					printf(" BUSWIDTH=2");
					u16 &= ~(1<<11);
				}
				// BUSWIDTH=1 is the default and not displayed

				if (u16 & (1<<9)) {
					printf(" BOOTMODE_1");
					u16 &= ~(1<<9);
				}
				if (u16 & (1<<8)) {
					printf(" BOOTMODE_0");
					u16 &= ~(1<<8);
				}

				if (unexpected_buswidth)
					printf("#W Unexpected bus width 0b11.\n");
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				if (u16)
					printf("#W Expected reserved 0, got 0x%x.\n", u16);
				continue;
			}
			if (packet_hdr_register == CCLK_FREQ) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected CCLK_FREQ"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 CCLK_FREQ");
				if (u16 & (1<<14)) {
					printf(" EXT_MCLK");
					u16 &= ~(1<<14);
				}
				printf(" MCLK_FREQ=0x%03X", u16 & 0x03FF);
				u16 &= ~(0x03FF);
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				if (u16)
					printf("#W Expected reserved 0, got 0x%x.\n", u16);
				continue;
			}
			if (packet_hdr_register == EYE_MASK) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected EYE_MASK"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 EYE_MASK 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == GENERAL1) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected GENERAL1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 GENERAL1 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == GENERAL2) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected GENERAL1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 GENERAL2 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == GENERAL3) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected GENERAL1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 GENERAL3 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == GENERAL4) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected GENERAL1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 GENERAL4 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == GENERAL5) {
				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected GENERAL1"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				printf("T1 GENERAL5 0x%X\n", u16);
				continue;
			}
			if (packet_hdr_register == EXP_SIGN) {
				if (packet_hdr_wordcount != 2) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected EXP_SIGN"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u32 = __be32_to_cpu(*(uint32_t*)&bit_data[u16_off+2]);
				u16_off+=4;
				printf("T1 EXP_SIGN 0x%X\n", u32);
				continue;
			}
			if (packet_hdr_register == SEU_OPT) {
				int seu_freq;

				if (packet_hdr_wordcount != 1) {
					fprintf(stderr, "#E 0x%x=0x%x Unexpected SEU_OPT"
						" wordcount %u.\n", u16_off,
					u16, packet_hdr_wordcount);
					goto fail;
				}
				u16 = __be16_to_cpu(*(uint16_t*)&bit_data[u16_off+2]);
				u16_off+=2;
				seu_freq = (u16 & 0x3FF0) >> 4;
				printf("T1 SEU_OPT SEU_FREQ=0x%X", seu_freq);
				u16 &= ~(0x3FF0);
				if (u16 & (1<<3)) {
					printf(" SEU_RUN_ON_ERR");
					u16 &= ~(1<<3);
				}
				if (u16 & (1<<1)) {
					printf(" GLUT_MASK");
					u16 &= ~(1<<1);
				}
				if (u16 & (1<<0)) {
					printf(" SEU_ENABLE");
					u16 &= ~(1<<0);
				}
				if (u16)
					printf(" 0x%x", u16);
				printf("\n");
				if (u16)
					printf("#W Expected reserved 0, got 0x%x.\n", u16);
				continue;
			}
			if (packet_hdr_register == CRC) {
				// Don't print CRC value for cleaner diff.
				printf("#W T1 CRC (%u words)\n",
					packet_hdr_wordcount);
				continue;
			}
			printf("#W 0x%x=0x%x T1 %i (%u words)", u16_off, u16,
				packet_hdr_register, packet_hdr_wordcount);
			for (i = 0; (i < 8) && (i < packet_hdr_wordcount); i++)
				printf(" 0x%x", __be16_to_cpu(*(uint16_t*)
					&bit_data[u16_off+2+i*2]));
			printf("\n");
			continue;
		}

		// packet type must be 2 here
		if (packet_hdr_wordcount != 0) {
			printf("#W 0x%x=0x%x Unexpected Type 2 "
				"wordcount.\n", u16_off, u16);
		}
		if (packet_hdr_register != FDRI) {
			fprintf(stderr, "#E 0x%x=0x%x Unexpected Type 2 "
				"register.\n", u16_off, u16);
			goto fail;
		}
		if (bit_cur + 4 > bit_eof) goto fail_eof;
		u32 = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur]);
		if (bit_cur+4+2*u32 > bit_eof) goto fail_eof;
		if (2*u32 < 130) {
			fprintf(stderr, "#E 0x%x=0x%x Unexpected Type2"
				" length %u.\n", u16_off, u16, 2*u32);
			goto fail;
		}

		printf("T2 FDRI words=%i\n", u32);
		bit_cur += 4;
		if (try_full_map) {
			try_full_map = 0;
			if (first_FAR_off != -1) {
				int outdelta;
				if (!full_map(bit_data, bit_eof, first_FAR_off,
				     &bits, &bits_len, idcodes[m_idcode].code,
				     m_FLR_value, &outdelta)) {
					printf_bits(bits, bits_len,
						idcodes[m_idcode].code);
					bit_cur = first_FAR_off + outdelta;
					continue;
				}
			}
		}

		num_frames = (2*u32)/130;
		i = 0;
		printf("\n");
		while (i < num_frames) {
			i += printf_frames(&bit_data[bit_cur+i*130],
				num_frames-i, -i /* row */, 0 /* major */,
				0 /* minor */, 1 /* print_empty */);
		}
		printf("\n");
		if (num_frames*130 < 2*u32) {
			int dump_len = 2*u32 - num_frames*130;
			printf("#D hexdump offset 0x%x, len 0x%x (%i)\n",
				num_frames*130, dump_len, dump_len);
			hexdump(1, &bit_data[bit_cur+num_frames*130], dump_len);
			printf("\n");
		}
		bit_cur += u32*2;

		if (bit_cur + 4 > bit_eof) goto fail_eof;
		u32 = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur]);
		if (g_cmd_info) printf("#I 0x%x=0x%x Ignoring Auto-CRC.\n", bit_cur, u32);
		bit_cur += 4;
	}
	free(bits);
	free(bit_data);
	return EXIT_SUCCESS;

fail_eof:
	fprintf(stderr, "#E Unexpected EOF.\n");
fail:
	free(bits);
	free(bit_data);
	return EXIT_FAILURE;
}
