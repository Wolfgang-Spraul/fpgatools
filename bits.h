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

// xc6 configuration registers, documentation in ug380, page90
enum fpga_config_reg {
	CRC = 0, FAR_MAJ, FAR_MIN, FDRI, FDRO, CMD, CTL, MASK, STAT, LOUT, COR1,
	COR2, PWRDN_REG, FLR, IDCODE, CWDT, HC_OPT_REG, CSBO = 18,
	GENERAL1, GENERAL2, GENERAL3, GENERAL4, GENERAL5, MODE_REG, PU_GWE,
	PU_GTS, MFWR, CCLK_FREQ, SEU_OPT, EXP_SIGN, RDBK_SIGN, BOOTSTS,
	EYE_MASK, CBC_REG
};

#define REG_NOOP -1 // pseudo register for noops

#define COR1_DEF		0x3D00
#define COR2_DEF		0x09EE

#define MASK_DEF		0xCF
#define MASK_SECURITY		0x0030

#define CTL_DEF			0x81
#define CCLK_FREQ_DEF		0x3CC8
#define PWRDN_REG_DEF		0x0881
#define EYE_MASK_DEF		0x0000
#define HC_OPT_REG_DEF		0x1F
#define CWDT_DEF		0xFFFF
#define PU_GWE_DEF		0x005
#define PU_GTS_DEF		0x004
#define MODE_REG_DEF		0x100
#define GENERAL1_DEF		0x0000
#define GENERAL2_DEF		0x0000
#define GENERAL3_DEF		0x0000
#define GENERAL4_DEF		0x0000
#define GENERAL5_DEF		0x0000
#define SEU_OPT_DEF		0x1BE2
#define EXP_SIGN_DEF		0

#define FAR_MAJ_O 0
#define FAR_MIN_O 1

struct fpga_config_reg_rw
{
	enum fpga_config_reg reg;
	union {
		int int_v;
		int far[2]; // 0 (FAR_MAJ_O) = major, 1 (FAR_MIN_O) = minor
	};
};

enum {
	CMD_NULL = 0, CMD_WCFG, CMD_MFW, CMD_LFRM, CMD_RCFG, CMD_START,
	CMD_RCRC = 7, CMD_AGHIGH, CMD_GRESTORE = 10, CMD_SHUTDOWN,
	CMD_DESYNC = 13, CMD_IPROG
};

#define MAX_HEADER_STR_LEN	128
#define MAX_REG_ACTIONS		256

#define BRAM_DATA_START		(4*505*130)
#define BRAM_DATA_LEN		(4*144*130)

struct fpga_config
{
	char header_str[4][MAX_HEADER_STR_LEN];

	int num_regs;
	struct fpga_config_reg_rw reg[MAX_REG_ACTIONS];
	// indices into reg (initialized to -1)
	int num_regs_before_bits;
	int idcode_reg;
	int FLR_reg;

	int bits_len;
	uint8_t* bits;
	int bram_off;
	int IOB_off;
};

int read_bitfile(struct fpga_config* cfg, FILE* f);
int extract_model(struct fpga_config* cfg, struct fpga_model* model);

#define DUMP_HEADER_STR		0x0001
#define DUMP_REGS		0x0002
#define DUMP_BITS		0x0004
int dump_config(struct fpga_config* cfg, int flags);

void free_config(struct fpga_config* cfg);

int write_bits(FILE* f, struct fpga_model* model);
