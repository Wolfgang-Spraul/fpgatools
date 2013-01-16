//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "bit.h"

static int parse_header(struct fpga_config* config, uint8_t* d,
	int len, int inpos, int* outdelta);
static int parse_commands(struct fpga_config* config, uint8_t* d,
	int len, int inpos);

#define SYNC_WORD	0xAA995566

#define PACKET_HDR_TYPE_S	13
#define PACKET_HDR_OPCODE_S	11
#define PACKET_HDR_REG_S	 5

#define PACKET_TYPE_1		 1
#define PACKET_TYPE_2		 2

#define PACKET_HDR_OPCODE_NOOP   0
#define PACKET_HDR_OPCODE_READ   1
#define PACKET_HDR_OPCODE_WRITE  2
#define PACKET_HDR_OPCODE_RSRV   3

#define BITSTREAM_READ_PAGESIZE		4096

int read_bitfile(struct fpga_config* cfg, FILE* f)
{
	uint8_t* bit_data = 0;
	int rc, file_len, bit_len, bit_cur;

	memset(cfg, 0, sizeof(*cfg));
	cfg->num_regs_before_bits = -1;
	cfg->idcode_reg = -1;
	cfg->FLR_reg = -1;

	// read .bit into memory
	if (fseek(f, 0, SEEK_END) == -1)
		FAIL(errno);
	if ((file_len = ftell(f)) == -1)
		FAIL(errno);
	if (fseek(f, 0, SEEK_SET) == -1)
		FAIL(errno);
	if (!file_len)
		FAIL(EINVAL);

	if (!(bit_data = malloc(file_len)))
		FAIL(ENOMEM);
	bit_len = 0;
	while (bit_len < file_len) {
		size_t num_read = fread(&bit_data[bit_len], sizeof(uint8_t),
			BITSTREAM_READ_PAGESIZE, f);
		bit_len += num_read;
		if (num_read != BITSTREAM_READ_PAGESIZE)
			break;
	}
	if (bit_len != file_len)
		FAIL(EINVAL);

	// parse header and commands
	if ((rc = parse_header(cfg, bit_data, bit_len, /*inpos*/ 0, &bit_cur)))
		FAIL(rc);
	if ((rc = parse_commands(cfg, bit_data, bit_len, bit_cur)))
		FAIL(rc);

	free(bit_data);
	return 0;
fail:
	free(bit_data);
	return rc;
}

static void dump_header(struct fpga_config* cfg)
{
	int i;
	for (i = 0; i < sizeof(cfg->header_str)
			/sizeof(cfg->header_str[0]); i++) {
		printf("header_str_%c %s\n", 'a'+i,
			cfg->header_str[i]);
	}
}

