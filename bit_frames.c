//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "bit.h"
#include "parts.h"
#include "control.h"

#define HCLK_BYTES 2

static uint8_t* get_first_minor(struct fpga_bits* bits, int row, int major)
{
	int i, num_frames;

	num_frames = 0;
	for (i = 0; i < major; i++)
		num_frames += get_major_minors(XC6SLX9, i);
	return &bits->d[(row*FRAMES_PER_ROW + num_frames)*FRAME_SIZE];
}

static int get_bit(struct fpga_bits* bits,
	int row, int major, int minor, int bit_i)
{
	return frame_get_bit(get_first_minor(bits, row, major)
		+ minor*FRAME_SIZE, bit_i);
}

static void set_bit(struct fpga_bits* bits,
	int row, int major, int minor, int bit_i)
{
	return frame_set_bit(get_first_minor(bits, row, major)
		+ minor*FRAME_SIZE, bit_i);
}

static void clear_bit(struct fpga_bits* bits,
	int row, int major, int minor, int bit_i)
{
	return frame_clear_bit(get_first_minor(bits, row, major)
		+ minor*FRAME_SIZE, bit_i);
}

struct bit_pos
{
	int row;
	int major;
	int minor;
	int bit_i;
};

static int get_bitp(struct fpga_bits* bits, struct bit_pos* pos)
{
	return get_bit(bits, pos->row, pos->major, pos->minor, pos->bit_i);
}

static void set_bitp(struct fpga_bits* bits, struct bit_pos* pos)
{
	set_bit(bits, pos->row, pos->major, pos->minor, pos->bit_i);
}

static void clear_bitp(struct fpga_bits* bits, struct bit_pos* pos)
{
	clear_bit(bits, pos->row, pos->major, pos->minor, pos->bit_i);
}

static struct bit_pos s_default_bits[] = {
	{ 0, 0, 3, 66 },
	{ 0, 1, 23, 1034 },
	{ 0, 1, 23, 1035 },
	{ 0, 1, 23, 1039 },
	{ 2, 0, 3, 66 }};

// sw_bitpos is relative to a tile, i.e. major (x) and
// row/v64_i (y) are defined outside.
struct sw_bitpos
{
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
	swidx_t uni_dir;
	swidx_t rev_dir; // NO_SWITCH for unidirectional switches
};

struct sw_yxpos
{
	int y;
	int x;
	swidx_t idx;
};

#define MAX_YX_SWITCHES 1024

struct extract_state
{
	struct fpga_model* model;
	struct fpga_bits* bits;
	int num_bit_pos;
	struct sw_bitpos bit_pos[MAX_SWITCHBOX_SIZE];
	// yx switches are fully extracted ones pointing into the
	// model, stored here for later processing into nets.
	int num_yx_pos;
	struct sw_yxpos yx_pos[MAX_YX_SWITCHES]; // needs to be dynamically alloced...
};

static int extract_iobs(struct fpga_model* model, struct fpga_bits* bits)
{
	int i, num_iobs, iob_y, iob_x, iob_idx, dev_idx, rc;
	uint32_t* u32_p;
	const char* iob_sitename;
	struct fpga_device* dev;

	num_iobs = get_num_iobs(XC6SLX9);
	for (i = 0; i < num_iobs; i++) {
		u32_p = (uint32_t*) &bits->d[IOB_DATA_START + i*IOB_ENTRY_LEN];
		if (!u32_p[0] && !u32_p[1])
			continue;
		iob_sitename = get_iob_sitename(XC6SLX9, i);
		if (!iob_sitename) {
			HERE();
			continue;
		}
		rc = fpga_find_iob(model, iob_sitename, &iob_y, &iob_x, &iob_idx);
		if (rc) FAIL(rc);
		dev_idx = fpga_dev_idx(model, iob_y, iob_x, DEV_IOB, iob_idx);
		if (dev_idx == NO_DEV) FAIL(EINVAL);
		dev = FPGA_DEV(model, iob_y, iob_x, dev_idx);

		// we only support 2 hardcoded types of IOB right now
		// todo: bit 7 goes on when out-net connected?
		if ((u32_p[0] & 0xFFFFFF7F) == 0x00000100
		    && u32_p[1] == 0x06001100) {
			dev->instantiated = 1;
			strcpy(dev->u.iob.ostandard, IO_LVCMOS33);
			dev->u.iob.drive_strength = 12;
			dev->u.iob.O_used = 1;
			dev->u.iob.slew = SLEW_SLOW;
			dev->u.iob.suspend = SUSP_3STATE;
			u32_p[0] = 0;
			u32_p[1] = 0;
		} else if (u32_p[0] == 0x00000107
			   && u32_p[1] == 0x0B002400) {
			dev->instantiated = 1;
			strcpy(dev->u.iob.istandard, IO_LVCMOS33);
			dev->u.iob.bypass_mux = BYPASS_MUX_I;
			dev->u.iob.I_mux = IMUX_I;
			u32_p[0] = 0;
			u32_p[1] = 0;
		} else HERE();
	}
	return 0;
fail:
	return rc;
}

