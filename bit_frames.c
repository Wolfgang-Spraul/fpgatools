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
	struct xc6_routing_bitpos* swpos, int* is_set)
{
	int row_num, row_pos, start_in_frame, two_bits_val, rc;

	*is_set = 0;
	is_in_row(es->model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	if (swpos->minor == 20) {
		two_bits_val = (get_bit(es->bits, row_num, es->model->x_major[x],
			20, start_in_frame + swpos->two_bits_o) << 1)
			| (get_bit(es->bits, row_num, es->model->x_major[x],
			20, start_in_frame + swpos->two_bits_o+1) << 2);
		if (two_bits_val != swpos->two_bits_val)
			return 0;

		if (!get_bit(es->bits, row_num, es->model->x_major[x], 20,
			start_in_frame + swpos->one_bit_o))
			return 0;
	} else {
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
	}
	*is_set = 1;
	return 0;
fail:
	return rc;
}

static int bitpos_clear_bits(struct extract_state* es, int y, int x,
	struct xc6_routing_bitpos* swpos)
{
	int row_num, row_pos, start_in_frame, rc;

	is_in_row(es->model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	if (swpos->minor == 20) {
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
			start_in_frame + swpos->two_bits_o);
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
			start_in_frame + swpos->two_bits_o+1);
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
			start_in_frame + swpos->one_bit_o);
	} else {
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
			start_in_frame + swpos->two_bits_o/2);
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor + 1,
			start_in_frame + swpos->two_bits_o/2);
		clear_bit(es->bits, row_num, es->model->x_major[x], swpos->minor + (swpos->one_bit_o&1),
			start_in_frame + swpos->one_bit_o/2);
	}
	return 0;
fail:
	return rc;
}

static int extract_routing_switches(struct extract_state* es, int y, int x)
{
	struct fpga_tile* tile;
	swidx_t sw_idx;
	int i, is_set, rc;

	tile = YX_TILE(es->model, y, x);
if (y != 68 || x != 12) return 0;

	for (i = 0; i < es->model->num_bitpos; i++) {
		rc = bitpos_is_set(es, y, x, &es->model->sw_bitpos[i], &is_set);
		if (rc) FAIL(rc);
		if (!is_set) continue;

		sw_idx = fpga_switch_lookup(es->model, y, x,
			fpga_wirestr_i(es->model, es->model->sw_bitpos[i].from),
			fpga_wirestr_i(es->model, es->model->sw_bitpos[i].to));
		if (sw_idx == NO_SWITCH) FAIL(EINVAL);
		// todo: es->model->sw_bitpos[i].bidir handling

		if (tile->switches[sw_idx] & SWITCH_BIDIRECTIONAL)
			HERE();
		if (tile->switches[sw_idx] & SWITCH_USED)
			HERE();
		if (es->num_yx_pos >= MAX_YX_SWITCHES)
			{ FAIL(ENOTSUP); }
		es->yx_pos[es->num_yx_pos].y = y;
		es->yx_pos[es->num_yx_pos].x = x;
		es->yx_pos[es->num_yx_pos].idx = sw_idx;
		es->num_yx_pos++;
		rc = bitpos_clear_bits(es, y, x, &es->model->sw_bitpos[i]);
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
		}
	}
	return 0;
fail:
	return rc;
}

static int construct_extract_state(struct extract_state* es, struct fpga_model* model)
{
	int rc;

	memset(es, 0, sizeof(*es));
	es->model = model;
	if (model->first_routing_y == -1)
		FAIL(EINVAL);
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
	for (i = 0; i < model->num_bitpos; i++) {

		width = (model->sw_bitpos[i].minor == 20) ? 64 : 128;
		for (j = 0; j < width; j++)
			bit_str[j] = '0';
		bit_str[j] = 0;

		if (model->sw_bitpos[i].two_bits_val & 2)
			bit_str[model->sw_bitpos[i].two_bits_o] = '1';
		if (model->sw_bitpos[i].two_bits_val & 1)
			bit_str[model->sw_bitpos[i].two_bits_o+1] = '1';
		bit_str[model->sw_bitpos[i].one_bit_o] = '1';
		printf("mi%02i %s %s %s %s\n", model->sw_bitpos[i].minor,
			fpga_wirestr(model, model->sw_bitpos[i].to),
			bit_str,
			fpga_wirestr(model, model->sw_bitpos[i].from),
			model->sw_bitpos[i].bidir ? "<->" : "->");
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
