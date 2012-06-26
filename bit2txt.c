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
#include <sys/types.h>

#define PROGRAM_REVISION "2012-06-01"

// 120 MB max bitstream size is enough for now
#define BITSTREAM_READ_PAGESIZE		4096
#define BITSTREAM_READ_MAXPAGES		30000	// 120 MB max bitstream

__inline uint16_t __swab16(uint16_t x)
{
        return (((x & 0x00ffU) << 8) | \
            ((x & 0xff00U) >> 8)); \
}

__inline uint32_t __swab32(uint32_t x)
{
        return (((x & 0x000000ffUL) << 24) | \
            ((x & 0x0000ff00UL) << 8) | \
            ((x & 0x00ff0000UL) >> 8) | \
            ((x & 0xff000000UL) >> 24)); \
}

#define __be32_to_cpu(x) __swab32((uint32_t)(x))
#define __be16_to_cpu(x) __swab16((uint16_t)(x))
#define __cpu_to_be32(x) __swab32((uint32_t)(x))
#define __cpu_to_be16(x) __swab16((uint16_t)(x))

#define __le32_to_cpu(x) ((uint32_t)(x))
#define __le16_to_cpu(x) ((uint16_t)(x))
#define __cpu_to_le32(x) ((uint32_t)(x))
#define __cpu_to_le16(x) ((uint16_t)(x))

void help();

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

#define REG_R	0x01
#define REG_W	0x02
#define REG_RW	(REG_R | REG_W)

typedef struct
{
	char* name;
	int rw;		// REG_READ / REG_WRITE
} REG_INFO;

const REG_INFO xc6_regs[] =
{
	[CRC] = {"CRC", REG_W},
	[FAR_MAJ] = {"FAR_MAJ", REG_W},	// frame address register block and major
	[FAR_MIN] = {"FAR_MIN", REG_W},	// frame address register minor
	[FDRI] = {"FDRI", REG_W}, // frame data input
	[FDRO] = {"FDRO", REG_R}, // frame data output
	[CMD] = {"CMD", REG_RW}, // command
	[CTL] = {"CTL", REG_RW}, // control
	[MASK] = {"MASK", REG_RW}, // control mask
	[STAT] = {"STAT", REG_R}, // status
	[LOUT] = {"LOUT", REG_W}, // legacy output for serial daisy-chain
	[COR1] = {"COR1", REG_RW}, // configuration option 1
	[COR2] = {"COR2", REG_RW}, // configuration option 2
	[PWRDN_REG] = {"PWRDN_REG", REG_RW}, // power-down option register
	[FLR] = {"FLR", REG_W}, // frame length register
	[IDCODE] = {"IDCODE", REG_RW}, // product IDCODE
	[CWDT] = {"CWDT", REG_RW}, // configuration watchdog timer
	[HC_OPT_REG] = {"HC_OPT_REG", REG_RW}, // house clean option register
	[CSBO] = {"CSBO", REG_W}, // CSB output for parallel daisy-chaining
	[GENERAL1] = {"GENERAL1", REG_RW}, // power-up self test or loadable
					   // program addr
	[GENERAL2] = {"GENERAL2", REG_RW}, // power-up self test or loadable
					   // program addr and new SPI opcode
	[GENERAL3] = {"GENERAL3", REG_RW}, // golden bitstream address
	[GENERAL4] = {"GENERAL4", REG_RW}, // golden bitstream address and new
					   // SPI opcode
	[GENERAL5] = {"GENERAL5", REG_RW}, // user-defined register for
					   // fail-safe scheme
	[MODE_REG] = {"MODE_REG", REG_RW}, // reboot mode
	[PU_GWE] = {"PU_GWE", REG_W}, // GWE cycle during wake-up from suspend
	[PU_GTS] = {"PU_GTS", REG_W}, // GTS cycle during wake-up from suspend
	[MFWR] = {"MFWR", REG_W}, // multi-frame write register
	[CCLK_FREQ] = {"CCLK_FREQ", REG_W}, // CCLK frequency select for
					    // master mode
	[SEU_OPT] = {"SEU_OPT", REG_RW}, // SEU frequency, enable and status
	[EXP_SIGN] = {"EXP_SIGN", REG_RW}, // expected readback signature for
					   // SEU detect
	[RDBK_SIGN] = {"RDBK_SIGN", REG_W}, // readback signature for readback
					    // cmd and SEU
	[BOOTSTS] = {"BOOTSTS", REG_R}, // boot history register
	[EYE_MASK] = {"EYE_MASK", REG_RW}, // mask pins for multi-pin wake-up
	[CBC_REG] = {"CBC_REG", REG_W} // initial CBC value register 
};

// The highest 4 bits are the binary revision and not
// used when performing IDCODE verification.
// ug380, Configuration Sequence, page 78
typedef struct
{
	char* name;
	uint32_t code;
} IDCODE_S;

const IDCODE_S idcodes[] =
{
	{"XC6SLX4", 0x04000093},
	{"XC6SLX9", 0x04001093},
	{"XC6SLX16", 0x04002093},
	{"XC6SLX25", 0x04004093},
	{"XC6SLX25T", 0x04024093},
	{"XC6SLX45", 0x04008093},
	{"XC6SLX45T", 0x04028093},
	{"XC6SLX75", 0x0400E093},
	{"XC6SLX75T", 0x0402E093},
	{"XC6SLX100", 0x04011093},
	{"XC6SLX100T", 0x04031093},
	{"XC6SLX150", 0x0401D093}
};

// CMD register - ug380, page 92
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