static int extract_logic(struct fpga_model* model, struct fpga_bits* bits)
{
	int dev_idx, row, row_pos, rc;
	int x, y, byte_off;
	uint8_t* u8_p;
	uint64_t u64;
	const char* lut_str;

	for (x = LEFT_SIDE_WIDTH; x < model->x_width-RIGHT_SIDE_WIDTH; x++) {
		if (!is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (!has_device_type(model, y, x, DEV_LOGIC, LOGIC_M))
				continue;
			row = which_row(y, model);
			row_pos = pos_in_row(y, model);
			if (row == -1 || row_pos == -1 || row_pos == 8) {
				HERE();
				continue;
			}
			if (row_pos > 8) row_pos--;
			u8_p = get_first_minor(bits, row, model->x_major[x]);
			byte_off = row_pos * 8;
			if (row_pos >= 8) byte_off += HCLK_BYTES;

			// M device
			dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGM);
			if (dev_idx == NO_DEV) FAIL(EINVAL);

			// A6_LUT
			if (frame_get_u32(u8_p + 24*FRAME_SIZE + byte_off + 4)
			    || frame_get_u32(u8_p + 25*FRAME_SIZE + byte_off + 4)) {
				u64 = read_lut64(u8_p + 24*FRAME_SIZE, (byte_off+4)*8);
				{ int logic_base[6] = {0,1,0,0,1,0};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 1); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGM,
						LUT_A, 6, lut_str, ZTERM);
					if (rc) FAIL(rc);
					*(uint32_t*)(u8_p+24*FRAME_SIZE+byte_off+4) = 0;
					*(uint32_t*)(u8_p+25*FRAME_SIZE+byte_off+4) = 0;
				}
			}
			// B6_LUT
			if (frame_get_u32(u8_p + 21*FRAME_SIZE + byte_off + 4)
			    || frame_get_u32(u8_p + 22*FRAME_SIZE + byte_off + 4)) {
				u64 = read_lut64(u8_p + 21*FRAME_SIZE, (byte_off+4)*8);
				{ int logic_base[6] = {1,1,0,1,0,1};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 1); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGM,
						LUT_B, 6, lut_str, ZTERM);
					if (rc) FAIL(rc);
					*(uint32_t*)(u8_p+21*FRAME_SIZE+byte_off+4) = 0;
					*(uint32_t*)(u8_p+22*FRAME_SIZE+byte_off+4) = 0;
				}
			}
			// C6_LUT
			if (frame_get_u32(u8_p + 24*FRAME_SIZE + byte_off)
			    || frame_get_u32(u8_p + 25*FRAME_SIZE + byte_off)) {
				u64 = read_lut64(u8_p + 24*FRAME_SIZE, byte_off*8);
				{ int logic_base[6] = {0,1,0,0,1,0};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 1); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGM,
						LUT_C, 6, lut_str, ZTERM);
					if (rc) FAIL(rc);
					*(uint32_t*)(u8_p+24*FRAME_SIZE+byte_off) = 0;
					*(uint32_t*)(u8_p+25*FRAME_SIZE+byte_off) = 0;
				}
			}
			// D6_LUT
			if (frame_get_u32(u8_p + 21*FRAME_SIZE + byte_off)
			    || frame_get_u32(u8_p + 22*FRAME_SIZE + byte_off)) {
				u64 = read_lut64(u8_p + 21*FRAME_SIZE, byte_off*8);
				{ int logic_base[6] = {1,1,0,1,0,1};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 1); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGM,
						LUT_D, 6, lut_str, ZTERM);
					if (rc) FAIL(rc);
					*(uint32_t*)(u8_p+21*FRAME_SIZE+byte_off) = 0;
					*(uint32_t*)(u8_p+22*FRAME_SIZE+byte_off) = 0;
				}
			}

			// X device
			u64 = frame_get_u64(u8_p + 26*FRAME_SIZE + byte_off);
			if ( u64 ) {
				// 21, 22, 36 and 37 are actually not default
				// and can go off with the FFMUXes or routing
				// say D over the FF to DQ etc. (AFFMUX=b37,
				// BFFMUX=b36, CFFMUX=b22, DFFMUX=b21).
				if (!(u64 & (1ULL<<1) && u64 & (1ULL<<2)
				      && u64 & (1ULL<<7) && u64 & (1ULL<<21)
				      && u64 & (1ULL<<22) && u64 & (1ULL<<36)
				      && u64 & (1ULL<<37) && u64 & (1ULL<<39))) {
					HERE();
					continue;
				}
				if (u64 & ~(0x000000B000600086ULL)) {
					HERE();
					continue;
				}

				dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGX);
				if (dev_idx == NO_DEV) FAIL(EINVAL);
				*(uint64_t*)(u8_p+26*FRAME_SIZE+byte_off) = 0;

				// A6_LUT
				u64 = read_lut64(u8_p + 27*FRAME_SIZE, (byte_off+4)*8);
				{ int logic_base[6] = {1,1,0,1,1,0};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 0); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGX,
						LUT_A, 6, lut_str, ZTERM);
					if (rc) FAIL(rc);
					*(uint32_t*)(u8_p+27*FRAME_SIZE+byte_off+4) = 0;
					*(uint32_t*)(u8_p+28*FRAME_SIZE+byte_off+4) = 0;
				}
				// B6_LUT
				u64 = read_lut64(u8_p + 29*FRAME_SIZE, (byte_off+4)*8);
				{ int logic_base[6] = {1,1,0,1,1,0};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 0); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGX,
						LUT_B, 6, lut_str, ZTERM);
					*(uint32_t*)(u8_p+29*FRAME_SIZE+byte_off+4) = 0;
					*(uint32_t*)(u8_p+30*FRAME_SIZE+byte_off+4) = 0;
				}
				// C6_LUT
				u64 = read_lut64(u8_p + 27*FRAME_SIZE, byte_off*8);
				{ int logic_base[6] = {0,1,0,0,0,1};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 0); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGX,
						LUT_C, 6, lut_str, ZTERM);
					*(uint32_t*)(u8_p+27*FRAME_SIZE+byte_off) = 0;
					*(uint32_t*)(u8_p+28*FRAME_SIZE+byte_off) = 0;
				}
				// D6_LUT
				u64 = read_lut64(u8_p + 29*FRAME_SIZE, byte_off*8);
				{ int logic_base[6] = {0,1,0,0,0,1};
				  lut_str = lut2bool(u64, 64, &logic_base, /*flip_b0*/ 0); }
				if (*lut_str) {
					rc = fdev_logic_set_lut(model, y, x, DEV_LOGX,
						LUT_D, 6, lut_str, ZTERM);
					*(uint32_t*)(u8_p+29*FRAME_SIZE+byte_off) = 0;
					*(uint32_t*)(u8_p+30*FRAME_SIZE+byte_off) = 0;
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int bitpos_is_set(struct extract_state* es, int y, int x,
	struct sw_bitpos* swpos, int* is_set)
{
	int row_num, row_pos, start_in_frame, two_bits_val, rc;

// TODO: does not support minor20 correctly
	*is_set = 0;
	is_in_row(es->model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	two_bits_val = 
		(get_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
			start_in_frame + swpos->two_bits_o/2) << 1)
		| (get_bit(es->bits, row_num, es->model->x_major[x], swpos->minor+1,
		    start_in_frame + swpos->two_bits_o/2) << 2);
	if (two_bits_val != swpos->two_bits_val)
		return 0;

	if (!get_bit(es->bits, row_num, es->model->x_major[x],
		swpos->minor + (swpos->one_bit_o&1),
		start_in_frame + swpos->one_bit_o/2))
		return 0;
	*is_set = 1;
	return 0;
fail:
	return rc;
}

static int bitpos_clear_bits(struct extract_state* es, int y, int x,
	struct sw_bitpos* swpos)
{
	int row_num, row_pos, start_in_frame, rc;

// TODO: does not support minor20 correctly
	is_in_row(es->model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
		start_in_frame + swpos->two_bits_o/2);
	clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor+ 1,
		start_in_frame + swpos->two_bits_o/2);
	clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor + (swpos->one_bit_o&1),
		start_in_frame + swpos->one_bit_o/2);

	return 0;
