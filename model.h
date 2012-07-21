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

// columns
//  'L' = X+L logic block
//  'l' = X+L logic block without IO at top and bottom
//  'M' = X+M logic block
//  'm' = X+M logic block without IO at top and bottom
//  'B' = block ram
//  'D' = dsp (macc)
//  'R' = registers and center IO/logic column
//
// wiring on the left and right side is described with 16
// characters for each row - 'W' = wired, 'U' = unwired
// order is top-down

#define XC6SLX9_ROWS	4
#define XC6SLX9_COLUMNS	"MLBMLDMRMlMLBML"
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
	int tile_x_range, tile_y_range;
	struct fpga_tile* tiles;
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

struct fpga_tile
{
	enum fpga_tile_type type;

	// expect up to 64 devices per tile
	int num_devices;
	struct fpga_device* devices;

	// expect up to 28k connections to other tiles per tile
	// 3*16 bit per connection:
	//   - x coordinate of other tile (16bit)
	//   - y coordinate of other tile (16bit)
	//   - endpoint index in other tile (16bit)
	int num_conns; // conns array is 3*num_conns 16-bit words
	uint16_t* conns; // num_conns*3 16-bit words: 16(x)-16(y)-16(endpoint)

	// expect up to 5k endpoints per tile
	// 16-bit index into conns (not yet multiplied by 3)
	int num_endpoints;
	uint16_t* endpoints;
	// endpoints0 are conceptual endpoints without outgoing wires.
	// Imagine their indices added to the end of num_endpoints, so
	// the first endpoint0 is at index num_endpoints, the second one
	// at num_endpoints+1, and so on.
	int num_endpoints0;
	// If != 0, endpoint_names will have
	// num_endpoints + num_endpoints0 entries.
	uint16_t* endpoint_names;

	// expect up to 4k connection pairs per tile
	// 32bit: 31    off: not in use      on: used
	//        30    off: unidirectional  on: bidirectional
	//        29:15 from, index into endpoints
	//        14:0  to, index into endpoints
	int num_connect_pairs;
	uint32_t* connect_pairs;
};

int fpga_build_model(struct fpga_model* model,
	int fpga_rows, const char* columns,
	const char* left_wiring, const char* right_wiring);
void fpga_free_model(struct fpga_model* model);

const char* fpga_tiletype_str(enum fpga_tile_type type);

unsigned long hash_djb2(const unsigned char* str);

// Strings are distributed among 1024 bins. Each bin
// is one continuous stream of zero-terminated strings
// prefixed with a 2*16-bit header. The allocation
// increment for each bin is 32k.
struct hashed_strarray
{
	uint32_t bin_offsets[256*256]; // min offset is 4, 0 means no entry
	uint16_t index_to_bin[256*256];
	char* bin_strings[1024];
	int bin_len[1024]; // points behind the last zero-termination
};

const char* strarray_lookup(struct hashed_strarray* array, uint16_t idx);
int strarray_find_or_add(struct hashed_strarray* array, const char* str,
	uint16_t* idx);
void strarray_init(struct hashed_strarray* array);
void strarray_free(struct hashed_strarray* array);