static int dump_regs(struct fpga_config* cfg, int start, int end, int dump_crc)
{
	uint16_t u16;
	int i, rc;

	for (i = start; i < end; i++) {
		if (cfg->reg[i].reg == REG_NOOP) {
			int times = 1;
			while (i+times < end && cfg->reg[i+times].reg
					== REG_NOOP)
				times++;
			if (times > 1)
				printf("noop times %i\n", times);
			else
				printf("noop\n");
			i += times-1;
			continue;
		}
		if (cfg->reg[i].reg == IDCODE) {
			switch (cfg->reg[i].int_v & IDCODE_MASK) {
				case XC6SLX4:    printf("T1 IDCODE XC6SLX4\n"); break;
				case XC6SLX9:    printf("T1 IDCODE XC6SLX9\n"); break;
				case XC6SLX16:   printf("T1 IDCODE XC6SLX16\n"); break;
				case XC6SLX25:   printf("T1 IDCODE XC6SLX25\n"); break;
				case XC6SLX25T:  printf("T1 IDCODE XC6SLX25T\n"); break;
				case XC6SLX45:   printf("T1 IDCODE XC6SLX45\n"); break;
				case XC6SLX45T:  printf("T1 IDCODE XC6SLX45T\n"); break;
				case XC6SLX75:   printf("T1 IDCODE XC6SLX75\n"); break;
				case XC6SLX75T:  printf("T1 IDCODE XC6SLX75T\n"); break;
				case XC6SLX100:  printf("T1 IDCODE XC6SLX100\n"); break;
				case XC6SLX100T: printf("T1 IDCODE XC6SLX100T\n"); break;
				case XC6SLX150:  printf("T1 IDCODE XC6SLX150\n"); break;
				default:
					printf("#W Unknown IDCODE 0x%X.\n", cfg->reg[i].int_v);
					break;
			}
			continue;
		}
		if (cfg->reg[i].reg == CMD) {
			static const char* cmds[] =
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
			if (cfg->reg[i].int_v >= sizeof(cmds)/sizeof(cmds[0])
			    || cmds[cfg->reg[i].int_v] == 0)
				printf("#W Unknown CMD 0x%X.\n", cfg->reg[i].int_v);
			else
				printf("T1 CMD %s\n", cmds[cfg->reg[i].int_v]);
			continue;
		}
		if (cfg->reg[i].reg == FDRI) {
			printf("T2 FDRI %i\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == FLR) {
			printf("T1 FLR %i\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == CRC) {
			if (dump_crc)
				printf("T1 CRC 0x%X\n", cfg->reg[i].int_v);
			else
				printf("T1 CRC\n");
			continue;
		}
		if (cfg->reg[i].reg == COR1) {
			int unexpected_clk11 = 0;

			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == COR2) {
			int unexpected_done_cycle = 0;
			int unexpected_lck_cycle = 0;
			unsigned cycle;

			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == FAR_MAJ) {
			uint16_t maj, min;
			int unexpected_blk_bit4 = 0;

			maj = cfg->reg[i].far[FAR_MAJ_O];
			min = cfg->reg[i].far[FAR_MIN_O];
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
		if (cfg->reg[i].reg == MFWR) {
			printf("T1 MFWR\n");
			continue;
		}
		if (cfg->reg[i].reg == CTL) {
			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == MASK) {
			u16 = cfg->reg[i].int_v;
			printf("T1 MASK");
			if (u16 & 0x0040) {
				printf(" DECRYPT");
				u16 &= ~0x0040;
			}
			if ((u16 & MASK_SECURITY) == MASK_SECURITY) {
				printf(" SECURITY");
				u16 &= ~MASK_SECURITY;
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
		if (cfg->reg[i].reg == PWRDN_REG) {
			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == HC_OPT_REG) {
			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == PU_GWE) {
			printf("T1 PU_GWE 0x%03X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == PU_GTS) {
			printf("T1 PU_GTS 0x%03X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == CWDT) {
			printf("T1 CWDT 0x%X\n", cfg->reg[i].int_v);
			if (cfg->reg[i].int_v < 0x0201)
				printf("#W Watchdog timer clock below"
				  " minimum value of 0x0201.\n");
			continue;
		}
		if (cfg->reg[i].reg == MODE_REG) {
			int unexpected_buswidth = 0;

			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == CCLK_FREQ) {
			u16 = cfg->reg[i].int_v;
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
		if (cfg->reg[i].reg == EYE_MASK) {
			printf("T1 EYE_MASK 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == GENERAL1) {
			printf("T1 GENERAL1 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == GENERAL2) {
			printf("T1 GENERAL2 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == GENERAL3) {
			printf("T1 GENERAL3 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == GENERAL4) {
			printf("T1 GENERAL4 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == GENERAL5) {
			printf("T1 GENERAL5 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == EXP_SIGN) {
			printf("T1 EXP_SIGN 0x%X\n", cfg->reg[i].int_v);
			continue;
		}
		if (cfg->reg[i].reg == SEU_OPT) {
			int seu_freq;

			u16 = cfg->reg[i].int_v;
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
		FAIL(EINVAL);
	}
	return 0;
fail:
	return rc;
}

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

static void print_ramb16_cfg(ramb16_cfg_t* cfg)
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

static void printf_routing_2minors(const uint8_t* bits, int row, int major,
	int even_minor)
{
	int y, i, hclk;
	uint64_t u64_0, u64_1;
	char bit_str[129];

	bit_str[128] = 0;
	for (y = 0; y < 16; y++) {
		hclk = (y < 8) ? 0 : 2;
		u64_0 = frame_get_u64(bits + y*8 + hclk);
		u64_1 = frame_get_u64(bits + y*8 + hclk + FRAME_SIZE);
		if (u64_0 || u64_1) {
			for (i = 0; i < 128; i++)
				bit_str[i] = '0';
			for (i = 0; i < 64; i++) {
				if (u64_0 & (1ULL << i))
					bit_str[i*2] = '1';
				if (u64_1 & (1ULL << i))
					bit_str[i*2+1] = '1';
			}
			printf("r%i ma%i v64_%02i mip%02i %s\n",
				row, major, y, even_minor, bit_str);
		}
	}
}

static void printf_v64_mi20(const uint8_t* bits, int row, int major)
{
	int y, i, num_bits_on, hclk;
	uint64_t u64;
	char bit_str[65];

	bit_str[64] = 0;
	for (y = 0; y < 16; y++) {
		hclk = (y < 8) ? 0 : 2;
		u64 = frame_get_u64(bits + y*8 + hclk);
		if (u64) {
			for (i = 0; i < 64; i++)
				bit_str[i] = (u64 & (1ULL << i)) ? '1' : '0';
			printf("r%i ma%i v64_%02i mi20 %s\n",
				row, major, y, bit_str);
			num_bits_on = 0;
			for (i = 0; i < 64; i++) {
				if (u64 & (1ULL << i))
					num_bits_on++;
			}
			// this is most helpful for bits 24:39 which are
			// part of logic device configuration
			if (num_bits_on < 5) {
				for (i = 0; i < 64; i++) {
					if (!(u64 & (1ULL << i)))
						continue;
					printf("r%i ma%i v64_%02i mi20 b%i\n",
						row, major, y, i);
				}
			}
		}
	}
}

static void printf_lut(const uint8_t* bits, int row, int major,
	int minor, int v32_i)
{
	char bit_str[64];
	uint64_t u64;
	int i, num_bits_on;

	u64 = frame_get_lut64(&bits[minor*FRAME_SIZE], v32_i);
	if (u64) {
		num_bits_on = 0;
		for (i = 0; i < 64; i++) {
			if (u64 & (1ULL << i))
				num_bits_on++;
		}
		if (num_bits_on < 5) {
			printf("r%i ma%02i v32_%02i mip%02i_lut", row,
				major, v32_i, minor);
			for (i = 0; i < 64; i++) {
				if (u64 & (1ULL << i))
					printf(" b%i", i);
			}
			printf("\n");
		} else {
			for (i = 0; i < 64; i++)
				bit_str[i] = (u64 & (1ULL << i)) ? '1' : '0';
			printf("r%i ma%02i v32_%02i mip%02i_lut %.64s\n", row,
				major, v32_i, minor, bit_str);
		}
	}
}

static int dump_maj_zero(const uint8_t* bits, int row, int major)
{
	int minor;

	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);
	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_frames(&bits[minor*FRAME_SIZE], /*max_frames*/ 1,
			row, major, minor, /*print_empty*/ 0, /*no_clock*/ 1);
	return 0;
}

static int dump_maj_left(const uint8_t* bits, int row, int major)
{
	int minor;

	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);
	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_frames(&bits[minor*FRAME_SIZE], /*max_frames*/ 1,
			row, major, minor, /*print_empty*/ 0, /*no_clock*/ 1);
	return 0;
}

static int dump_maj_right(const uint8_t* bits, int row, int major)
{
	int minor;

	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);
	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_frames(&bits[minor*FRAME_SIZE], /*max_frames*/ 1,
			row, major, minor, /*print_empty*/ 0, /*no_clock*/ 1);
	return 0;
}

static int dump_maj_logic(const uint8_t* bits, int row, int major)
{
	const struct xc_die* xci = xc_die_info(XC6SLX9);
	int minor, i, logdev_start, logdev_end;

	for (minor = 0; minor < xci->majors[major].minors; minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);

	// 0:19 routing minor pairs
	for (i = 0; i < 10; i++)
		printf_routing_2minors(&bits[i*2*FRAME_SIZE], row, major, i*2);

	// mi20 as 64-char 0/1 string
	printf_v64_mi20(&bits[20*FRAME_SIZE], row, major);

	logdev_start = 0;
	logdev_end = 15;
	if (xci->majors[major].flags & XC_MAJ_TOP_BOT_IO) {
		if (row == xc_die_info(XC6SLX9)->num_rows-1)
			logdev_start += TOPBOT_IO_ROWS;
		else if (!row)
			logdev_end -= TOPBOT_IO_ROWS;
	}

	if (xci->majors[major].flags & XC_MAJ_XM) {
		if (xci->majors[major].minors != 31) HERE();

		// M devices
		if (logdev_start)
			printf_extrabits(bits, 21, 2, 0, logdev_start*64, row, major);
		for (i = logdev_start; i <= logdev_end; i++) {
			printf_lut(bits, row, major, 21, i*2);
			printf_lut(bits, row, major, 21, i*2+1);
		}
		if (logdev_end < 15)
			printf_extrabits(bits, 21, 2, i*64 + XC6_HCLK_BITS, (15-logdev_end)*64, row, major);
		printf_frames(&bits[23*FRAME_SIZE], /*max_frames*/ 1,
			row, major, 23, /*print_empty*/ 0, /*no_clock*/ 1);
		if (logdev_start)
			printf_extrabits(bits, 24, 2, 0, logdev_start*64, row, major);
		for (i = logdev_start; i <= logdev_end; i++) {
			printf_lut(bits, row, major, 24, i*2);
			printf_lut(bits, row, major, 24, i*2+1);
		}
		if (logdev_end < 15)
			printf_extrabits(bits, 24, 2, i*64 + XC6_HCLK_BITS, (15-logdev_end)*64, row, major);

		// X devices
		printf_frames(&bits[26*FRAME_SIZE], /*max_frames*/ 1,
			row, major, 26, /*print_empty*/ 0, /*no_clock*/ 1);
		if (logdev_start)
			printf_extrabits(bits, 27, 4, 0, logdev_start*64, row, major);
		for (i = logdev_start; i <= logdev_end; i++) {
			printf_lut(bits, row, major, 27, i*2);
			printf_lut(bits, row, major, 29, i*2);
			printf_lut(bits, row, major, 27, i*2+1);
			printf_lut(bits, row, major, 29, i*2+1);
		}
		if (logdev_end < 15)
			printf_extrabits(bits, 27, 4, i*64 + XC6_HCLK_BITS, (15-logdev_end)*64, row, major);
	} else if (xci->majors[major].flags & (XC_MAJ_XL|XC_MAJ_CENTER)) {

		// L devices
		if (logdev_start)
			printf_extrabits(bits, 21, 4, 0, logdev_start*64, row, major);
		for (i = logdev_start; i <= logdev_end; i++) {
			printf_lut(bits, row, major, 21, i*2);
			printf_lut(bits, row, major, 23, i*2);
			printf_lut(bits, row, major, 21, i*2+1);
			printf_lut(bits, row, major, 23, i*2+1);
		}
		if (logdev_end < 15)
			printf_extrabits(bits, 21, 4, i*64 + XC6_HCLK_BITS, (15-logdev_end)*64, row, major);
		printf_frames(&bits[25*FRAME_SIZE], /*max_frames*/ 1,
			row, major, 25, /*print_empty*/ 0, /*no_clock*/ 1);
		// X devices
		if (logdev_start)
			printf_extrabits(bits, 26, 4, 0, logdev_start*64, row, major);
		for (i = logdev_start; i <= logdev_end; i++) {
			printf_lut(bits, row, major, 26, i*2);
			printf_lut(bits, row, major, 28, i*2);
			printf_lut(bits, row, major, 26, i*2+1);
			printf_lut(bits, row, major, 28, i*2+1);
		}
		if (logdev_end < 15)
			printf_extrabits(bits, 26, 4, i*64 + XC6_HCLK_BITS, (15-logdev_end)*64, row, major);

		// one extra minor in the center major
		if (xci->majors[major].flags & XC_MAJ_CENTER) {
			if (xci->majors[major].minors != 31) HERE();
			printf_frames(&bits[30*FRAME_SIZE], /*max_frames*/ 1,
				row, major, 30, /*print_empty*/ 0, /*no_clock*/ 1);
		} else { // XL
			if (xci->majors[major].minors != 30) HERE();
		}
	} else
		HERE();

	return 0;
}

static int dump_maj_bram(const uint8_t* bits, int row, int major)
{
	ramb16_cfg_t ramb16_cfg[4];
	int minor, i, j, offset_in_frame;

	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);

	// 0:19 routing minor pairs
	for (i = 0; i < 10; i++)
		printf_routing_2minors(&bits[i*2*FRAME_SIZE], row, major, i*2);

	// mi20 as 64-char 0/1 string
	printf_v64_mi20(&bits[20*FRAME_SIZE], row, major);

	printf_frames(&bits[21*FRAME_SIZE], /*max_frames*/ 1,
		row, major, 21, /*print_empty*/ 0, /*no_clock*/ 1);
	printf_frames(&bits[22*FRAME_SIZE], /*max_frames*/ 1,
		row, major, 22, /*print_empty*/ 0, /*no_clock*/ 1);

	// minors 23&24
	for (i = 0; i < 4; i++) {
		offset_in_frame = i*32;
		if (offset_in_frame >= 64)
			offset_in_frame += 2;
		for (j = 0; j < 32; j++) {
			ramb16_cfg[i].byte[j] = bits[23*130+offset_in_frame+j];
			ramb16_cfg[i].byte[j+32] = bits[24*130+offset_in_frame+j];
		}
	}
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 64; j++) {
			if (ramb16_cfg[i].byte[j])
				break;
		}
		if (j >= 64)
			continue;
		printf("r%i ma%i ramb16 i%i\n",
			row, major, i);
		print_ramb16_cfg(&ramb16_cfg[i]);
	}
	return 0;
}

static int dump_maj_macc(const uint8_t* bits, int row, int major)
{
	int minor, i;

	for (minor = 0; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_clock(&bits[minor*FRAME_SIZE], row, major, minor);

	// 0:19 routing minor pairs
	for (i = 0; i < 10; i++)
		printf_routing_2minors(&bits[i*2*FRAME_SIZE], row, major, i*2);

	// mi20 as 64-char 0/1 string
	printf_v64_mi20(&bits[20*FRAME_SIZE], row, major);

	for (minor = 21; minor < get_major_minors(XC6SLX9, major); minor++)
		printf_frames(&bits[minor*FRAME_SIZE], /*max_frames*/ 1,
			row, major, minor, /*print_empty*/ 0, /*no_clock*/ 1);
	return 0;
}

static int dump_bits(struct fpga_config* cfg)
{
	int idcode, num_rows, row, major, off, rc;
	struct xc_die* die_info;

	if (cfg->idcode_reg == -1) FAIL(EINVAL);
	idcode = cfg->reg[cfg->idcode_reg].int_v;
	die_info = xc_die_info(idcode);
	if (!die_info) FAIL(EINVAL);
	num_rows = die_info->num_rows;

	// type0
	for (major = 0; major <= get_rightside_major(idcode); major++) {
		for (row = num_rows-1; row >= 0; row--) {
			off = (row*get_frames_per_row(idcode) + get_major_framestart(idcode, major)) * FRAME_SIZE;
			switch (get_major_type(idcode, major)) {
				case MAJ_ZERO:
					rc = dump_maj_zero(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				case MAJ_LEFT:
					rc = dump_maj_left(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				case MAJ_RIGHT:
					rc = dump_maj_right(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				case MAJ_LOGIC_XM:
				case MAJ_LOGIC_XL:
				case MAJ_CENTER:
					rc = dump_maj_logic(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				case MAJ_BRAM:
					rc = dump_maj_bram(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				case MAJ_MACC:
					rc = dump_maj_macc(&cfg->bits.d[off], row, major);
					if (rc) FAIL(rc);
					break;
				default: HERE(); break;
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int dump_bram(struct fpga_config* cfg)
{
	int row, i, j, off, newline;

	newline = 0;
	for (row = 0; row < 4; row++) {
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 18*130; j++) {
				if (cfg->bits.d[BRAM_DATA_START + row*144*130
					  + i*18*130 + j])
					break;
			}
			if (j >= 18*130)
				continue;
			if (!newline) {
				newline = 1;
				printf("\n");
			}
			printf("br%i ramb16 i%i\n", row, i);
			printf("{\n");
			off = BRAM_DATA_START + row*144*130 + i*18*130;
			printf_ramb16_data(cfg->bits.d, off);
			printf("}\n");
		}
	}
	return 0;
}

int dump_config(struct fpga_config* cfg, int flags)
{
	int rc;

	if (flags & DUMP_HEADER_STR)
		dump_header(cfg);
	if (flags & DUMP_REGS) {
		rc = dump_regs(cfg, /*start*/ 0, cfg->num_regs_before_bits, flags & DUMP_CRC);
		if (rc) FAIL(rc);
	}
	if (flags & DUMP_BITS) {
		rc = dump_bits(cfg);
		if (rc) FAIL(rc);
		rc = dump_bram(cfg);
		if (rc) FAIL(rc);
		printf_type2(cfg->bits.d, cfg->bits.len,
			BRAM_DATA_START + BRAM_DATA_LEN, 896*2/8);
		if (flags & DUMP_CRC)
			printf("auto-crc 0x%X\n", cfg->auto_crc);
	}
	if (flags & DUMP_REGS) {
		rc = dump_regs(cfg, cfg->num_regs_before_bits, cfg->num_regs, flags & DUMP_CRC);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

void free_config(struct fpga_config* cfg)
{
	free(cfg->bits.d);
	cfg->bits.d = 0;
	memset(cfg, 0, sizeof(*cfg));
}

static const uint8_t s_bit_bof[] = {
	0x00, 0x09, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0,
	0x0F, 0xF0, 0x00, 0x00, 0x01 };

static const uint8_t s_0xFF_words[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static int parse_header(struct fpga_config* cfg, uint8_t* d, int len,
	int inpos, int* outdelta)
{
	int i, str_len;

	*outdelta = 0;
	if (inpos + 13 > len) {
		fprintf(stderr, "#E File size %i below minimum of 13 bytes.\n",
			len);
		return -1;
	}
	for (i = 0; i < 13; i++) {
		if (d[inpos+*outdelta+i] == s_bit_bof[i])
			continue;
		fprintf(stderr, "#E Expected 0x%x, got 0x%x at off %i\n",
			s_bit_bof[i], d[inpos+*outdelta+i],
			inpos+*outdelta+i);
	}
	*outdelta += 13;
	// 4 strings 'a' - 'd', 16-bit length
	for (i = 'a'; i <= 'd'; i++) {
		if (inpos + *outdelta + 3 > len) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", len);
			return -1;
		}
		if (d[inpos + *outdelta] != i) {
			fprintf(stderr, "#E Expected string code '%c', got "
				"'%c'.\n", i, d[inpos + *outdelta]);
			return -1;
		}
		str_len = __be16_to_cpu(*(uint16_t*)&d[inpos + *outdelta + 1]);
		if (inpos + *outdelta + 3 + str_len > len) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", len);
			return -1;
		}
		if (d[inpos + *outdelta + 3 + str_len - 1]) {
			fprintf(stderr, "#E z-terminated string ends with %0xh"
				".\n", d[inpos + *outdelta + 3 + str_len - 1]);
			return -1;
		}
		strcpy(cfg->header_str[i-'a'], (char*) &d[inpos + *outdelta + 3]);
		*outdelta += 3 + str_len;
	}
	return 0;
}

static int FAR_pos(int FAR_row, int FAR_major, int FAR_minor)
{
	int result, i;

	if (FAR_row < 0 || FAR_major < 0 || FAR_minor < 0)
		return -1;
	if (FAR_row > 3 || FAR_major > 17
	    || FAR_minor >= get_major_minors(XC6SLX9, FAR_major))
		return -1;
	result = FAR_row * 505*130;
	for (i = 0; i < FAR_major; i++)
		result += get_major_minors(XC6SLX9, i)*130;
	return result + FAR_minor*130;
}

static int read_bits(struct fpga_config* cfg, uint8_t* d, int len,
	int inpos, int* outdelta)
{
	int src_off, packet_hdr_type, packet_hdr_opcode;
	int packet_hdr_register, packet_hdr_wordcount;
	int FAR_block, FAR_row, FAR_major, FAR_minor, i, j, rc, MFW_src_off;
	int offset_in_bits, block0_words, padding_frames, last_FDRI_pos;
	uint16_t u16;
	uint32_t u32;

	last_FDRI_pos = -1;
	*outdelta = 0;
	if (cfg->idcode_reg == -1 || cfg->FLR_reg == -1
	    || (cfg->reg[cfg->idcode_reg].int_v != XC6SLX4
	        && cfg->reg[cfg->idcode_reg].int_v != XC6SLX9)
	    || cfg->reg[cfg->FLR_reg].int_v != 896)
		FAIL(EINVAL);

	cfg->bits.len = (4*505 + 4*144) * 130 + 896*2;
	cfg->bits.d = calloc(cfg->bits.len, 1 /* elsize */);
	if (!cfg->bits.d) FAIL(ENOMEM);
	cfg->auto_crc = 0;

	FAR_block = -1;
	FAR_row = -1;
	FAR_major = -1;
	FAR_minor = -1;
	MFW_src_off = -1;
	// Go through bit_file from first_FAR_off (inpos) until last byte
	// of IOB was read, plus padding, plus CRC verification.
	src_off = inpos;
	while (src_off < len) {
		if (src_off + 2 > len) FAIL(EINVAL);
		u16 = __be16_to_cpu(*(uint16_t*)&d[src_off]);
		src_off += 2;

		// 3 bits: 001 = Type 1; 010 = Type 2
		packet_hdr_type = (u16 & 0xE000) >> 13;
		if (packet_hdr_type != 1 && packet_hdr_type != 2)
			FAIL(EINVAL);

		// 2 bits: 00 = noop; 01 = read; 10 = write; 11 = reserved
		packet_hdr_opcode = (u16 & 0x1800) >> 11;
		if (packet_hdr_opcode == 3) FAIL(EINVAL);

		if (packet_hdr_opcode == 0) { // noop
			if (packet_hdr_type != 1 || u16 & 0x07FF) FAIL(EINVAL);
			continue;
		}

		// Now we must look at a Type 1 command
		packet_hdr_register = (u16 & 0x07E0) >> 5;
		packet_hdr_wordcount = u16 & 0x001F;
		if (src_off + packet_hdr_wordcount*2 > len) FAIL(EINVAL);

		if (packet_hdr_type == 1) {
			if (packet_hdr_register == CMD) {
				if (packet_hdr_wordcount != 1) FAIL(EINVAL);
				u16 = __be16_to_cpu(
					*(uint16_t*)&d[src_off]);
				if (u16 == CMD_GRESTORE || u16 == CMD_LFRM) {
					if (last_FDRI_pos == -1) FAIL(EINVAL);
					src_off = last_FDRI_pos;
					goto success;
				}
				if (u16 != CMD_MFW && u16 != CMD_WCFG)
					FAIL(EINVAL);
				if (u16 == CMD_MFW) {
					if (FAR_block != 0) FAIL(EINVAL);
					MFW_src_off = FAR_pos(FAR_row, FAR_major, FAR_minor);
					if (MFW_src_off == -1) FAIL(EINVAL);
				}
				src_off += 2;
				continue;
			}
			if (packet_hdr_register == FAR_MAJ) {
				uint16_t maj, min;

				if (packet_hdr_wordcount != 2) FAIL(EINVAL);
				maj = __be16_to_cpu(*(uint16_t*)
					&d[src_off]);
				min = __be16_to_cpu(*(uint16_t*)
					&d[src_off+2]);

				FAR_block = (maj & 0xF000) >> 12;
				if (FAR_block > 7) FAIL(EINVAL);
				FAR_row = (maj & 0x0F00) >> 8;
				FAR_major = maj & 0x00FF;
				FAR_minor = min & 0x03FF;
				src_off += 4;
				continue;
			}
			if (packet_hdr_register == MFWR) {
				uint32_t first_dword, second_dword;

				if (packet_hdr_wordcount != 4) FAIL(EINVAL);
				first_dword = __be32_to_cpu(
					*(uint32_t*)&d[src_off]);
				second_dword = __be32_to_cpu(
					*(uint32_t*)&d[src_off+4]);
				if (first_dword || second_dword) FAIL(EINVAL);
				// The first MFWR will overwrite itself, so
				// use memmove().
				if (FAR_block != 0) FAIL(EINVAL);
				offset_in_bits = FAR_pos(FAR_row, FAR_major, FAR_minor);
				if (offset_in_bits == -1) FAIL(EINVAL);
				memmove(&cfg->bits.d[offset_in_bits], &cfg->bits.d[MFW_src_off], 130);
				   
				src_off += 8;
				continue;
			}
			FAIL(EINVAL);
		}

		// packet type must be 2 here
		if (packet_hdr_wordcount != 0) FAIL(EINVAL);
		if (packet_hdr_register != FDRI) FAIL(EINVAL);

		if (src_off + 4 > len) FAIL(EINVAL);
		u32 = __be32_to_cpu(*(uint32_t*)&d[src_off]);
		src_off += 4;
		if (src_off+2*u32 > len) FAIL(EINVAL);
		if (2*u32 < 130) FAIL(EINVAL);

		last_FDRI_pos = src_off+u32*2+/*auto-crc*/4;

		// fdri words u32
		if (FAR_block == -1 || FAR_block > 1 || FAR_row == -1
		    || FAR_major == -1 || FAR_minor == -1)
			FAIL(EINVAL);

		block0_words = 0;
		if (!FAR_block) {

			offset_in_bits = FAR_pos(FAR_row, FAR_major, FAR_minor);
			if (offset_in_bits == -1) FAIL(EINVAL);
			if (!FAR_row && !FAR_major && !FAR_minor
			    && u32 > 4*(505+2)*65)
				block0_words = 4*(505+2)*65;
			else {
				block0_words = u32;
				if (block0_words % 65) FAIL(EINVAL);
			}
			padding_frames = 0;
			for (i = 0; i < block0_words/65; i++) {
				if (i && i+1 == block0_words/65) {
					for (j = 0; j < 130; j++) {
						if (d[src_off+i*130+j]
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
						if (d[src_off+i*130+j]
						    != 0xFF) FAIL(EINVAL);
					}
					i++;
					padding_frames += 2;
					continue;
				}
				memcpy(&cfg->bits.d[offset_in_bits
					+ (i-padding_frames)*130],
					&d[src_off + i*130], 130);
			}
		}
		if (u32 - block0_words > 0) {
			int bram_data_words = 4*144*65 + 896;
			if (u32 - block0_words != bram_data_words + 1) FAIL(EINVAL);
			offset_in_bits = BRAM_DATA_START;
			memcpy(&cfg->bits.d[offset_in_bits],
				&d[src_off+block0_words*2],
				bram_data_words*2);
			u16 = __be16_to_cpu(*(uint16_t*)&d[
			  (src_off+block0_words+bram_data_words)*2]);
			if (u16) FAIL(EINVAL);
		}
		src_off += 2*u32;
		cfg->auto_crc = __be32_to_cpu(*(uint32_t*)&d[src_off]);
		src_off += 4;
	}
	rc = EINVAL;
fail:
	free(cfg->bits.d);
	cfg->bits.d = 0;
	return rc;
success:
	*outdelta = src_off - inpos;
	return 0;
}

static int parse_commands(struct fpga_config* cfg, uint8_t* d,
	int len, int inpos)
{
	int curpos, cmd_len, first_FAR_off, u16_off, rc;
	int packet_hdr_type, packet_hdr_opcode, packet_hdr_register;
	int packet_hdr_wordcount, i;
	uint32_t u32;
	uint16_t u16;

	curpos = inpos;
	if (curpos + 5 > len
	    || d[curpos] != 'e') FAIL(EINVAL);
	cmd_len = __be32_to_cpu(*(uint32_t*)&d[curpos + 1]);
	curpos += 5;
	if (curpos + cmd_len > len) FAIL(EINVAL);
	if (curpos + cmd_len < len) {
		printf("#W Unexpected continuation after offset "
			"%i (len %i).\n", curpos + cmd_len, len);
	}

	if (curpos >= len) FAIL(EINVAL);
	if (d[curpos] != 0xAA) {
		while (curpos < len && d[curpos] != 0xAA) {
			if (d[curpos] != 0xFF) {
				printf("#W Expected 0xFF, but got 0x%X at "
					"offset %i\n", d[curpos], curpos);
			}
			curpos++; if (curpos >= len) FAIL(EINVAL);
		}
	}
	if (curpos + 4 > len) FAIL(EINVAL);
	u32 = __be32_to_cpu(*(uint32_t*)&d[curpos]);
	curpos += 4;
	if (u32 != SYNC_WORD) {
		fprintf(stderr, "#E Unexpected sync word 0x%x.\n", u32);
		FAIL(EINVAL);
	}
	first_FAR_off = -1;
	while (curpos < len) {
		// packet header: ug380, Configuration Packets (p88)
		if (curpos + 2 > len) FAIL(EINVAL);
		u16 = __be16_to_cpu(*(uint16_t*)&d[curpos]);
		u16_off = curpos; curpos += 2;

		// 3 bits: 001 = Type 1; 010 = Type 2
		packet_hdr_type = (u16 & 0xE000) >> 13;
		if (packet_hdr_type != 1 && packet_hdr_type != 2) FAIL(EINVAL);

		// 2 bits: 00 = noop; 01 = read; 10 = write; 11 = reserved
		packet_hdr_opcode = (u16 & 0x1800) >> 11;
		if (packet_hdr_opcode == 3) FAIL(EINVAL);
		if (packet_hdr_opcode == 0) { // noop
			if (packet_hdr_type != 1 || u16 & 0x07FF) FAIL(EINVAL);
			cfg->reg[cfg->num_regs++].reg = REG_NOOP;
			continue;
		}

		packet_hdr_register = (u16 & 0x07E0) >> 5;
		packet_hdr_wordcount = u16 & 0x001F;
		if (curpos + packet_hdr_wordcount*2 > len) FAIL(EINVAL);
		curpos += packet_hdr_wordcount*2;

		if (packet_hdr_type == 2) {
			int outdelta;

			if (packet_hdr_wordcount != 0) {
				printf("#W 0x%x=0x%x Unexpected Type 2 "
					"wordcount.\n", u16_off, u16);
			}
			if (packet_hdr_register != FDRI) FAIL(EINVAL);

			// first FAR must be before FDRI, and we should only
			// execute the FDRI code here once.
			if (first_FAR_off == -1) FAIL(EINVAL);
			if (cfg->num_regs_before_bits != -1) FAIL(EINVAL);

			if (curpos + 4 > len) FAIL(EINVAL);
			u32 = __be32_to_cpu(*(uint32_t*)&d[curpos]);
			curpos += 4;
			if (curpos+2*u32 > len) FAIL(EINVAL);
			if (2*u32 < 130) FAIL(EINVAL);

			cfg->reg[cfg->num_regs].reg = FDRI;
			cfg->reg[cfg->num_regs].int_v = u32;
			cfg->num_regs++;
			cfg->num_regs_before_bits = cfg->num_regs;

			rc = read_bits(cfg, d, len, first_FAR_off, &outdelta);
			if (rc) FAIL(rc);
			curpos = first_FAR_off + outdelta;
			continue;
		}

		if (packet_hdr_type != 1) FAIL(EINVAL);
		if (packet_hdr_register == IDCODE) {
			if (packet_hdr_wordcount != 2) FAIL(EINVAL);

			if (cfg->idcode_reg != -1) FAIL(EINVAL);
			cfg->idcode_reg = cfg->num_regs;

			cfg->reg[cfg->num_regs].reg = IDCODE;
			cfg->reg[cfg->num_regs].int_v =
				__be32_to_cpu(*(uint32_t*)&d[u16_off+2]);
			cfg->num_regs++;

			if ((cfg->reg[cfg->idcode_reg].int_v == XC6SLX4
			     || cfg->reg[cfg->idcode_reg].int_v == XC6SLX9)
			    && cfg->reg[cfg->FLR_reg].int_v != 896)
				printf("#W Unexpected FLR value %i on "
					"idcode 0x%X.\n",
					cfg->reg[cfg->FLR_reg].int_v,
					cfg->reg[cfg->idcode_reg].int_v);
			continue;
		}
		if (packet_hdr_register == FLR) {
			if (packet_hdr_wordcount != 1) FAIL(EINVAL);

			if (cfg->FLR_reg != -1) FAIL(EINVAL);
			cfg->FLR_reg = cfg->num_regs;

			//
			// First come the type 0 frames (clb, bram
			// cfg, dsp, etc). Then type 1 (bram data),
			// then type 2, the IOB cfg data block.
			// FLR is counted in 16-bit words, and there is
			// 1 extra dummy 0x0000 after that.
			//

			cfg->reg[cfg->num_regs].reg = FLR;
			cfg->reg[cfg->num_regs].int_v =
				__be16_to_cpu(*(uint16_t*)&d[u16_off+2]);
			cfg->num_regs++;

			if ((cfg->reg[cfg->FLR_reg].int_v*2) % 8)
				printf("#W FLR*2 should be multiple of "
				"8, but modulo 8 is %i\n",
				(cfg->reg[cfg->FLR_reg].int_v*2) % 8);
			continue;
		}
		if (packet_hdr_register == FAR_MAJ) {
			if (packet_hdr_wordcount != 2) FAIL(EINVAL); 
			if (first_FAR_off == -1)
				first_FAR_off = u16_off;

			cfg->reg[cfg->num_regs].reg = FAR_MAJ;
			cfg->reg[cfg->num_regs].far[FAR_MAJ_O] =
				__be16_to_cpu(*(uint16_t*)&d[u16_off+2]);
			cfg->reg[cfg->num_regs].far[FAR_MIN_O] =
				 __be16_to_cpu(*(uint16_t*)&d[u16_off+4]);
			cfg->num_regs++;
			continue;
		}
		if (packet_hdr_register == MFWR) {
			uint32_t first_dword, second_dword;

			if (packet_hdr_wordcount != 4) FAIL(EINVAL);
			first_dword = __be32_to_cpu(*(uint32_t*)&d[u16_off+2]);
			second_dword = __be32_to_cpu(*(uint32_t*)&d[u16_off+6]);
			if (first_dword || second_dword) FAIL(EINVAL);

			cfg->reg[cfg->num_regs++].reg = MFWR;
			continue;
		}
		if (packet_hdr_register == CRC
		    || packet_hdr_register == EXP_SIGN) {
			if (packet_hdr_wordcount != 2) FAIL(EINVAL);

			cfg->reg[cfg->num_regs].reg = packet_hdr_register;
			cfg->reg[cfg->num_regs].int_v =
				__be32_to_cpu(*(uint32_t*)&d[u16_off+2]);
			cfg->num_regs++;
			continue;
		}
		if (packet_hdr_register == CMD
		    || packet_hdr_register == COR1
		    || packet_hdr_register == COR2
		    || packet_hdr_register == CTL
		    || packet_hdr_register == MASK
		    || packet_hdr_register == PWRDN_REG
		    || packet_hdr_register == HC_OPT_REG
		    || packet_hdr_register == PU_GWE
		    || packet_hdr_register == PU_GTS
		    || packet_hdr_register == CWDT
		    || packet_hdr_register == MODE_REG
		    || packet_hdr_register == CCLK_FREQ
		    || packet_hdr_register == EYE_MASK
		    || packet_hdr_register == GENERAL1
		    || packet_hdr_register == GENERAL2
		    || packet_hdr_register == GENERAL3
		    || packet_hdr_register == GENERAL4
		    || packet_hdr_register == GENERAL5
		    || packet_hdr_register == SEU_OPT) {
			if (packet_hdr_wordcount != 1) FAIL(EINVAL);

			cfg->reg[cfg->num_regs].reg = packet_hdr_register;
			cfg->reg[cfg->num_regs].int_v =
				__be16_to_cpu(*(uint16_t*)&d[u16_off+2]);
			cfg->num_regs++;
			continue;
		}
		printf("#W 0x%x=0x%x T1 %i (%u words)", u16_off, u16,
			packet_hdr_register, packet_hdr_wordcount);
		for (i = 0; (i < 8) && (i < packet_hdr_wordcount); i++)
			printf(" 0x%x", __be16_to_cpu(*(uint16_t*)
				&d[u16_off+2+i*2]));
		printf("\n");
	}
	return 0;
fail:
	return rc;
}

static int write_header_str(FILE* f, int code, const char* s)
{
	uint16_t be16_len;
	int s_len, nwritten, rc;

	// format:  8-bit code 'a' - 'd'
	//         16-bit string len, including '\0'
	//         z-terminated string
	if (fputc(code, f) == EOF) FAIL(errno);
	s_len = strlen(s)+1;
	be16_len = __cpu_to_be16(s_len);
	nwritten = fwrite(&be16_len, /*size*/ 1, sizeof(be16_len), f);
	if (nwritten != sizeof(be16_len)) FAIL(errno);
	nwritten = fwrite(s, /*size*/ 1, s_len, f);
	if (nwritten != s_len) FAIL(errno);
	return 0;
fail:
	return rc;
}

static int write_header(FILE* f, const char* str_a, const char* str_b, const char* str_c, const char* str_d)
{
	int nwritten, rc;

	nwritten = fwrite(s_bit_bof, /*size*/ 1, sizeof(s_bit_bof), f);
	if (nwritten != sizeof(s_bit_bof)) FAIL(errno);

	rc = write_header_str(f, 'a', str_a);
	if (rc) FAIL(rc);
	rc = write_header_str(f, 'b', str_b);
	if (rc) FAIL(rc);
	rc = write_header_str(f, 'c', str_c);
	if (rc) FAIL(rc);
	rc = write_header_str(f, 'd', str_d);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

static struct fpga_config_reg_rw s_defregs_before_bits[] =
	{{ CMD,		.int_v = CMD_RCRC },
	 { REG_NOOP },
	 { FLR,		.int_v = 896 }, 
	 { COR1,	.int_v = COR1_DEF | COR1_CRC_BYPASS }, 
	 { COR2,	.int_v = COR2_DEF }, 
	 { IDCODE,	.int_v = XC6SLX9 }, 
	 { MASK,	.int_v = MASK_DEF }, 
	 { CTL,		.int_v = CTL_DEF }, 
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP },
	 { CCLK_FREQ,	.int_v = CCLK_FREQ_DEF },
	 { PWRDN_REG,	.int_v = PWRDN_REG_DEF },
	 { EYE_MASK,	.int_v = EYE_MASK_DEF },
	 { HC_OPT_REG,	.int_v = HC_OPT_REG_DEF },
	 { CWDT,	.int_v = CWDT_DEF },
	 { PU_GWE,	.int_v = PU_GWE_DEF },
	 { PU_GTS,	.int_v = PU_GTS_DEF },
	 { MODE_REG,	.int_v = MODE_REG_DEF },
	 { GENERAL1,	.int_v = GENERAL1_DEF },
	 { GENERAL2,	.int_v = GENERAL2_DEF },
	 { GENERAL3,	.int_v = GENERAL3_DEF },
	 { GENERAL4,	.int_v = GENERAL4_DEF },
	 { GENERAL5,	.int_v = GENERAL5_DEF },
	 { SEU_OPT,	.int_v = SEU_OPT_DEF	},
	 { EXP_SIGN,	.int_v = EXP_SIGN_DEF },
	 { REG_NOOP }, { REG_NOOP },
	 { FAR_MAJ,	.far = { 0, 0 }},
	 { CMD, 	.int_v = CMD_WCFG }};

static struct fpga_config_reg_rw s_defregs_after_bits[] =
	{{ REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { CMD,		.int_v = CMD_GRESTORE },
	 { CMD,		.int_v = CMD_LFRM },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { CMD,		.int_v = CMD_GRESTORE },
	 { CMD,		.int_v = CMD_START },
	 { MASK,	.int_v = MASK_DEF | MASK_SECURITY }, 
	 { CTL,		.int_v = CTL_DEF }, 
	 { CRC,		.int_v = DEFAULT_AUTO_CRC },
	 { CMD,		.int_v = CMD_DESYNC },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }, { REG_NOOP }, { REG_NOOP },
	 { REG_NOOP }, { REG_NOOP }};

static int write_reg_action(FILE* f, const struct fpga_config_reg_rw* reg)
{
	uint16_t u16;
	int nwritten, i, rc;

	if (reg->reg == REG_NOOP) {
		u16 = __cpu_to_be16(1 << PACKET_HDR_TYPE_S);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);
		return 0;
	}
	if (reg->reg == MFWR) {
		u16 = PACKET_TYPE_1 << PACKET_HDR_TYPE_S;
		u16 |= PACKET_HDR_OPCODE_WRITE << PACKET_HDR_OPCODE_S;
		u16 |= reg->reg << PACKET_HDR_REG_S;
		u16 |= 4; // four 16-bit words

		u16 = __cpu_to_be16(u16);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);

		u16 = 0;
		for (i = 0; i < 4; i++) {
			nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
			if (nwritten != sizeof(u16)) FAIL(errno);
		}
		return 0;
	}
	if (reg->reg == FAR_MAJ) {
		u16 = PACKET_TYPE_1 << PACKET_HDR_TYPE_S;
		u16 |= PACKET_HDR_OPCODE_WRITE << PACKET_HDR_OPCODE_S;
		u16 |= reg->reg << PACKET_HDR_REG_S;
		u16 |= 2; // two 16-bit words

		u16 = __cpu_to_be16(u16);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);

		if (reg->far[FAR_MAJ_O] > 0xFFFF
		    || reg->far[FAR_MIN_O] > 0xFFF) FAIL(EINVAL);

		u16 = __cpu_to_be16(reg->far[FAR_MAJ_O]);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);

		u16 = __cpu_to_be16(reg->far[FAR_MIN_O]);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);
		return 0;
	}
	if (reg->reg == CRC || reg->reg == IDCODE || reg->reg == EXP_SIGN) {
		uint32_t u32;

		u16 = PACKET_TYPE_1 << PACKET_HDR_TYPE_S;
		u16 |= PACKET_HDR_OPCODE_WRITE << PACKET_HDR_OPCODE_S;
		u16 |= reg->reg << PACKET_HDR_REG_S;
		u16 |= 2; // two 16-bit words

		u16 = __cpu_to_be16(u16);
		nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
		if (nwritten != sizeof(u16)) FAIL(errno);

		u32 = __cpu_to_be32(reg->int_v);
		nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
		if (nwritten != sizeof(u32)) FAIL(errno);
		return 0;
	}
	static const int t1_oneword_regs[] =
		{ CMD, COR1, COR2, CTL, FLR, MASK, PWRDN_REG, HC_OPT_REG,
		  PU_GWE, PU_GTS, CWDT, MODE_REG, CCLK_FREQ, EYE_MASK,
		  GENERAL1, GENERAL2, GENERAL3, GENERAL4, GENERAL5,
		  SEU_OPT };
	for (i = 0; i < sizeof(t1_oneword_regs)/sizeof(t1_oneword_regs[0]); i++) {
		if (reg->reg == t1_oneword_regs[i])
			break;
	}
	if (i >= sizeof(t1_oneword_regs)/sizeof(t1_oneword_regs[0]))
		FAIL(EINVAL);
	
	u16 = PACKET_TYPE_1 << PACKET_HDR_TYPE_S;
	u16 |= PACKET_HDR_OPCODE_WRITE << PACKET_HDR_OPCODE_S;
	u16 |= reg->reg << PACKET_HDR_REG_S;
	u16 |= 1; // one word

	u16 = __cpu_to_be16(u16);
	nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
	if (nwritten != sizeof(u16)) FAIL(errno);

	if (reg->int_v > 0xFFFF) FAIL(EINVAL);
	u16 = __cpu_to_be16(reg->int_v);
	nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
	if (nwritten != sizeof(u16)) FAIL(errno);

	return 0;
fail:
	return rc;
}

static int write_bits(FILE* f, struct fpga_model* model)
{
	struct fpga_bits bits;
	uint16_t u16;
	uint32_t u32;
	int nwritten, i, j, rc;
	char padding_frame[FRAME_SIZE];

	RC_CHECK(model);
	bits.len = IOB_DATA_START + IOB_DATA_LEN;
	bits.d = calloc(bits.len, /*elsize*/ 1);
	if (!bits.d) FAIL(ENOMEM);

	rc = write_model(&bits, model);
	if (rc) FAIL(rc);

	u16 = PACKET_TYPE_2 << PACKET_HDR_TYPE_S;
	u16 |= PACKET_HDR_OPCODE_WRITE << PACKET_HDR_OPCODE_S;
	u16 |= FDRI << PACKET_HDR_REG_S;
	u16 |= 0; // zero 16-bit words

	u16 = __cpu_to_be16(u16);
	nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
	if (nwritten != sizeof(u16)) FAIL(errno);

	u32 = (FRAMES_DATA_LEN + NUM_ROWS*PADDING_FRAMES_PER_ROW*FRAME_SIZE
		+ BRAM_DATA_LEN + IOB_DATA_LEN)/2;
	u32++; // there is one extra 16-bit 0x0000 padding at the end
	u32 = __cpu_to_be32(u32);
	nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
	if (nwritten != sizeof(u32)) FAIL(errno);

	// initialize padding frame to 0xFF
	for (i = 0; i < FRAME_SIZE; i++)
		padding_frame[i] = 0xFF;

	// write rows with padding frames
	for (i = 0; i < NUM_ROWS; i++) {
		nwritten = fwrite(&bits.d[i*FRAMES_PER_ROW*FRAME_SIZE],
			/*size*/ 1, FRAMES_PER_ROW*FRAME_SIZE, f);
		if (nwritten != FRAMES_PER_ROW*FRAME_SIZE) FAIL(errno);
		for (j = 0; j < PADDING_FRAMES_PER_ROW; j++) {
			nwritten = fwrite(padding_frame,
				/*size*/ 1, FRAME_SIZE, f);
			if (nwritten != FRAME_SIZE) FAIL(errno);
		}
	}

	// write bram data
	nwritten = fwrite(&bits.d[BRAM_DATA_START],
		/*size*/ 1, BRAM_DATA_LEN, f);
	if (nwritten != BRAM_DATA_LEN) FAIL(errno);

	// write IOB data
	nwritten = fwrite(&bits.d[IOB_DATA_START],
		/*size*/ 1, IOB_DATA_LEN, f);
	if (nwritten != IOB_DATA_LEN) FAIL(errno);

	// write extra 0x0000 padding at end of FDRI block
	u16 = 0;
	nwritten = fwrite(&u16, /*size*/ 1, sizeof(u16), f);
	if (nwritten != sizeof(u16)) FAIL(errno);

	// todo: support real auto-crc calculation
	u32 = DEFAULT_AUTO_CRC;
	u32 = __cpu_to_be32(u32);
	nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
	if (nwritten != sizeof(u32)) FAIL(errno);

	free(bits.d);
	return 0;
fail:
	free(bits.d);
	return rc;
}

int write_bitfile(FILE* f, struct fpga_model* model)
{
	uint32_t u32;
	int len_to_eof_pos, eof_pos, nwritten, i, rc;

	RC_CHECK(model);
	rc = write_header(f, "fpgatools.fp;UserID=0xFFFFFFFF",
		"6slx9tqg144", "2010/05/26", "08:00:00");
	if (rc) FAIL(rc);
	if (fputc('e', f) == EOF) FAIL(errno);
	if ((len_to_eof_pos = ftell(f)) == -1)
		FAIL(errno);
	u32 = 0;
	nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
	if (nwritten != sizeof(u32)) FAIL(errno);

	nwritten = fwrite(s_0xFF_words, /*size*/ 1, sizeof(s_0xFF_words), f);
	if (nwritten != sizeof(s_0xFF_words)) FAIL(errno);

	u32 = __cpu_to_be32(SYNC_WORD);
	nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
	if (nwritten != sizeof(u32)) FAIL(errno);

	for (i = 0; i < sizeof(s_defregs_before_bits)/sizeof(s_defregs_before_bits[0]); i++) {
		rc = write_reg_action(f, &s_defregs_before_bits[i]);
		if (rc) FAIL(rc);
	}
	rc = write_bits(f, model);
	if (rc) FAIL(rc);
	for (i = 0; i < sizeof(s_defregs_after_bits)/sizeof(s_defregs_after_bits[0]); i++) {
		rc = write_reg_action(f, &s_defregs_after_bits[i]);
		if (rc) FAIL(rc);
	}

	// write len to eof at offset len_to_eof_pos
	if ((eof_pos = ftell(f)) == -1)
		FAIL(errno);
	if (fseek(f, len_to_eof_pos, SEEK_SET) == -1)
		FAIL(errno);
	u32 = __cpu_to_be32(eof_pos - len_to_eof_pos - sizeof(u32));
	nwritten = fwrite(&u32, /*size*/ 1, sizeof(u32), f);
	if (nwritten != sizeof(u32)) FAIL(errno);
	if (fseek(f, eof_pos, SEEK_SET) == -1)
		FAIL(errno);
	return 0;
fail:
	return rc;
}