fail:
	return rc;
}

static int extract_routing_switches(struct extract_state* es, int y, int x)
{
	struct fpga_tile* tile;
	int i, is_set, rc;

	tile = YX_TILE(es->model, y, x);
if (y != 68 || x != 12) return 0;

	for (i = 0; i < es->num_bit_pos; i++) {
		rc = bitpos_is_set(es, y, x, &es->bit_pos[i], &is_set);
		if (rc) FAIL(rc);
		if (!is_set) continue;

		if (tile->switches[es->bit_pos[i].uni_dir] & SWITCH_BIDIRECTIONAL)
			HERE();
		if (tile->switches[es->bit_pos[i].uni_dir] & SWITCH_USED)
			HERE();
		if (es->num_yx_pos >= MAX_YX_SWITCHES)
			{ FAIL(ENOTSUP); }
		es->yx_pos[es->num_yx_pos].y = y;
		es->yx_pos[es->num_yx_pos].x = x;
		es->yx_pos[es->num_yx_pos].idx = es->bit_pos[i].uni_dir;
		es->num_yx_pos++;
		rc = bitpos_clear_bits(es, y, x, &es->bit_pos[i]);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int extract_switches(struct extract_state* es)
{
	int x, y, rc;

	// go through all tiles, look for one with switches
	// go through each switch, lookup device, is_enabled() -> enable
	for (x = 0; x < es->model->x_width; x++) {
		for (y = 0; y < es->model->y_height; y++) {
			if (is_atx(X_ROUTING_COL, es->model, x)
			    && y >= TOP_IO_TILES
			    && y < es->model->y_height-BOT_IO_TILES
			    && !is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					es->model, y)) {
				rc = extract_routing_switches(es, y, x);
				if (rc) FAIL(rc);
			}
#if 0
// when all switches are supported, we can turn this
// on to make the model more robust
			if (tile->num_switches)
				fprintf(stderr, "%i unsupported switches in y%02i x%02i\n",
					tile->num_switches, y, x);
#endif
		}
	}
	return 0;
fail:
	return rc;
}

#define LOGIN_ROW 2
#define LOGIN_MIP_ROWS 8
static const int logicin_matrix[] =
{
		  /*mip 12*/
	/* 000 */ LW + (LI_C6|LD1),	LW + LI_D6,
	/* 016 */ LW + (LI_B1|LD1),	LW + (LI_DI|LD1),
	/* 032 */ LW + (LI_C5|LD1),	LW + LI_D5,
	/* 048 */ LW + (LI_CI|LD1),	LW + LI_A2,
	/* 064 */ LW + (LI_C4|LD1),	LW + LI_D4,
	/* 080 */ LW + LI_A1,		LW + LI_CE,
	/* 096 */ LW + (LI_C3|LD1),	LW + LI_D3,
	/* 112 */ LW + (LI_B2|LD1),	LW + (LI_WE|LD1),

		  /*mip 14*/
	/* 000 */ LW + LI_C1,		LW + LI_DX,
	/* 016 */ LW + (LI_A3|LD1),	LW + LI_B3,
	/* 032 */ LW + (LI_CX|LD1),	LW + (LI_D2|LD1),
	/* 048 */ LW + (LI_A4|LD1),	LW + LI_B4,
	/* 064 */ LW + (LI_D1|LD1),	LW + LI_BX,
	/* 080 */ LW + (LI_A5|LD1),	LW + LI_B5,
	/* 096 */ LW + (LI_AX|LD1),	LW + LI_C2,
	/* 112 */ LW + (LI_A6|LD1),	LW + LI_B6,

		  /*mip 16*/
	/* 000 */ LW + (LI_B3|LD1),	LW + LI_A3,
	/* 016 */ LW + (LI_C2|LD1),	LW + (LI_DX|LD1),
	/* 032 */ LW + (LI_B4|LD1),	LW + LI_A4,
	/* 048 */ LW + LI_CX,		LW + LI_D1,
	/* 064 */ LW + (LI_B5|LD1),	LW + LI_A5,
	/* 080 */ LW + (LI_BX|LD1),	LW + LI_D2,
	/* 096 */ LW + (LI_B6|LD1),	LW + LI_A6,
	/* 112 */ LW + (LI_C1|LD1),	LW + LI_AX,

		  /*mip 18*/
	/* 000 */ LW + LI_B2,		FAN_B,
	/* 016 */ LW + (LI_D6|LD1),	LW + LI_C6,
	/* 032 */ LW + (LI_A1|LD1),	LW + (LI_CE|LD1),
	/* 048 */ LW + (LI_D5|LD1),	LW + LI_C5,
	/* 064 */ LW + (LI_A2|LD1),	LW + (LI_BI|LD1),
	/* 080 */ LW + (LI_D4|LD1),	LW + LI_C4,
	/* 096 */ LW + (LI_AI|LD1),	LW + LI_B1,
	/* 112 */ LW + (LI_D3|LD1),	LW + LI_C3
};

struct sw_mip_src
{
	int minor;

	int m0_sw_to;
	int m0_two_bits_o;
	int m0_two_bits_val;
	int m0_one_bit_start;

	int m1_sw_to;
	int m1_two_bits_o;
	int m1_two_bits_val;
	int m1_one_bit_start;

	int src_wire[6];
};

struct sw_mi20_src
{
	int sw_to;
	int two_bits_o;
	int two_bits_val;
	int one_bit_start;

	int src_wire[6];
};

static int add_bitpos(struct extract_state* es, int minor, int sw_to, int two_bits_o,
	int two_bits_val, int one_bit_o, int sw_from)
{
	// the first member of bidir switch pairs is where the bits reside
	static const int bidir[] = {
		LW + (LI_BX|LD1),	FAN_B,
		LW + (LI_AX|LD1),	GFAN0,
		LW + LI_AX,		GFAN0,
		LW + (LI_CE|LD1),	GFAN1,
		LW + (LI_CI|LD1),	GFAN1,
		LW + LI_BX,		LW + (LI_CI|LD1),
		LW + LI_BX,		LW + (LI_DI|LD1),
		LW + (LI_AX|LD1),	LW + (LI_CI|LD1),
		LW + (LI_BX|LD1),	LW + (LI_CE|LD1),
		LW + LI_AX,		LW + (LI_CE|LD1) };
	int i, rc;

	// bidirectional switches are ignored on one side, and
	// marked as bidir on the other side
	for (i = 0; i < sizeof(bidir)/sizeof(*bidir)/2; i++) {
		if (sw_from == bidir[i*2] && sw_to == bidir[i*2+1])
			// nothing to do where no bits reside
			return 0;
	}

	es->bit_pos[es->num_bit_pos].minor = minor,
	es->bit_pos[es->num_bit_pos].two_bits_o = two_bits_o;
	es->bit_pos[es->num_bit_pos].two_bits_val = two_bits_val;
	es->bit_pos[es->num_bit_pos].one_bit_o = one_bit_o;
	es->bit_pos[es->num_bit_pos].uni_dir = fpga_switch_lookup(es->model,
		es->model->first_routing_y, es->model->first_routing_x,
		fpga_wirestr_i(es->model, sw_from),
		fpga_wirestr_i(es->model, sw_to));
	if (es->bit_pos[es->num_bit_pos].uni_dir == NO_SWITCH) {
		fprintf(stderr, "#E routing switch %s -> %s not in model\n",
			fpga_wirestr(es->model, sw_from),
			fpga_wirestr(es->model, sw_to));
		FAIL(EINVAL);
	}

	es->bit_pos[es->num_bit_pos].rev_dir = NO_SWITCH;
	for (i = 0; i < sizeof(bidir)/sizeof(*bidir)/2; i++) {
		if (sw_from == bidir[i*2+1] && sw_to == bidir[i*2]) {
			es->bit_pos[es->num_bit_pos].rev_dir = fpga_switch_lookup(es->model,
				es->model->first_routing_y, es->model->first_routing_x,
				fpga_wirestr_i(es->model, sw_to),
				fpga_wirestr_i(es->model, sw_from));
			if (es->bit_pos[es->num_bit_pos].rev_dir == NO_SWITCH) {
				fprintf(stderr, "#E reverse routing switch %s -> %s not in model\n",
					fpga_wirestr(es->model, sw_to),
					fpga_wirestr(es->model, sw_from));
				FAIL(EINVAL);
			}
			break;
		}
	}
	es->num_bit_pos++;
	return 0;
fail:
	return rc;
}

static int src_to_bitpos(struct extract_state* es, const struct sw_mip_src* src, int src_len)
{
	int i, j, rc;

	for (i = 0; i < src_len; i++) {
		for (j = 0; j < sizeof(src->src_wire)/sizeof(src->src_wire[0]); j++) {
			if (src[i].src_wire[j] == NO_WIRE) continue;

			rc = add_bitpos(es, src[i].minor, src[i].m0_sw_to,
				src[i].m0_two_bits_o, src[i].m0_two_bits_val,
				src[i].m0_one_bit_start + j*2, src[i].src_wire[j]);
			if (rc) FAIL(rc);
			rc = add_bitpos(es, src[i].minor, src[i].m1_sw_to,
				src[i].m1_two_bits_o, src[i].m1_two_bits_val,
				src[i].m1_one_bit_start + j*2, src[i].src_wire[j]);
			if (rc) FAIL(rc);
		}
	}
	return 0;
fail:
	return rc;
}

static int wire_decrement(int wire)
{
	int _wire, flags;

	if (wire >= DW && wire <= DW_LAST) {
		_wire = wire - DW;
		flags = _wire & DIR_FLAGS;
		_wire &= ~DIR_FLAGS;

		if (_wire%4 == 0)
			return DW + ((_wire + 3) | flags);
		return DW + ((_wire - 1) | flags);
	}
	if (wire >= LW && wire <= LW_LAST) {
		_wire = wire - LW;
		flags = _wire & LD1;
		_wire &= ~LD1;

		if (_wire == LO_A)
			return LW + (LO_D|flags);
		if (_wire == LO_AMUX)
			return LW + (LO_DMUX|flags);
		if (_wire == LO_AQ)
			return LW + (LO_DQ|flags);
		if ((_wire >= LO_B && _wire <= LO_D)
		    || (_wire >= LO_BMUX && _wire <= LO_DMUX)
		    || (_wire >= LO_BQ && _wire <= LO_DQ))
			return LW + ((_wire-1)|flags);
	}
	if (wire == NO_WIRE) return wire;
	HERE();
	return wire;
}

static int mip_to_bitpos(struct extract_state* es, int minor, int m0_two_bits_val,
	int m0_one_bit_start, int m1_two_bits_val, int m1_one_bit_start, int (*src_wires)[8][6])
{
	struct sw_mip_src src;
	int i, j, rc;

	src.minor = minor;
	src.m0_two_bits_o = 0;
	src.m0_two_bits_val = m0_two_bits_val;
	src.m0_one_bit_start = m0_one_bit_start;
	src.m1_two_bits_o = 14;
	src.m1_two_bits_val = m1_two_bits_val;
	src.m1_one_bit_start = m1_one_bit_start;
	for (i = 0; i < 8; i++) {
		int logicin_o = ((src.minor-12)/2)*LOGIN_MIP_ROWS*LOGIN_ROW;
		logicin_o += i*LOGIN_ROW;
		src.m0_sw_to = logicin_matrix[logicin_o+0];
		src.m1_sw_to = logicin_matrix[logicin_o+1];
		if (i) {
			src.m0_two_bits_o += 16;
			src.m0_one_bit_start += 16;
			src.m1_two_bits_o += 16;
			src.m1_one_bit_start += 16;
		}
		for (j = 0; j < sizeof(src.src_wire)/sizeof(src.src_wire[0]); j++)
			src.src_wire[j] = (*src_wires)[i][j];

		rc = src_to_bitpos(es, &src, /*src_len*/ 1);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

static int construct_extract_state(struct extract_state* es, struct fpga_model* model)
{
	int i, j, k, rc;

	memset(es, 0, sizeof(*es));
	es->model = model;
	if (model->first_routing_y == -1)
		FAIL(EINVAL);

	// mip 0-10 (6*288=1728 switches)
	{ struct sw_mip_src src[] = {
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 2, 2,
			{DW + ((W_SW2*4+2)|DIR_N3), DW + ((W_SS2*4+2)|DIR_N3), DW + ((W_WW2*4+2)|DIR_N3),
			 DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_NW2*4+3}},
		{0, DW + ((W_WW4*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NW4*4+3) | DIR_BEG), 14, 0, 2,
			{DW + ((W_SW4*4+2)|DIR_N3), DW + ((W_SS4*4+2)|DIR_N3), DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_NW4*4+3, DW + W_WW4*4+3}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SW2*4+3, DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_SS2*4+3, DW + W_SE2*4+3, DW + W_EE2*4+3}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{0, DW + ((W_SS4*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SW4*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SW4*4+3, DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0),
			 DW + ((W_NW4*4+0)|DIR_S0), DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_EE2*4+3, DW + W_SE2*4+3, DW + ((W_WW2*4+2)|DIR_N3),
			 DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_NW2*4+3}},
		{2, DW + ((W_NN4*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NE4*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_NW4*4+3, DW + W_WW4*4+3}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SW2*4+3, DW + W_NN2*4+3, DW + W_NE2*4+3,
			 DW + W_SS2*4+3, DW + W_SE2*4+3, DW + W_EE2*4+3}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{2, DW + ((W_EE4*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SE4*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SW4*4+3, DW + W_SS4*4+3, DW + W_NN4*4+3,
			 DW + W_NE4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{4, DW + ((W_NW2*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NN2*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_NW4*4+3, DW + W_WW4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + ((W_WW2*4+2)|DIR_N3), DW + W_NW2*4+3}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + ((W_WR1*4+0)|DIR_S0),
			 DW + W_WL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{4, DW + ((W_WW2*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SW2*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0), DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0), DW + ((W_NW4*4+0)|DIR_S0)}},

		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 2, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 1, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_EL1*4+3,
			 DW + W_ER1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{6, DW + ((W_NE2*4+3) | DIR_BEG), 0, 0, 3,
		    DW + ((W_EE2*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 2, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + W_ER1*4+3,
			 DW + W_EL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 1, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{6, DW + ((W_SS2*4+3) | DIR_BEG), 16, 0, 18,
		    DW + ((W_SE2*4+3) | DIR_BEG), 30, 0, 19,
			{DW + W_SE2*4+3, DW + W_EE2*4+3, DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}},

		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 2, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX, LW + LO_DQ, LW + LO_D}},
		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 1, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{8, DW + ((W_WR1*4+0) | DIR_BEG), 0, 0, 3,
		    DW + ((W_NL1*4+2) | DIR_BEG), 14, 0, 2,
			{DW + W_NW4*4+3, DW + W_WW4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + ((W_WW2*4+2)|DIR_N3), DW + W_NW2*4+3}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 2, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + ((W_WR1*4+0)|DIR_S0),
			 DW + W_WL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 1, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 2, 19,
			{LW + LO_D, LW + LO_DQ, LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{8, DW + ((W_SR1*4+0) | DIR_BEG), 16, 0, 18,
		    DW + ((W_WL1*4+2) | DIR_BEG), 30, 0, 19,
			{DW + W_WW2*4+3, DW + ((W_NW2*4+0)|DIR_S0), DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + ((W_WW4*4+0)|DIR_S0), DW + ((W_NW4*4+0)|DIR_S0)}},

		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 2, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 1, 2,
			{LW + (LO_BMUX|LD1), LW + (LO_DQ|LD1), LW + (LO_D|LD1),
			 LW + LO_BMUX,	     LW + LO_DQ,       LW + LO_D}},
		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 1, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 2, 2,
			{DW + W_NE2*4+3, DW + W_NN2*4+3, DW + W_EL1*4+3,
			 DW + W_ER1*4+3, DW + W_NR1*4+3, DW + W_NL1*4+3}},
		{10, DW + ((W_EL1*4+2) | DIR_BEG), 0, 0, 3,
		     DW + ((W_NR1*4+3) | DIR_BEG), 14, 0, 2,
			{DW + W_EE4*4+3, DW + W_SE4*4+3, DW + W_NE4*4+3,
			 DW + W_NN4*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 2, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 1, 19,
			{DW + W_SR1*4+3, DW + W_SL1*4+3, DW + W_ER1*4+3,
			 DW + W_EL1*4+3, DW + W_SW2*4+3, DW + W_SS2*4+3}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 1, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 2, 19,
			{LW + LO_D,       LW + LO_DQ,       LW + LO_BMUX,
			 LW + (LO_D|LD1), LW + (LO_DQ|LD1), LW + (LO_BMUX|LD1)}},
		{10, DW + ((W_SL1*4+3) | DIR_BEG), 16, 0, 18,
		     DW + ((W_ER1*4+0) | DIR_BEG), 30, 0, 19,
			{DW + W_SE2*4+3, DW + W_EE2*4+3, DW + W_SW4*4+3,
			 DW + W_SS4*4+3, DW + W_SE4*4+3, DW + W_EE4*4+3}}};

		for (i = 0;; i++) {
			rc = src_to_bitpos(es, src, sizeof(src)/sizeof(*src));
			if (rc) FAIL(rc);
			if (i >= 3) break;
			for (j = 0; j < sizeof(src)/sizeof(*src); j++) {
				src[j].m0_sw_to = wire_decrement(src[j].m0_sw_to);
				src[j].m0_two_bits_o += 32;
				src[j].m0_one_bit_start += 32;
				src[j].m1_sw_to = wire_decrement(src[j].m1_sw_to);
				src[j].m1_two_bits_o += 32;
				src[j].m1_one_bit_start += 32;
				for (k = 0; k < sizeof(src[0].src_wire)/sizeof(src[0].src_wire[0]); k++)
					src[j].src_wire[k] = wire_decrement(src[j].src_wire[k]);
			}
		}
	}

	// mip 12-18, decrementing directional wires (1024 switches)
	{ struct sw_mip_src src[] = {
		{12, NO_WIRE, 0, 2, 2,
		     NO_WIRE, 14, 2, 3,
			{DW + W_EL1*4+3, DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{12, NO_WIRE, 0, 0, 2,
		     NO_WIRE, 14, 0, 3,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_WW2*4+3, DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{12, NO_WIRE, 0, 1, 2,
		     NO_WIRE, 14, 1, 3,
			{NO_WIRE, NO_WIRE, DW + ((W_NL1*4+0)|DIR_S0),
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + W_SR1*4+3}},

		{14, NO_WIRE, 0, 1, 3,
		     NO_WIRE, 14, 1, 2,
			{DW + ((W_EL1*4+0)|DIR_S0), DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + ((W_WR1*4+0)|DIR_S0), DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{14, NO_WIRE, 0, 0, 3,
		     NO_WIRE, 14, 0, 2,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + ((W_NW2*4+0)|DIR_S0),
			 DW + W_WW2*4+3, DW + ((W_NE2*4+0)|DIR_S0), DW + ((W_NN2*4+0)|DIR_S0)}},
		{14, NO_WIRE, 0, 2, 3,
		     NO_WIRE, 14, 2, 2,
			{NO_WIRE, NO_WIRE, DW + ((W_NL1*4+0)|DIR_S0),
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + W_SR1*4+3}},

		{16, NO_WIRE, 0, 2, 2,
		     NO_WIRE, 14, 2, 3,
			{DW + W_EL1*4+3, DW + W_ER1*4+3, DW + W_WL1*4+3,
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{16, NO_WIRE, 0, 0, 2,
		     NO_WIRE, 14, 0, 3,
			{DW + W_SS2*4+3, DW + W_SW2*4+3, DW + W_NW2*4+3,
			 DW + ((W_WW2*4+2)|DIR_N3), DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{16, NO_WIRE, 0, 1, 2,
		     NO_WIRE, 14, 1, 3,
			{NO_WIRE, NO_WIRE, DW + W_NL1*4+3,
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + ((W_SR1*4+2)|DIR_N3)}},

		{18, NO_WIRE, 0, 1, 3,
		     NO_WIRE, 14, 1, 2,
			{DW + W_EL1*4+3, DW + ((W_ER1*4+2)|DIR_N3), DW + ((W_WL1*4+2)|DIR_N3),
			 DW + W_WR1*4+3, DW + W_EE2*4+3, DW + W_SE2*4+3}},
		{18, NO_WIRE, 0, 0, 3,
		     NO_WIRE, 14, 0, 2,
			{DW + ((W_SS2*4+2)|DIR_N3), DW + ((W_SW2*4+2)|DIR_N3), DW + W_NW2*4+3,
			 DW + ((W_WW2*4+2)|DIR_N3), DW + W_NE2*4+3, DW + W_NN2*4+3}},
		{18, NO_WIRE, 0, 2, 3,
		     NO_WIRE, 14, 2, 2,
			{NO_WIRE, NO_WIRE, DW + W_NL1*4+3,
			 DW + W_NR1*4+3, DW + W_SL1*4+3, DW + ((W_SR1*4+2)|DIR_N3)}}};

		for (i = 0; i < 8; i++) {
			for (j = 0; j < sizeof(src)/sizeof(*src); j++) {

				int logicin_o = ((src[j].minor-12)/2)*LOGIN_MIP_ROWS*LOGIN_ROW;
				logicin_o += i*LOGIN_ROW;

				src[j].m0_sw_to = logicin_matrix[logicin_o+0];
				src[j].m1_sw_to = logicin_matrix[logicin_o+1];

				if (i) {
					src[j].m0_two_bits_o += 16;
					src[j].m0_one_bit_start += 16;
					src[j].m1_two_bits_o += 16;
					src[j].m1_one_bit_start += 16;
					if (!(i%2)) // at 2, 4 and 6 we decrement the wires
						for (k = 0; k < sizeof(src[0].src_wire)/sizeof(src[0].src_wire[0]); k++)
							src[j].src_wire[k] = wire_decrement(src[j].src_wire[k]);
				}
			}
			rc = src_to_bitpos(es, src, sizeof(src)/sizeof(*src));
			if (rc) FAIL(rc);
		}
	}

	// VCC/GND/GFAN, logicin and logicout sources
	// mip12-14
	{ int logicin_src[8][6] = {
		{VCC_WIRE, LW + (LO_D|LD1), LW + LO_DQ,       LW + (LO_BMUX|LD1), LOGICIN_S62,      LOGICIN_S20},
		{GFAN1,    LW + (LO_D|LD1), LW + LO_DQ,       LW + (LO_BMUX|LD1), LOGICIN_S62,      LOGICIN_S20},
		{VCC_WIRE, LW + LO_C,       LW + (LO_CQ|LD1), LW + LO_AMUX,       LOGICIN_S62,      LW + (LI_AX|LD1)},
		{GFAN1,    LW + LO_C,       LW + (LO_CQ|LD1), LW + LO_AMUX,       LOGICIN_S62,      LW + (LI_AX|LD1)},
		{VCC_WIRE, LW + (LO_B|LD1), LW + LO_BQ,       LW + (LO_DMUX|LD1), LW + (LI_AX|LD1), LW + (LI_CI|LD1)},
		{GFAN0,    LW + (LO_B|LD1), LW + LO_BQ,       LW + (LO_DMUX|LD1), LW + (LI_AX|LD1), LW + (LI_CI|LD1)},
		{VCC_WIRE, LW + LO_A,       LW + (LO_AQ|LD1), LW + LO_CMUX,       LOGICIN20,        LW + (LI_CI|LD1)},
		{GFAN0,    LW + LO_A,       LW + (LO_AQ|LD1), LW + LO_CMUX,       LOGICIN20,        LW + (LI_CI|LD1)},
		};

		rc = mip_to_bitpos(es, 12, 3, 2, 3, 3, &logicin_src);
		if (rc) FAIL(rc);

		logicin_src[1][0] = logicin_src[3][0] = logicin_src[5][0] = logicin_src[7][0] = VCC_WIRE;
		logicin_src[0][0] = logicin_src[2][0] = GFAN1;
		logicin_src[4][0] = logicin_src[6][0] = GFAN0;
		rc = mip_to_bitpos(es, 14, 3, 3, 3, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	{ int logicin_src[8][6] = {
		{ LW + LI_BX,       LOGICIN52 },
		{ LW + LI_BX,       LOGICIN52 },
		{ LW + LI_BX,       LW + (LI_DI|LD1) },
		{ LW + LI_BX,       LW + (LI_DI|LD1) },
		{ LW + (LI_DI|LD1), LOGICIN_N28 },
		{ LW + (LI_DI|LD1), LOGICIN_N28 },
		{ LOGICIN_N52,      LOGICIN_N28 },
		{ LOGICIN_N52,      LOGICIN_N28 }};

		rc = mip_to_bitpos(es, 12, 1, 2, 1, 3, &logicin_src);
		if (rc) FAIL(rc);
		rc = mip_to_bitpos(es, 14, 2, 3, 2, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	// mip16-18
	{ int logicin_src[8][6] = {
		{VCC_WIRE, LW + LO_D,       LW + (LO_DQ|LD1), LW + LO_BMUX,       LOGICIN_S36, LOGICIN_S44},
		{GFAN1,    LW + LO_D,       LW + (LO_DQ|LD1), LW + LO_BMUX,       LOGICIN_S36, LOGICIN_S44},
		{VCC_WIRE, LW + (LO_C|LD1), LW + LO_CQ,       LW + (LO_AMUX|LD1), LOGICIN_S36, LW + LI_AX},
		{GFAN1,    LW + (LO_C|LD1), LW + LO_CQ,       LW + (LO_AMUX|LD1), LOGICIN_S36, LW + LI_AX},
		{VCC_WIRE, LW + LO_B,       LW + (LO_BQ|LD1), LW + LO_DMUX,       LW + LI_AX,  LW + (LI_CE|LD1)},
		{GFAN0,    LW + LO_B,       LW + (LO_BQ|LD1), LW + LO_DMUX,       LW + LI_AX,  LW + (LI_CE|LD1)},
		{VCC_WIRE, LW + (LO_A|LD1), LW + LO_AQ,       LW + (LO_CMUX|LD1), LOGICIN44,   LW + (LI_CE|LD1)},
		{GFAN0,    LW + (LO_A|LD1), LW + LO_AQ,       LW + (LO_CMUX|LD1), LOGICIN44,   LW + (LI_CE|LD1)},
		};

		rc = mip_to_bitpos(es, 16, 3, 2, 3, 3, &logicin_src);
		if (rc) FAIL(rc);

		logicin_src[1][0] = logicin_src[3][0] = logicin_src[5][0] = logicin_src[7][0] = VCC_WIRE;
		logicin_src[0][0] = logicin_src[2][0] = GFAN1;
		logicin_src[4][0] = logicin_src[6][0] = GFAN0;
		rc = mip_to_bitpos(es, 18, 3, 3, 3, 2, &logicin_src);
		if (rc) FAIL(rc);
	}
	{ int logicin_src[8][6] = {
		{ LW + (LI_BX|LD1),     LOGICIN21 },
		{ LW + (LI_BX|LD1),     LOGICIN21 },
		{ LW + (LI_BX|LD1),     FAN_B },
		{ LW + (LI_BX|LD1),     FAN_B },
		{ FAN_B,		LOGICIN_N60 },
		{ FAN_B,		LOGICIN_N60 },
		{ LOGICIN_N21,		LOGICIN_N60 },
		{ LOGICIN_N21,		LOGICIN_N60 }};

		rc = mip_to_bitpos(es, 16, 1, 2, 1, 3, &logicin_src);
		if (rc) FAIL(rc);
		rc = mip_to_bitpos(es, 18, 2, 3, 2, 2, &logicin_src);
		if (rc) FAIL(rc);
	}

	// minor 20 switches (SR, CLK, GFAN = 113 switches (4 bidir added on other side))
	{ const struct sw_mi20_src src[] = {
		{SR1, 6, 3, 0, {GCLK11, GCLK10, GCLK13, GCLK12, GCLK9, GCLK8}},
		{SR1, 6, 2, 0, {DW+W_WR1*4+2, DW+W_NR1*4+2,
			VCC_WIRE, GND_WIRE, DW+W_ER1*4+2, DW+W_SR1*4+2}},
		{SR1, 6, 1, 0, {FAN_B, LW+(LI_DI|LD1), LW+(LI_BX|LD1),
			LW+LI_BX, GCLK15, GCLK14}},

		{SR0, 8, 3, 10, {GCLK8, GCLK9, GCLK10, GCLK13, GCLK12, GCLK11}},
		{SR0, 8, 2, 10, {GCLK14, GCLK15, LW+(LI_DI|LD1), LW+(LI_BX|LD1),
			LW+LI_BX, FAN_B}},
		{SR0, 8, 1, 10, {DW+W_SR1*4+2, DW+W_ER1*4+2, DW+W_NR1*4+2,
			VCC_WIRE, NO_WIRE, DW+W_WR1*4+2}},

		{CLK0, 16, 3, 18, {GCLK0, GCLK1, GCLK2, GCLK5, GCLK4, GCLK3}},
		{CLK0, 16, 2, 18, {GCLK6, GCLK7, GCLK8, GCLK11, GCLK10, GCLK9}},
		{CLK0, 16, 1, 18, {GCLK12, GCLK13, GCLK14, LW+(LI_BX|LD1), LW+(LI_CI|LD1), GCLK15}},
		{CLK0, 16, 0, 18, {DW+W_NR1*4+2, DW+W_WR1*4+2,
			DW+W_SR1*4+1, VCC_WIRE, NO_WIRE, DW+W_ER1*4+1}},

		{CLK1, 46, 3, 40, {GCLK3, GCLK2, GCLK5, GCLK4, GCLK1, GCLK0}},
		{CLK1, 46, 2, 40, {GCLK15, GCLK14, LW+(LI_BX|LD1), LW+(LI_CI|LD1), GCLK13, GCLK12}},
		{CLK1, 46, 1, 40, {GCLK9, GCLK8, GCLK11, GCLK10, GCLK7, GCLK6}},
		{CLK1, 46, 0, 40, {DW+W_ER1*4+1, DW+W_SR1*4+1, VCC_WIRE,
			NO_WIRE, DW+W_WR1*4+2, DW+W_NR1*4+2}},

		{GFAN0, 54, 3, 48, {GCLK3, GCLK4, GCLK5, GCLK2, GCLK1, GCLK0}},
		{GFAN0, 54, 2, 48, {DW+W_WR1*4+1, GND_WIRE, VCC_WIRE, DW+W_NR1*4+1, DW+W_ER1*4+1, DW+W_SR1*4+1}},
		{GFAN0, 54, 1, 48, {LW+(LI_CE|LD1), NO_WIRE, NO_WIRE, LW+(LI_CI|LD1), GCLK7, GCLK6}},

		{GFAN1, 56, 3, 58, {GCLK0, GCLK1, GCLK4, GCLK5, GCLK2, GCLK3}},
		{GFAN1, 56, 2, 58, {GCLK6, GCLK7, LW+(LI_AX|LD1), LW+LI_AX, NO_WIRE, NO_WIRE}},
		{GFAN1, 56, 1, 58, {DW+W_SR1*4+1, DW+W_ER1*4+1, GND_WIRE, VCC_WIRE, DW+W_NR1*4+1, DW+W_WR1*4+1}}};

	for (i = 0; i < sizeof(src)/sizeof(*src); i++) {
		for (j = 0; j < sizeof(src[0].src_wire)/sizeof(src[0].src_wire[0]); j++) {
			if (src[i].src_wire[j] == NO_WIRE) continue;

			es->bit_pos[es->num_bit_pos].minor = 20;
			es->bit_pos[es->num_bit_pos].two_bits_o = src[i].two_bits_o;
			es->bit_pos[es->num_bit_pos].two_bits_val = src[i].two_bits_val;
			es->bit_pos[es->num_bit_pos].one_bit_o = src[i].one_bit_start + j;
			es->bit_pos[es->num_bit_pos].uni_dir = fpga_switch_lookup(es->model,
				es->model->first_routing_y, es->model->first_routing_x,
				fpga_wirestr_i(es->model, src[i].src_wire[j]),
				fpga_wirestr_i(es->model, src[i].sw_to));
			if (es->bit_pos[es->num_bit_pos].uni_dir == NO_SWITCH) {
				fprintf(stderr, "#E routing switch %s -> %s not in model\n",
					fpga_wirestr(es->model, src[i].src_wire[j]),
					fpga_wirestr(es->model, src[i].sw_to));
				FAIL(EINVAL);
			}
			es->bit_pos[es->num_bit_pos].rev_dir = NO_SWITCH;
			es->num_bit_pos++;
		}
	}}
	return 0;
fail:
	return rc;
}

int extract_model(struct fpga_model* model, struct fpga_bits* bits)
{
	struct extract_state es;
	net_idx_t net_idx;
	int i, rc;

	rc = construct_extract_state(&es, model);
	if (rc) FAIL(rc);
	es.bits = bits;
	for (i = 0; i < sizeof(s_default_bits)/sizeof(s_default_bits[0]); i++) {
		if (!get_bitp(bits, &s_default_bits[i]))
			FAIL(EINVAL);
		clear_bitp(bits, &s_default_bits[i]);
	}

	rc = extract_iobs(model, bits);
	if (rc) FAIL(rc);
	rc = extract_logic(model, bits);
	if (rc) FAIL(rc);
	rc = extract_switches(&es);
	if (rc) FAIL(rc);

	// turn switches into nets
	if (model->nets)
		HERE(); // should be empty here
	for (i = 0; i < es.num_yx_pos; i++) {
		rc = fpga_net_new(model, &net_idx);
		if (rc) FAIL(rc);
		rc = fpga_net_add_sw(model, net_idx, es.yx_pos[i].y,
			es.yx_pos[i].x, &es.yx_pos[i].idx, 1);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

int printf_swbits(struct fpga_model* model)
{
	struct extract_state es;
	char bit_str[129];
	int i, j, width, rc;

	rc = construct_extract_state(&es, model);
	if (rc) FAIL(rc);
	for (i = 0; i < es.num_bit_pos; i++) {

		width = (es.bit_pos[i].minor == 20) ? 64 : 128;
		for (j = 0; j < width; j++)
			bit_str[j] = '0';
		bit_str[j] = 0;

		if (es.bit_pos[i].two_bits_val & 2)
			bit_str[es.bit_pos[i].two_bits_o] = '1';
		if (es.bit_pos[i].two_bits_val & 1)
			bit_str[es.bit_pos[i].two_bits_o+1] = '1';
		bit_str[es.bit_pos[i].one_bit_o] = '1';
		printf("mi%02i %s %s %s %s\n", es.bit_pos[i].minor,
			fpga_switch_str(model, model->first_routing_y, model->first_routing_x, es.bit_pos[i].uni_dir, SW_TO),
			bit_str,
			fpga_switch_str(model, model->first_routing_y, model->first_routing_x, es.bit_pos[i].uni_dir, SW_FROM),
			es.bit_pos[i].rev_dir != NO_SWITCH ? "<->" : "->");
	}
	return 0;
fail:
	return rc;
}

int write_model(struct fpga_bits* bits, struct fpga_model* model)
{
	int i;
	for (i = 0; i < sizeof(s_default_bits)/sizeof(s_default_bits[0]); i++)
		set_bitp(bits, &s_default_bits[i]);
	return 0;
}