typedef struct
{
	char* name;
	int minors;
} MAJOR;

const MAJOR majors[] =
{
	/*  0 */ { 0, 4 }, // clock?
	/*  1 */ { 0, 30 },
	/*  2 */ { "clb", 31 },
	/*  3 */ { 0, 30 },
	/*  4 */ { "bram", 25 },
	/*  5 */ { "clb", 31 },
	/*  6 */ { 0, 30 },
	/*  7 */ { 0, 24 }, // dsp?
	/*  8 */ { "clb", 31 },
	/*  9 */ { 0, 31 },
	/* 10 */ { "clb", 31 },
	/* 11 */ { 0, 30 },
	/* 12 */ { 0, 31 },
	/* 13 */ { 0, 30 },
	/* 14 */ { 0, 25 },
	/* 15 */ { "clb", 31 },
	/* 16 */ { 0, 30 },
	/* 17 */ { 0, 30 }
};

static const char* bitstr(uint32_t value, int digits)
{
	static char str[2 /* "0b" */ + 32 + 1 /* '\0' */];
	int i;

	str[0] = '0';
	str[1] = 'b';
	for (i = 0; i < digits; i++)
		str[digits-i+1] = (value & (1<<i)) ? '1' : '0';
	str[digits+2] = 0;
	return str;
}

void hexdump(int indent, const uint8_t* data, int len)
{
	int i, j;
	char fmt_str[16] = "%s@%05x %02x";
	char indent_str[16];

	if (indent > 15)
		indent = 15;
	for (i = 0; i < indent; i++)
		indent_str[i] = ' ';
	indent_str[i] = 0;

	i = 0;
	if (len <= 0x100)
		fmt_str[5] = '2';
	else if (len <= 0x10000)
		fmt_str[5] = '4';
	else
		fmt_str[5] = '6';

	while (i < len) {
		printf(fmt_str, indent_str, i, data[i]);
		for (j = 1; (j < 8) && (i + j < len); j++) {
			if (i + j >= len) break;
			printf(" %02x", data[i+j]);
		}
		printf("\n");
		i += 8;
	}
}

static const char* iob_xc6slx4_sitenames[896*2/8] =
{
	[0x0000/8] "P70",
		   "P69",
		   "P67",
		   "P66",
		   "P65",
		   "P64",
		   "P62",
		   "P61",
		   "P60",
		   "P59",
		   "P58",
		   "P57",
		   0,
		   0,
		   0,
		   0,
	[0x0080/8] 0,
		   0,
		   "P56",
		   "P55",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P51",
		   "P50",
		   0,
		   0,
		   0,
		   0,
	[0x0100/8] 0,
		   0,
		   0,
		   0,
		   "UNB131",
		   "UNB132",
		   "P48",
		   "P47",
		   "P46",
		   "P45",
		   "P44",
		   "P43",
		   0,
		   0,
		   "P41",
		   "P40",
	[0x0180/8] "P39",
		   "P38",
		   "P35",
		   "P34",
		   "P33",
		   "P32",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
	[0x0200/8] "P30",
		   "P29",
		   "P27",
		   "P26",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P24",
		   "P23",
		   "P22",
		   "P21",
		   0,
		   0,
	[0x0280/8] 0,
		   0,
		   0,
		   0,
		   "P17",
		   "P16",
		   "P15",
		   "P14",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
	[0x0300/8] "P12",
		   "P11",
		   "P10",
		   "P9",
		   "P8",
		   "P7",
		   "P6",
		   "P5",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P2",
		   "P1",
	[0x0380/8] "P144",
		   "P143",
		   "P142",
		   "P141",
		   "P140",
		   "P139",
		   "P138",
		   "P137",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
	[0x0400/8] 0,
		   0,
		   0,
		   0,
		   "P134",
		   "P133",
		   "P132",
		   "P131",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P127",
		   "P126",
	[0x0480/8] "P124",
		   "P123",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P121",
		   "P120",
		   "P119",
		   "P118",
		   "P117",
		   "P116",
		   "P115",
		   "P114",
	[0x0500/8] "P112",
		   "P111",
		   "P105",
		   "P104",
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P102",
		   "P101",
		   "P99",
		   "P98",
		   "P97",
	[0x0580/8] 0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P95",
		   "P94",
		   "P93",
		   "P92",
		   0,
		   0,
	[0x0600/8] 0,
		   0,
		   0,
		   "P88",
		   "P87",
		   0,
		   "P85",
		   "P84",
		   0,
		   0,
		   "P83",
		   "P82",
		   "P81",
		   "P80",
		   "P79",
		   "P78",
	[0x0680/8] 0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   0,
		   "P75",
		   "P74"
};

#define ATOM_MAX_BITS	32+1 // -1 signals end of array

typedef struct _cfg_atom
{
	int must_0[ATOM_MAX_BITS];
	int must_1[ATOM_MAX_BITS];
	const char* str;
	int flag; // used to remember a state such as 'found'
} cfg_atom_t;

int atom_found(char* bits, const cfg_atom_t* atom)
{
	int i;
	for (i = 0; atom->must_0[i] != -1; i++)
		if (bits[atom->must_0[i]])
			break;
	if (atom->must_0[i] != -1)
		return 0;
	for (i = 0; atom->must_1[i] != -1; i++)
		if (!bits[atom->must_1[i]])
			break;
	return atom->must_1[i] == -1;
}

void atom_remove(char* bits, const cfg_atom_t* atom)
{
	int i;
	for (i = 0; atom->must_1[i] != -1; i++) {
		if (bits[atom->must_1[i]])
			bits[atom->must_1[i]] = 0;
	}
}

typedef struct ramb16_cfg
{
	uint8_t byte[64];
} __attribute((packed)) ramb16_cfg_t;

static const cfg_atom_t ramb16_default =
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

	{ {          -1}, { 26,  27, -1}, "CLKAINV:CLKA" },
	{ {          -1}, { 30,  31, -1}, "CLKBINV:CLKB"  },
	{ {          -1}, {266, 267, -1}, "RST_PRIORITY_B_CE"  },
	{ {          -1}, {268, 269, -1}, "RST_PRIORITY_A_CE"  },
	{ {          -1}, {270, 271, -1}, "RSTTYPE_ASYNC"  },
	{ {          -1}, {278, 279, -1}, "WRITE_MODE_B_READ_FIRST"  },
	{ {          -1}, {280, 281, -1}, "WRITE_MODE_A_READ_FIRST"  },
	{ {          -1}, {282, 283, -1}, "WRITE_MODE_B_NO_CHANGE"  },
	{ {          -1}, {284, 285, -1}, "WRITE_MODE_A_NO_CHANGE"  },
	{ {          -1}, {290, 291, -1}, "EN_RSTRAM_A"  },
	{ {          -1}, {306, 307, -1}, "DOB_REG"  },
	{ {          -1}, {308, 309, -1}, "DOA_REG"  },
	{ {          -1}, {444, 445, -1}, "EN_RSTRAM_B" },
	{ {431, 467, -1}, {430, 466, -1}, "ENAINV:ENA" },
	{ {430, 431, 466, 467, -1}, {-1}, "ENAINV:ENA_B" },
	{ {465, 469, -1}, {464, 468, -1}, "ENBINV:ENB" },
	{ {464, 465, 468, 469, -1}, {-1}, "ENBINV:ENB_B" },
	{ {          -1}, { 20,  21, -1}, "REGCEAINV:REGCEA" },
	{ { 20,  21, -1}, {          -1}, "REGCEAINV:REGCEA_B" },
	{ {          -1}, {  8,   9, -1}, "REGCEBINV:REGCEB" },
	{ {  8,   9, -1}, {          -1}, "REGCEBINV:REGCEB_B" },
	{ { 24,  25, -1}, {          -1}, "RSTAINV:RSTA" },
	{ {          -1}, { 24,  25, -1}, "RSTAINV:RSTA_B" },
	{ {          -1}, {  4,   5, -1}, "RSTBINV:RSTB" },
	{ {  4,   5, -1}, {          -1}, "RSTBINV:RSTB_B" },
	{ {          -1}, {      19, -1}, "WEA0INV:WEA0" },
	{ {      19, -1}, {          -1}, "WEA0INV:WEA0_B" },
	{ {          -1}, {      23, -1}, "WEA2INV:WEA1" },
	{ {      23, -1}, {          -1}, "WEA2INV:WEA1_B" },
	{ {          -1}, {      18, -1}, "WEA2INV:WEA2" },
	{ {      18, -1}, {          -1}, "WEA2INV:WEA2_B" },
	{ {          -1}, {      22, -1}, "WEA2INV:WEA3" },
	{ {      22, -1}, {          -1}, "WEA2INV:WEA3_B" },
	{ {          -1}, {       7, -1}, "WEB0INV:WEB0" },
	{ {       7, -1}, {          -1}, "WEB0INV:WEB0_B" },
	{ {          -1}, {       3, -1}, "WEB1INV:WEB1" },
	{ {       3, -1}, {          -1}, "WEB1INV:WEB1_B" },
	{ {          -1}, {       6, -1}, "WEB2INV:WEB2" },
	{ {       6, -1}, {          -1}, "WEB2INV:WEB2_B" },
	{ {          -1}, {       2, -1}, "WEB3INV:WEB3" },
	{ {       2, -1}, {          -1}, "WEB3INV:WEB3_B" },
};

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
	if (ramb16_default.must_1[0] != -1) {
		if (atom_found(bits, &ramb16_default)) {
			printf("  default_bits\n");
			atom_remove(bits, &ramb16_default);
		} else
			printf("  #W Not all default bits set.\n");
	}
	for (i = 0; i < sizeof(ramb16_atoms)/sizeof(ramb16_atoms[0]); i++) {
		if (atom_found(bits, &ramb16_atoms[i])) {
			printf("  %s\n", ramb16_atoms[i].str);
			ramb16_atoms[i].flag = 1;
		} else
			ramb16_atoms[i].flag = 0;
	}
	for (i = 0; i < sizeof(ramb16_atoms)/sizeof(ramb16_atoms[0]); i++) {
		if (ramb16_atoms[i].flag)
			atom_remove(bits, &ramb16_atoms[i]);
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

// for an equivalent schematic, see lut.svg
const int lut_base_vars[6] = {0 /* A1 */, 1, 0, 0, 0, 1 /* A6 */};

int bool_nextlen(const char* expr, int len)
{
	int i, depth;

	if (!len) return -1;
	i = 0;
	if (expr[i] == '~') {
		i++;
		if (i >= len) return -1;
	}
	if (expr[i] == '(') {
		if (i+2 >= len) return -1;
		i++;
		for (depth = 1; depth && i < len; i++) {
			if (expr[i] == '(')
				depth++;
			else if (expr[i] == ')')
				depth--;
		}
		if (depth) return -1;
		return i;
	}
	if (expr[i] == 'A') {
		i++;
		if (i >= len) return -1;
		if (expr[i] < '1' || expr[i] > '6') return -1;
		return i+1;
	}
	return -1;
}

// + or, * and, @ xor, ~ not
// var must point to array of A1..A6 variables
int bool_eval(const char* expr, int len, const int* var)
{
	int i, negate, result, oplen;

	oplen = bool_nextlen(expr, len);
	if (oplen < 1) goto fail;
	i = 0;
	negate = 0;
	if (expr[i] == '~') {
		negate = 1;
		if (++i >= oplen) goto fail;
	}
	if (expr[i] == '(') {
		if (i+2 >= oplen) goto fail;
		result = bool_eval(&expr[i+1], oplen-i-2, var);
		if (result == -1) goto fail;
	} else if (expr[i] == 'A') {
		if (i+1 >= oplen) goto fail;
		if (expr[i+1] < '1' || expr[i+1] > '6')
			goto fail;
		result = var[expr[i+1]-'1'];
		if (oplen != i+2) goto fail;
	} else goto fail;
	if (negate) result = !result;
	i = oplen;
	while (i < len) {
		if (expr[i] == '+') {
			if (result) return 1;
			return bool_eval(&expr[i+1], len-i-1, var);
		}
		if (expr[i] == '@') {
			int right_side = bool_eval(&expr[i+1], len-i-1, var);
			if (right_side == -1) goto fail;
			return (result && !right_side) || (!result && right_side);
		}
		if (expr[i] != '*') goto fail;
		if (!result) break;
		if (++i >= len) goto fail;

		oplen = bool_nextlen(&expr[i], len-i);
		if (oplen < 1) goto fail;
		result = bool_eval(&expr[i], oplen, var);
		if (result == -1) goto fail;
		i += oplen;
	}
	return result;
fail:
	return -1;
}

int parse_boolexpr(const char* expr, uint64_t* lut)
{
	int i, j, result, vars[6];

	*lut = 0;
	for (i = 0; i < 64; i++) {
		memcpy(vars, lut_base_vars, sizeof(vars));
		for (j = 0; j < 6; j++) {
			if (i & (1<<j))
				vars[j] = !vars[j];
		}
		result = bool_eval(expr, strlen(expr), vars);
		if (result == -1) return -1;
		if (result) *lut |= 1LL<<i;
	}
	return 0;
}

void printf_lut6(const char* cfg)
{
	uint64_t lut;
	uint32_t first_major, second_major;
	int i;

	first_major = 0;
	second_major = 0;
	parse_boolexpr(cfg, &lut);

	for (i = 0; i < 16; i++) {
		if (lut & (1LL<<(i*4)))
			first_major |= 1<<(i*2);
		if (lut & (1LL<<(i*4+1)))
			first_major |= 1<<(i*2+1);
		if (lut & (1LL<<(i*4+2)))
			second_major |= 1<<(i*2);
		if (lut & (1LL<<(i*4+3)))
			second_major |= 1<<(i*2+1);
	}
	printf("first_major 0x%X second_major 0x%X\n", first_major, second_major);
}

typedef struct _minterm_entry
{
	char a[6]; // 0=A1, 5=A6. value can be 0, 1 or 2 for 'removed'
	int merged;
} minterm_entry;

// bits is tested only for 32 and 64
void lut2bool(const uint64_t lut, int bits, char* str)
{
	// round 0 needs 64 entries
	// round 1 (size2): 192
	// round 2 (size4): 240
	// round 3 (size8): 160
	// round 4 (size16): 60
	// round 5 (size32): 12
	// round 6 (size64): 1
	minterm_entry mt[7][256];
	int mt_size[7];
	int i, j, k, round, only_diff_bit;
	int str_end, first_op;

	memset(mt, 0, sizeof(mt));
	memset(mt_size, 0, sizeof(mt_size));

	for (i = 0; i < bits; i++) {
		if (lut & (1LL<<i)) {
			mt[0][mt_size[0]].a[0] = lut_base_vars[0];
			mt[0][mt_size[0]].a[1] = lut_base_vars[1];
			mt[0][mt_size[0]].a[2] = lut_base_vars[2];
			mt[0][mt_size[0]].a[3] = lut_base_vars[3];
			mt[0][mt_size[0]].a[4] = lut_base_vars[4];
			mt[0][mt_size[0]].a[5] = lut_base_vars[5];
			for (j = 0; j < 6; j++) {
				if (i & (1<<j))
					mt[0][mt_size[0]].a[j]
						= !mt[0][mt_size[0]].a[j];
			}
			mt_size[0]++;
		}
	}

	// special case: no minterms -> empty string
	if (mt_size[0] == 0) {
		str[0] = 0;
		return;
	}

	// go through five rounds of merging
	for (round = 1; round < 7; round++) {
		for (i = 0; i < mt_size[round-1]; i++) {
			for (j = i+1; j < mt_size[round-1]; j++) {
				only_diff_bit = -1;
				for (k = 0; k < 6; k++) {
					if (mt[round-1][i].a[k] != mt[round-1][j].a[k]) {
						if (only_diff_bit != -1) {
							only_diff_bit = -1;
							break;
						}
						only_diff_bit = k;
					}
				}
				if (only_diff_bit != -1) {
					char new_term[6];
	
					for (k = 0; k < 6; k++)
						new_term[k] =
						  (k == only_diff_bit) ? 2 
						    : mt[round-1][i].a[k];
					for (k = 0; k < mt_size[round]; k++) {
						if (new_term[0] == mt[round][k].a[0]
						    && new_term[1] == mt[round][k].a[1]
						    && new_term[2] == mt[round][k].a[2]
						    && new_term[3] == mt[round][k].a[3]
						    && new_term[4] == mt[round][k].a[4]
						    && new_term[5] == mt[round][k].a[5])
							break;
					}
					if (k >= mt_size[round]) {
						mt[round][mt_size[round]].a[0] = new_term[0];
						mt[round][mt_size[round]].a[1] = new_term[1];
						mt[round][mt_size[round]].a[2] = new_term[2];
						mt[round][mt_size[round]].a[3] = new_term[3];
						mt[round][mt_size[round]].a[4] = new_term[4];
						mt[round][mt_size[round]].a[5] = new_term[5];
						mt_size[round]++;
					}
					mt[round-1][i].merged = 1;
					mt[round-1][j].merged = 1;
				}
			}
		}
	}
	// special case: 222222 -> (A6+~A6)
	for (i = 0; i < mt_size[6]; i++) {
		if (mt[6][i].a[0] == 2
		    && mt[6][i].a[1] == 2
		    && mt[6][i].a[2] == 2
		    && mt[6][i].a[3] == 2
		    && mt[6][i].a[4] == 2
		    && mt[6][i].a[5] == 2) {
			strcpy(str, "A6+~A6");
			return;
		}
	}

	str_end = 0;
	for (round = 0; round < 7; round++) {
		for (i = 0; i < mt_size[round]; i++) {
			if (!mt[round][i].merged) {
				if (str_end)
					str[str_end++] = '+';
				first_op = 1;
				for (j = 0; j < 6; j++) {
					if (mt[round][i].a[j] != 2) {
						if (!first_op)
							str[str_end++] = '*';
						if (!mt[round][i].a[j])
							str[str_end++] = '~';
						str[str_end++] = 'A';
						str[str_end++] = '1' + j;
						first_op = 0;
					}
				}
			}
		}
	}
	str[str_end] = 0;
	// TODO: This could be further simplified, see Petrick's method.
	// XOR don't simplify well, try A2@A3
}


int g_FLR_value = -1;

int main(int argc, char** argv)
{
	uint8_t* bit_data = 0;
	FILE* bitf = 0;
	uint32_t bit_cur, bit_eof, cmd_len, u32, u16_off, bit_off;
	uint32_t last_processed_pos;
	uint16_t str_len, u16, packet_hdr_type, packet_hdr_opcode;
	uint16_t packet_hdr_register, packet_hdr_wordcount;
	int info = 0; // whether to print #I info messages (offsets and others)
	char* bit_path = 0;
	int i, j, k, l, num_frames, max_frames_to_scan, offset_in_frame, times;

	//
	// parse command line
	//

	if (argc < 2) {
		help();
		return EXIT_SUCCESS;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help")) {
			help();
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[i], "--version")) {
			printf("%s\n", PROGRAM_REVISION);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[i], "--info")) {
			info = 1;
		} else {
			bit_path = argv[i];
			if (argc > i+1) { // only 1 path supported
				help();
				return EXIT_FAILURE;
			}
		}
	}
	if (!bit_path) { // shouldn't get here, just in case
		help();
		return EXIT_FAILURE;
	}

	//
	// read .bit into memory
	//

	bit_data = malloc(BITSTREAM_READ_MAXPAGES * BITSTREAM_READ_PAGESIZE);
	if (!bit_data) {
		fprintf(stderr, "#E Cannot allocate %i bytes for bitstream.\n",
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

	// offset 0 - magic header
	if (bit_eof < 13) {
		fprintf(stderr, "#E File size %i below minimum of 13 bytes.\n",
			bit_eof);
		goto fail;
	}
	printf("hex");
	for (i = 0; i < 13; i++)
		printf(" %.02x", bit_data[i]);
	printf("\n");

	// 4 strings 'a' - 'd', 16-bit length
	bit_cur = 13;
	for (i = 'a'; i <= 'd'; i++) {
		if (bit_eof < bit_cur + 3) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", bit_eof);
			goto fail;
		}
		if (bit_data[bit_cur] != i) {
			fprintf(stderr, "#E Expected string code '%c', got "
				"'%c'.\n", i, bit_data[bit_cur]);
			goto fail;
		}
		str_len = __be16_to_cpu(*(uint16_t*)&bit_data[bit_cur + 1]);
		if (bit_eof < bit_cur + 3 + str_len) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", bit_eof);
			goto fail;
		}
		if (bit_data[bit_cur + 3 + str_len - 1] != 0) {
			fprintf(stderr, "#E z-terminated string ends with %0xh"
				".\n", bit_data[bit_cur + 3 + str_len - 1]);
			goto fail;
		}
		printf("header_str_%c %s\n", i, &bit_data[bit_cur + 3]);
		bit_cur += 3 + str_len;
	}

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
	if (info) printf("#I sync word at offset 0x%x.\n", bit_cur);
	u32 = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur]);
	bit_cur += 4;
	if (u32 != 0xAA995566) {
		fprintf(stderr, "#E Unexpected sync word 0x%x.\n", u32);
		goto fail;
	}
	printf("sync_word\n");

	while (bit_cur < bit_eof) {
		// 8 is 2 padding frames between each row and 2 at the end
		static const int type1_bram_data_start_frame = 4*505+8;
		static const int type2_iob_start_frame = 4*505+8 + 4*144;

		// packet header: ug380, Configuration Packets (p88)
		if (info) printf("#I Packet header at off 0x%x.\n", bit_cur);

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

		// Check whether register and r/w action on register
		// looks valid.

		if (packet_hdr_type == 1) {
			if (packet_hdr_register >= sizeof(xc6_regs)
				/ sizeof(xc6_regs[0])
			    || xc6_regs[packet_hdr_register].name[0] == 0) {
				printf("#W 0x%x=0x%x unknown T1 reg %u, "
					"skipping %d words.\n", u16_off, u16,
				packet_hdr_register, packet_hdr_wordcount);
				continue;
			}
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
						break;
					}
				}
				if (i >= sizeof(idcodes)/sizeof(idcodes[0]))
					printf("#W Unknown IDCODE 0x%x.\n", u32);
				else if (u32 & 0xF0000000)
					printf("#W Unexpected revision bits in IDCODE 0x%x.\n", u32);
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
				g_FLR_value = u16;
				if ((g_FLR_value*2) % 8)
					printf("#W FLR*2 should be multiple of "
					"8, but modulo 8 is %i\n", (g_FLR_value*2) % 8);
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
				// Reserved min bits 13:10 should be 000
				// according to documentation.
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
			printf("#W T1 %s (%u words)",
				xc6_regs[packet_hdr_register].name,
				packet_hdr_wordcount);
			if (packet_hdr_register == CRC) {
				printf("\n"); // omit CRC for diff beauty
				continue;
			}
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
		bit_cur += 4;

		printf("T2 FDRI\n");
		if (bit_cur + 2*u32 > bit_eof) goto fail_eof;
		if (2*u32 < 130) {
			fprintf(stderr, "#E 0x%x=0x%x Unexpected Type2"
				" length %u.\n", u16_off, u16, 2*u32);
			goto fail;
		}

		num_frames = (2*u32)/130;
		for (i = 0; i < num_frames; i++) {
			int first64_all_zero, last64_all_zero;
			uint8_t middle_byte0, middle_byte1;
			int count, cur_row, cur_major, cur_minor;

			if (i >= type1_bram_data_start_frame) break;

			if (i%(505+2) == 0 && info)
				printf("\n#D row %i\n", i/505);
			else if (!i)
				printf("\n");
			cur_row = i/(505+2);
			if (i%(505+2) == 505) {
				// If two frames of all one are following,
				// it's standard padding and we don't need
				// to display anything.
				if (i + 2 < num_frames) {
					for (j = 0; j < 2*130; j++) {
						if (bit_data[bit_cur+i*130+j]
							!= 0xFF)
							break;
					}
					if (j >= 2*130) {
						i++;
						continue;
					}
				}
				printf("\n#D padding\n");
			}

			cur_major = 0;
			cur_minor = 0;
			count = 0;
			for (j = 0; j < sizeof(majors)/sizeof(majors[0]); j++) {
				if (count + majors[j].minors > i%(505+2)) {
					cur_major = j;
					cur_minor = i%(505+2) - count;
					break;
				}
				count += majors[j].minors;
			}
			if (!cur_minor) {
				if (i+majors[cur_major].minors <= num_frames) {
					for (j = 0; j <
					  majors[cur_major].minors * 130; j++) {
						if (bit_data[bit_cur+i*130+j])
							break;
					}
					if (j >= majors[cur_major].minors*130) {
						if (majors[cur_major].minors > 1)
							i += majors[cur_major].minors - 1;
						continue;
					}
				}
				if (info) {
					printf("\n#D r%i major %i (%s) minors "
					  "%i\n", cur_row, cur_major,
					  majors[cur_major].name ?
					  majors[cur_major].name : "?",
					  majors[cur_major].minors);
				}
			}
			if (cur_major == 4 && cur_minor == 23
			    && i+1 < num_frames
			    && !(__be16_to_cpu(*(uint16_t*)&bit_data[bit_cur+i*130+64]))
			    && !(__be16_to_cpu(*(uint16_t*)&bit_data[bit_cur+i*130+130+64]))
			    && (cur_row > 0)) {
				ramb16_cfg_t ramb16_cfg;

				for (j = 0; j < 4; j++) {
					offset_in_frame = (3-j)*32;
					if (offset_in_frame >= 64)
						offset_in_frame += 2;

					for (k = 0; k < 32; k++) {
						ramb16_cfg.byte[k] = bit_data[bit_cur+i*130+offset_in_frame+k];
						ramb16_cfg.byte[k+32] = bit_data[bit_cur+i*130+130+offset_in_frame+k];
					}
					for (k = 0; k < 64; k++) {
						if (ramb16_cfg.byte[k])
							break;
					}
					if (k >= 64) continue; // empty
					printf("RAMB16_X0Y%i inst\n", ((cur_row-1)*4 + j)*2);
					print_ramb16_cfg(&ramb16_cfg);
				}
				i++; // we processed two frames
				continue;
			}

			middle_byte0 = bit_data[bit_cur+i*130+64];
			middle_byte1 = bit_data[bit_cur+i*130+65];
			first64_all_zero = 1;
			last64_all_zero = 1;

			for (j = 0; j < 64; j++) {
				if (bit_data[bit_cur+i*130+j] != 0) {
					first64_all_zero = 0;
					break;
				}
			}
			for (j = 66; j < 130; j++) {
				if (bit_data[bit_cur+i*130+j] != 0) {
					last64_all_zero = 0;
					break;
				}
			}

			if (!(first64_all_zero && !middle_byte0 &&
                              !middle_byte1 && last64_all_zero)) {
				for (j = 0; j < 16; j++) {
					if ((j < 8 && middle_byte1 & (1<<(7-j)))
					  || (j >= 8 && middle_byte0 & (1<<(7-j-8))))
						printf("cfg r%i m%i-%i/%i c%i ?\n",
							cur_row, cur_major,
							cur_minor,
					majors[cur_major].minors, j);
				}
				for (j = 0; j < 1024; j++) {
					int word_o, byte_o, bit_o;

					word_o = j / 16;
					if (word_o >= 64)
						word_o++;
					byte_o = !((j/8)%2);
					bit_o = 7-(j%8);

					if (bit_data[bit_cur+i*130+word_o*2+byte_o] & (1<<bit_o))
						printf("cfg r%i m%i-%i/%i b%i ?\n",
							cur_row, cur_major,
							cur_minor,
					majors[cur_major].minors, j);
				}
				if (info) {
				  printf("hex r%i m%i-%i/%i\n", cur_row,
					cur_major, cur_minor,
					majors[cur_major].minors);
				  hexdump(1, &bit_data[bit_cur+i*130], 130);
				}
				continue;
			}

			max_frames_to_scan = num_frames - i - 1;
			if (i + 1 + max_frames_to_scan >= ((i/507)+1)*507 - 2)
				max_frames_to_scan = ((i/507)+1)*507 - i - 3;
			count = 0;
			for (j = 0; j < sizeof(majors)/sizeof(majors[0]); j++) {
				if ((i/507)*507 + count > i
				    && (i/507)*507 + count <= i+1+max_frames_to_scan) {
					max_frames_to_scan = (i/507)*507 + count - i - 1;
					break;
				}
				count += majors[j].minors;
			}
			for (j = 0; j < max_frames_to_scan*130; j++) {
				if (bit_data[bit_cur+i*130+130+j] != 0)
					break;
			}
			if (j/130) {
				if (info)
					printf("frame_130 times %i all_0\n", 1+j/130);
				i += j/130;
				continue;
			}
			if (info)
				printf("frame_130 all_0\n");
			continue;
		}

		for (i = type1_bram_data_start_frame; i < num_frames; i++) {
			static const int ram_starts[] =
				{ 144, 162, 180, 198, 
				  288, 306, 324, 342,
				  432, 450, 468, 486 };

			if (i >= type2_iob_start_frame)
				break;
			if (i == type1_bram_data_start_frame) {
				if (info) printf("#D type 1 bram data start "
					"frame %i", i);
			}
			if (((i-type1_bram_data_start_frame) % 144) == 0
				&& info)
				printf("\n#D bram row %i\n", (i-type1_bram_data_start_frame)/144);

			for (j = 0; j < sizeof(ram_starts) / sizeof(ram_starts[0]); j++) {
				if (i == type1_bram_data_start_frame + ram_starts[j]
				    && num_frames >= i+18)
					break; 
			}
			if (j < sizeof(ram_starts) / sizeof(ram_starts[0])) {
				uint8_t init_byte;
				char init_str[65];
				int print_header = j*2;

				// We are at the beginning of a RAMB16 block
				// (or two RAMB8 blocks), and have the full
				// 18 frames available.
				// Verify that the first and last 18 bytes are
				// all 0. If not, hexdump them.
				for (j = 0; j < 18; j++) {
					if (bit_data[bit_cur+i*130+j] != 0)
						break;
				}
				if (j < 18) {
					if (print_header != -1) {
						printf("\nRAMB16_X0Y%i data\n",
							print_header);
						print_header = -1;
					}
					printf("ramb16_head");
					for (j = 0; j < 18; j++)
						printf(" %02x", bit_data[bit_cur+i*130+j]);
					printf("\n");
				}
				for (j = 0; j < 18; j++) {
					if (bit_data[bit_cur+(i+18)*130-18+j] != 0)
						break;
				}
				if (j < 18) {
					if (print_header != -1) {
						printf("\nRAMB16_X0Y%i data\n",
							print_header);
						print_header = -1;
					}
					printf("ramb16_tail");
					for (j = 0; j < 18; j++)
						printf(" %02x", bit_data[bit_cur+(i+18)*130-18+j]);
					printf("\n");
				}
				for (j = 0; j < 8; j++) { // 8 parity configs
					for (k = 0; k < 32; k++) { // 32 bytes per config
						init_byte = 0;
						for (l = 0; l < 8; l++) {
							bit_off = (j*(2048+256)) + (31-k)*4*18;
							bit_off += 1+(l/2)*18-(l&1);
							if (bit_data[bit_cur+i*130+18+bit_off/8] & (1<<(7-(bit_off%8))))
								init_byte |= 1<<l;
						}
						sprintf(&init_str[k*2], "%02x", init_byte);
					}
					for (k = 0; k < 64; k++) {
						if (init_str[k] != '0')
							break;
					}
					if (k < 64) {
						if (print_header != -1) {
							printf("\nRAMB16_X0Y%i "
							  "data\n",
							   print_header);
							print_header = -1;
						}
						printf("initp_%02i \"%s\"\n",
							j, init_str);
					}
				}
				for (j = 0; j < 32; j++) {
					for (k = 0; k < 32; k++) { // 32 bytes per config
						init_byte = 0;
						for (l = 0; l < 8; l++) {
							bit_off = (j*(2048+256)) + ((31-k)/2)*18 + (8-((31-k)&1)*8) + 2 + l;
							if (bit_data[bit_cur+i*130+18+bit_off/8] & (1<<(7-(bit_off%8))))
								init_byte |= 1<<(7-l);
						}
						sprintf(&init_str[k*2], "%02x", init_byte);
					}
					for (k = 0; k < 64; k++) {
						if (init_str[k] != '0')
							break;
					}
					if (k < 64) {
						if (print_header != -1) {
							printf("\nRAMB16_X0Y%i "
							  "data\n",
							   print_header);
							print_header = -1;
						}
						printf("init_%02i \"%s\"\n",
							j, init_str);
					}
				}
				i += 17; // 17 (+1) frames have been processed
				continue;
			}
			// everything from now on should be 0
			for (j = 0; j < 130; j++) {
				if (bit_data[bit_cur+i*130+j]) {
					printf("frame_130 %i frames into "
					  "content, file off 0x%xh (%i)\n",
					  i-type1_bram_data_start_frame,
					  bit_cur+i*130, bit_cur+i*130);
					hexdump(1, &bit_data[bit_cur+i*130],
						130);
					continue;
				}
			}

			// Check whether more all zero frames are following.
			// That way we can make the output more readable.
			max_frames_to_scan = num_frames - i - 1;
			if (max_frames_to_scan > type2_iob_start_frame - i - 1)
				max_frames_to_scan = type2_iob_start_frame - i - 1;

			for (j = 0; j < sizeof(ram_starts) / sizeof(ram_starts[0]); j++) {
				if ((type1_bram_data_start_frame + ram_starts[j] > i)
				    && (type1_bram_data_start_frame + ram_starts[j] <= i+1+max_frames_to_scan)) {
					max_frames_to_scan = type1_bram_data_start_frame+ram_starts[j] - i - 1;
					// ram_starts is sorted in ascending order
					break;
				}
			}
			for (j = 0; j < max_frames_to_scan*130; j++) {
				if (bit_data[bit_cur+i*130+130+j] != 0)
					break;
			}
			if (j/130) {
				if (info)
					printf("frame_130 times %i all_0\n",
						1+j/130);
				i += j/130;
				continue;
			}
			printf("frame_130 all_0\n");
			continue;
		}
		last_processed_pos = i*130;
		if (i == type2_iob_start_frame) {
			int iob_end_pos;

			printf("\n");
			if (info) printf("#D type 2 iob start frame %i\n", i);
			iob_end_pos = (type2_iob_start_frame*130 + (g_FLR_value+1)*2);

			if (g_FLR_value == -1)
				printf("#W No FLR value set, cannot process IOB block.\n");
			else if (iob_end_pos > 2*u32)
				printf("#W Expected %i bytes IOB data, only got %i.\n",
					(g_FLR_value+1)*2, 2*u32 - (g_FLR_value+1)*2);
			else {
				uint16_t post_iob_padding;

				if (g_FLR_value*2/8 != sizeof(iob_xc6slx4_sitenames)/sizeof(iob_xc6slx4_sitenames[0]))
					printf("#W Expected %li IOB entries but got %i.\n",
						sizeof(iob_xc6slx4_sitenames)/sizeof(iob_xc6slx4_sitenames[0]),
						g_FLR_value*2/8);
				for (j = 0; j < g_FLR_value*2/8; j++) {
					if (*(uint32_t*)&bit_data[bit_cur+type2_iob_start_frame*130+j*8]
					    || *(uint32_t*)&bit_data[bit_cur+type2_iob_start_frame*130+j*8+4]) {
						if (j < sizeof(iob_xc6slx4_sitenames)/sizeof(iob_xc6slx4_sitenames[0])
						    && iob_xc6slx4_sitenames[j]) {
							printf("iob %s", iob_xc6slx4_sitenames[j]);
						} else
							printf("iob %i", j);
						for (k = 0; k < 8; k++)
							printf(" %02X", bit_data[bit_cur+type2_iob_start_frame*130+j*8+k]);
						printf("\n");
					}
				}
				if (info) hexdump(1, &bit_data[bit_cur+type2_iob_start_frame*130], g_FLR_value*2);
				post_iob_padding = __be16_to_cpu(*(uint16_t*)&bit_data[bit_cur+type2_iob_start_frame*130+g_FLR_value*2]);
				if (post_iob_padding)
					printf("#W Unexpected post IOB padding 0x%x.\n", post_iob_padding);
				if (iob_end_pos < 2*u32)
					printf("#W Extra data after IOB.\n");
				printf("\n");
				last_processed_pos = iob_end_pos;
			}
		}

		if (last_processed_pos < 2*u32) {
			int dump_len = 2*u32 - last_processed_pos;
			printf("#D hexdump offset 0x%x, len 0x%x (%i)\n",
				last_processed_pos, dump_len, dump_len);
			hexdump(1, &bit_data[bit_cur+last_processed_pos], dump_len);
		}
		bit_cur += u32*2;

		if (bit_cur + 4 > bit_eof) goto fail_eof;
		u32 = __be32_to_cpu(*(uint32_t*)&bit_data[bit_cur]);
		if (info) printf("#I 0x%x=0x%x Ignoring Auto-CRC.\n", bit_cur, u32);
		bit_cur += 4;
	}
	free(bit_data);
	return EXIT_SUCCESS;

fail_eof:
	fprintf(stderr, "#E Unexpected EOF.\n");
fail:
	free(bit_data);
	return EXIT_FAILURE;
}

void help()
{
	printf("\n"
	       "bit2txt %s - convert FPGA bitstream to text\n"
	       "(c) 2012 Wolfgang Spraul\n"
	       "\n"
	       "bit2txt [options] <path to .bit file>\n"
	       "  --help                 print help message\n"
	       "  --version              print version number\n"
	       "  --info                 add extra info to output (marked #I)\n"
	       "  <path to .bit file>    bitstream to print on stdout\n"
	       "                         (proposing extension .b2t)\n"
	       "\n", PROGRAM_REVISION);
}
