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

static int write_iobs(struct fpga_bits* bits, struct fpga_model* model)
{
	int i, y, x, type_idx, part_idx, dev_idx, first_iob, rc;
	struct fpga_device* dev;
	uint64_t u64;
	const char* name;

	first_iob = 0;
	for (i = 0; (name = fpga_enum_iob(model, i, &y, &x, &type_idx)); i++) {
		dev_idx = fpga_dev_idx(model, y, x, DEV_IOB, type_idx);
		if (dev_idx == NO_DEV) FAIL(EINVAL);
		dev = FPGA_DEV(model, y, x, dev_idx);
		if (!dev->instantiated)
			continue;

		part_idx = find_iob_sitename(XC6SLX9, name);
		if (part_idx == -1) {
			HERE();
			continue;
		}

		if (!first_iob) {
			first_iob = 1;
			// todo: is this right on the other sides?
			set_bit(bits, /*row*/ 0, get_rightside_major(XC6SLX9),
				/*minor*/ 22, 64*15+XC6_HCLK_BITS+4);
		}

		if (dev->u.iob.istandard[0]) {
			if (!dev->u.iob.I_mux
			    || !dev->u.iob.bypass_mux
			    || strcmp(dev->u.iob.istandard, IO_LVCMOS33)
			    || dev->u.iob.ostandard[0])
				HERE();

			u64 = XC6_IOB_INSTANTIATED;
			u64 |= XC6_IOB_INPUT_LVCMOS33;
			if (dev->u.iob.I_mux == IMUX_I)
				u64 |= XC6_IOB_IMUX_I;
			else if (dev->u.iob.I_mux == IMUX_I_B)
				u64 |= XC6_IOB_IMUX_I_B;
			else HERE();

			frame_set_u64(&bits->d[IOB_DATA_START
				+ part_idx*IOB_ENTRY_LEN], u64);
		} else if (dev->u.iob.ostandard[0]) {
			if (!dev->u.iob.drive_strength
			    || !dev->u.iob.slew
			    || !dev->u.iob.suspend
			    || strcmp(dev->u.iob.ostandard, IO_LVCMOS33)
			    || dev->u.iob.istandard[0])
				HERE();

			u64 = XC6_IOB_INSTANTIATED;
			// for now we always turn on O_PINW even if no net
			// is connected to the pinw
			u64 |= XC6_IOB_MASK_O_PINW;
			switch (dev->u.iob.drive_strength) {
				case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_2; break;
				case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4; break;
				case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6; break;
				case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8; break;
				case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12; break;
				case 16: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16; break;
				case 24: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24; break;
				default: HERE();
			}
			switch (dev->u.iob.slew) {
				case SLEW_SLOW: u64 |= XC6_IOB_SLEW_SLOW; break;
				case SLEW_FAST: u64 |= XC6_IOB_SLEW_FAST; break;
				case SLEW_QUIETIO: u64 |= XC6_IOB_SLEW_QUIETIO; break;
				default: HERE();
			}
			switch (dev->u.iob.suspend) {
				case SUSP_LAST_VAL: u64 |= XC6_IOB_SUSP_LAST_VAL; break;
				case SUSP_3STATE: u64 |= XC6_IOB_SUSP_3STATE; break;
				case SUSP_3STATE_PULLUP: u64 |= XC6_IOB_SUSP_3STATE_PULLUP; break;
				case SUSP_3STATE_PULLDOWN: u64 |= XC6_IOB_SUSP_3STATE_PULLDOWN; break;
				case SUSP_3STATE_KEEPER: u64 |= XC6_IOB_SUSP_3STATE_KEEPER; break;
				case SUSP_3STATE_OCT_ON: u64 |= XC6_IOB_SUSP_3STATE_OCT_ON; break;
				default: HERE();
			}

			frame_set_u64(&bits->d[IOB_DATA_START
				+ part_idx*IOB_ENTRY_LEN], u64);
		} else HERE();
	}
	return 0;
fail:
	return rc;
}

static int extract_iobs(struct fpga_model* model, struct fpga_bits* bits)
{
	int i, num_iobs, iob_y, iob_x, iob_idx, dev_idx, first_iob, rc;
	uint64_t u64;
	const char* iob_sitename;
	struct fpga_device* dev;
	struct fpgadev_iob cfg;

	num_iobs = get_num_iobs(XC6SLX9);
	first_iob = 0;
	for (i = 0; i < num_iobs; i++) {
		u64 = frame_get_u64(&bits->d[IOB_DATA_START + i*IOB_ENTRY_LEN]);
		if (!u64) continue;

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
		memset(&cfg, 0, sizeof(cfg));

		if (!first_iob) {
			first_iob = 1;
			// todo: is this right on the other sides?
			if (!get_bit(bits, /*row*/ 0, get_rightside_major(XC6SLX9),
				/*minor*/ 22, 64*15+XC6_HCLK_BITS+4))
				HERE();
			clear_bit(bits, /*row*/ 0, get_rightside_major(XC6SLX9),
				/*minor*/ 22, 64*15+XC6_HCLK_BITS+4);
		}
		if ((u64 & XC6_IOB_MASK_INSTANTIATED) == XC6_IOB_INSTANTIATED)
			u64 &= ~XC6_IOB_MASK_INSTANTIATED;
		else
			HERE();

		switch (u64 & XC6_IOB_MASK_IO) {
			case XC6_IOB_INPUT_LVCMOS33:
				u64 &= ~XC6_IOB_MASK_IO;

				strcpy(cfg.istandard, IO_LVCMOS33);
				cfg.bypass_mux = BYPASS_MUX_I;

				switch (u64 & XC6_IOB_MASK_I_MUX) {
					case XC6_IOB_IMUX_I:
						u64 &= ~XC6_IOB_MASK_I_MUX;
						cfg.I_mux = IMUX_I;
						break;
					case XC6_IOB_IMUX_I_B:
						u64 &= ~XC6_IOB_MASK_I_MUX;
						cfg.I_mux = IMUX_I_B;
						break;
					default: HERE();
				}
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_2:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16:
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24:
				switch (u64 & XC6_IOB_MASK_IO) {
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_2:
						cfg.drive_strength = 2; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4:
						cfg.drive_strength = 4; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6:
						cfg.drive_strength = 6; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8:
						cfg.drive_strength = 8; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12:
						cfg.drive_strength = 12; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16:
						cfg.drive_strength = 16; break;
					case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24:
						cfg.drive_strength = 24; break;
					default: HERE(); break;
				}
				u64 &= ~XC6_IOB_MASK_IO;
				u64 &= ~XC6_IOB_MASK_O_PINW;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				cfg.O_used = 1;
				switch (u64 & XC6_IOB_MASK_SLEW) {
					case XC6_IOB_SLEW_SLOW:
						u64 &= ~XC6_IOB_MASK_SLEW;
						cfg.slew = SLEW_SLOW;
						break;
					case XC6_IOB_SLEW_FAST:
						u64 &= ~XC6_IOB_MASK_SLEW;
						cfg.slew = SLEW_FAST;
						break;
					case XC6_IOB_SLEW_QUIETIO:
						u64 &= ~XC6_IOB_MASK_SLEW;
						cfg.slew = SLEW_QUIETIO;
						break;
					default: HERE();
				}
				switch (u64 & XC6_IOB_MASK_SUSPEND) {
					case XC6_IOB_SUSP_3STATE:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_3STATE;
						break;
					case XC6_IOB_SUSP_3STATE_OCT_ON:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_3STATE_OCT_ON;
						break;
					case XC6_IOB_SUSP_3STATE_KEEPER:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_3STATE_KEEPER;
						break;
					case XC6_IOB_SUSP_3STATE_PULLUP:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_3STATE_PULLUP;
						break;
					case XC6_IOB_SUSP_3STATE_PULLDOWN:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_3STATE_PULLDOWN;
						break;
					case XC6_IOB_SUSP_LAST_VAL:
						u64 &= ~XC6_IOB_MASK_SUSPEND;
						cfg.suspend = SUSP_LAST_VAL;
						break;
					default: HERE();
				}
				break;
			default: HERE(); break;
		}
		if (!u64) {
			frame_set_u64(&bits->d[IOB_DATA_START
				+ i*IOB_ENTRY_LEN], 0);
			dev->instantiated = 1;
			dev->u.iob = cfg;
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
		two_bits_val = ((get_bit(es->bits, row_num, es->model->x_major[x],
			20, start_in_frame + swpos->two_bits_o) != 0) << 1)
			| ((get_bit(es->bits, row_num, es->model->x_major[x],
			20, start_in_frame + swpos->two_bits_o+1) != 0) << 0);
		if (two_bits_val != swpos->two_bits_val)
			return 0;

		if (!get_bit(es->bits, row_num, es->model->x_major[x], 20,
			start_in_frame + swpos->one_bit_o))
			return 0;
	} else {
		two_bits_val = 
			((get_bit(es->bits, row_num, es->model->x_major[x], swpos->minor,
				start_in_frame + swpos->two_bits_o/2) != 0) << 1)
			| ((get_bit(es->bits, row_num, es->model->x_major[x], swpos->minor+1,
			    start_in_frame + swpos->two_bits_o/2) != 0) << 0);
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

static int bitpos_set_bits(struct fpga_bits* bits, struct fpga_model* model,
	int y, int x, struct xc6_routing_bitpos* swpos)
{
	int row_num, row_pos, start_in_frame, rc;

	is_in_row(model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	if (swpos->minor == 20) {
		if (swpos->two_bits_val & 0x02)
			set_bit(bits, row_num, model->x_major[x], swpos->minor,
				start_in_frame + swpos->two_bits_o);
		if (swpos->two_bits_val & 0x01)
			set_bit(bits, row_num, model->x_major[x], swpos->minor,
				start_in_frame + swpos->two_bits_o+1);
		set_bit(bits, row_num, model->x_major[x], swpos->minor,
			start_in_frame + swpos->one_bit_o);
	} else {
		if (swpos->two_bits_val & 0x02)
			set_bit(bits, row_num, model->x_major[x], swpos->minor,
				start_in_frame + swpos->two_bits_o/2);
		if (swpos->two_bits_val & 0x01)
			set_bit(bits, row_num, model->x_major[x], swpos->minor + 1,
				start_in_frame + swpos->two_bits_o/2);
		set_bit(bits, row_num, model->x_major[x], swpos->minor + (swpos->one_bit_o&1),
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

	for (i = 0; i < es->model->num_bitpos; i++) {
		rc = bitpos_is_set(es, y, x, &es->model->sw_bitpos[i], &is_set);
		if (rc) FAIL(rc);
		if (!is_set) continue;

		sw_idx = fpga_switch_lookup(es->model, y, x,
			fpga_wire2str_i(es->model, es->model->sw_bitpos[i].from),
			fpga_wire2str_i(es->model, es->model->sw_bitpos[i].to));
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

static int construct_extract_state(struct extract_state* es,
	struct fpga_model* model)
{
	memset(es, 0, sizeof(*es));
	es->model = model;
	return 0;
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
		rc = fnet_new(model, &net_idx);
		if (rc) FAIL(rc);
		rc = fnet_add_sw(model, net_idx, es.yx_pos[i].y,
			es.yx_pos[i].x, &es.yx_pos[i].idx, 1);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

int printf_swbits(struct fpga_model* model)
{
	char bit_str[129];
	int i, j, width;

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
			fpga_wire2str(model->sw_bitpos[i].to),
			bit_str,
			fpga_wire2str(model->sw_bitpos[i].from),
			model->sw_bitpos[i].bidir ? "<->" : "->");
	}
	return 0;
}

static int find_bitpos(struct fpga_model* model, int y, int x, swidx_t sw)
{
	enum extra_wires from_w, to_w;
	const char* from_str, *to_str;
	int i;

	from_str = fpga_switch_str(model, y, x, sw, SW_FROM);
	to_str = fpga_switch_str(model, y, x, sw, SW_TO);
	from_w = fpga_str2wire(from_str);
	to_w = fpga_str2wire(to_str);

	if (from_w == NO_WIRE || to_w == NO_WIRE) {
		HERE();
		return -1;
	}
	for (i = 0; i < model->num_bitpos; i++) {
		if (model->sw_bitpos[i].from == from_w
		    && model->sw_bitpos[i].to == to_w)
			return i;
		if (model->sw_bitpos[i].bidir
		    && model->sw_bitpos[i].to == from_w
		    && model->sw_bitpos[i].from == to_w) {
			if (!fpga_switch_is_bidir(model, y, x, sw))
				HERE();
			return i;
		}
	}
	fprintf(stderr, "#E switch %s (%i) to %s (%i) not in model\n",
		from_str, from_w, to_str, to_w);
	return -1;
}

static int write_routing_sw(struct fpga_bits* bits, struct fpga_model* model, int y, int x)
{
	struct fpga_tile* tile;
	int i, bit_pos, rc;

	// go through enabled switches, lookup in sw_bitpos
	// and set bits
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;
		bit_pos = find_bitpos(model, y, x, i);
		if (bit_pos == -1) {
			HERE();
			continue;
		}
		rc = bitpos_set_bits(bits, model, y, x,
			&model->sw_bitpos[bit_pos]);
		if (rc) FAIL(rc);
	}
	return 0;
fail:
	return rc;
}

#define MAX_IOLOGIC_SWBLOCK	4
#define MAX_IOLOGIC_BITS	4
struct iologic_sw_pos
{
	const char* to[MAX_IOLOGIC_SWBLOCK];
	const char* from[MAX_IOLOGIC_SWBLOCK];
	int minor[MAX_IOLOGIC_BITS];
	int b64[MAX_IOLOGIC_BITS];
};

struct iologic_sw_pos s_left_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_right_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_top_outer_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_top_inner_io_swpos[] = { {{0}} };

struct iologic_sw_pos s_bot_inner_io_swpos[] =
{
	// input
	{{"D_ILOGIC_IDATAIN_IODELAY_S"}, {"BIOI_INNER_IBUF0"},
 	 {23, 23, -1},
	 {28, 29}},
	{{"D_ILOGIC_SITE"}, {"D_ILOGIC_IDATAIN_IODELAY"}, {26, -1}, {20}},
	{{"D_ILOGIC_SITE_S"}, {"D_ILOGIC_IDATAIN_IODELAY_S"}, {26, -1}, {23}},
	{{"DFB_ILOGIC_SITE"}, {"D_ILOGIC_SITE"}, {28, -1}, {63}},
	{{"DFB_ILOGIC_SITE_S"}, {"D_ILOGIC_SITE_S"}, {28, -1}, {0}},
	{{"FABRICOUT_ILOGIC_SITE"}, {"D_ILOGIC_SITE"}, {29, -1}, {49}},
	{{"FABRICOUT_ILOGIC_SITE_S"}, {"D_ILOGIC_SITE_S"}, {29, -1}, {14}},

	// output
	{{"OQ_OLOGIC_SITE", "BIOI_INNER_O0"},
	 {"D1_OLOGIC_SITE", "OQ_OLOGIC_SITE"},
	 {26, 27, 28, -1},
	 {40, 21, 57}},
	{{"OQ_OLOGIC_SITE_S", "BIOI_INNER_O1"},
	 {"D1_OLOGIC_SITE_S", "OQ_OLOGIC_SITE_S"},
	 {26, 27, 28, -1},
	 {43, 56, 6}},

	{{"IOI_LOGICOUT0"}, {"IOI_INTER_LOGICOUT0"}, {-1}},
	{{"IOI_LOGICOUT7"}, {"IOI_INTER_LOGICOUT7"}, {-1}},
	{{"IOI_INTER_LOGICOUT0"}, {"FABRICOUT_ILOGIC_SITE"}, {-1}},
	{{"IOI_INTER_LOGICOUT7"}, {"FABRICOUT_ILOGIC_SITE_S"}, {-1}},
	{{"D_ILOGIC_IDATAIN_IODELAY"}, {"BIOI_INNER_IBUF0"}, {-1}},
	{{"D_ILOGIC_IDATAIN_IODELAY_S"}, {"BIOI_INNER_IBUF1"}, {-1}},
	{{"D1_OLOGIC_SITE"}, {"IOI_LOGICINB31"}, {-1}},
	{{0}}
};

struct iologic_sw_pos s_bot_outer_io_swpos[] =
{
	// input
	{{"D_ILOGIC_IDATAIN_IODELAY_S"}, {"BIOI_OUTER_IBUF0"},
 	 {23, 23, -1},
	 {28, 29}},
	{{"D_ILOGIC_SITE"}, {"D_ILOGIC_IDATAIN_IODELAY"}, {26, -1}, {20}},
	{{"D_ILOGIC_SITE_S"}, {"D_ILOGIC_IDATAIN_IODELAY_S"}, {26, -1}, {23}},
	{{"DFB_ILOGIC_SITE"}, {"D_ILOGIC_SITE"}, {28, -1}, {63}},
	{{"DFB_ILOGIC_SITE_S"}, {"D_ILOGIC_SITE_S"}, {28, -1}, {0}},
	{{"FABRICOUT_ILOGIC_SITE"}, {"D_ILOGIC_SITE"}, {29, -1}, {49}},
	{{"FABRICOUT_ILOGIC_SITE_S"}, {"D_ILOGIC_SITE_S"}, {29, -1}, {14}},

	// output
	{{"OQ_OLOGIC_SITE", "BIOI_OUTER_O0"},
	 {"D1_OLOGIC_SITE", "OQ_OLOGIC_SITE"},
	 {26, 27, 28, -1},
	 {40, 21, 57}},
	{{"OQ_OLOGIC_SITE_S", "BIOI_OUTER_O1"},
	 {"D1_OLOGIC_SITE_S", "OQ_OLOGIC_SITE_S"},
	 {26, 27, 28, -1},
	 {43, 56, 6}},

	{{"OQ_OLOGIC_SITE", "BIOI_OUTER_O0"},
	 {"D1_OLOGIC_SITE", "OQ_OLOGIC_SITE"},
	 {26, 27, 28, -1},
	 {40, 21, 57}},

	{{"IOI_LOGICOUT0"}, {"IOI_INTER_LOGICOUT0"}, {-1}},
	{{"IOI_LOGICOUT7"}, {"IOI_INTER_LOGICOUT7"}, {-1}},
	{{"IOI_INTER_LOGICOUT0"}, {"FABRICOUT_ILOGIC_SITE"}, {-1}},
	{{"IOI_INTER_LOGICOUT7"}, {"FABRICOUT_ILOGIC_SITE_S"}, {-1}},
	{{"D_ILOGIC_IDATAIN_IODELAY"}, {"BIOI_INNER_IBUF0"}, {-1}},
	{{"D_ILOGIC_IDATAIN_IODELAY_S"}, {"BIOI_INNER_IBUF1"}, {-1}},
	{{"D1_OLOGIC_SITE"}, {"IOI_LOGICINB31"}, {-1}},
	{{0}}
};

struct str_sw
{
	const char* from_str;
	const char* to_str;
};

static int get_used_switches(struct fpga_model* model, int y, int x,
	struct str_sw** str_sw, int* num_sw)
{
	int i, num_used, rc;
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	num_used = 0;
	for (i = 0; i < tile->num_switches; i++) {
		if (tile->switches[i] & SWITCH_USED)
			num_used++;
	}
	if (!num_used) {
		*num_sw = 0;
		*str_sw = 0;
		return 0;
	}
	*str_sw = malloc(num_used * sizeof(**str_sw));
	if (!(*str_sw)) FAIL(ENOMEM);
	*num_sw = 0;
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;
		(*str_sw)[*num_sw].from_str =
			fpga_switch_str(model, y, x, i, SW_FROM);
		(*str_sw)[*num_sw].to_str =
			fpga_switch_str(model, y, x, i, SW_TO);
		(*num_sw)++;
	}
	return 0;
fail:
	return rc;
}

// returns the number of found to-from pairs from search_sw that
// were found in the str_sw array, but only if all of the to-from
// pairs in search_sw were found. The found array then contains
// the indices into str_sw were the matches were made.
// If not all to-from pairs were found (or none), returns 0.
static int find_switches(struct iologic_sw_pos* search_sw,
	struct str_sw* str_sw, int num_str_sw, int (*found)[MAX_IOLOGIC_SWBLOCK])
{
	int i, j;

	for (i = 0; i < MAX_IOLOGIC_SWBLOCK && search_sw->to[i]; i++) {
		for (j = 0; j < num_str_sw; j++) {
			if (!str_sw[j].to_str)
				continue;
			if (strcmp(str_sw[j].to_str, search_sw->to[i])
			    || strcmp(str_sw[j].from_str, search_sw->from[i]))
				continue;
			(*found)[i] = j;
			break;
		}
		if (j >= num_str_sw)
			return 0;
	}
	return i;
}

static int write_iologic_sw(struct fpga_bits* bits, struct fpga_model* model,
	int y, int x)
{
	int i, j, row_num, row_pos, start_in_frame, rc;
	struct iologic_sw_pos* sw_pos;
	struct str_sw* str_sw;
	int found_i[MAX_IOLOGIC_SWBLOCK];
	int num_sw, num_found;

	if (x < LEFT_SIDE_WIDTH) {
		if (x != LEFT_IO_DEVS) FAIL(EINVAL);
		sw_pos = s_left_io_swpos;
	} else if (x >= model->x_width-RIGHT_SIDE_WIDTH) {
		if (x != model->x_width - RIGHT_IO_DEVS_O) FAIL(EINVAL);
		sw_pos = s_right_io_swpos;
	} else if (y == TOP_OUTER_IO)
		sw_pos = s_top_outer_io_swpos;
	else if (y == TOP_INNER_IO)
		sw_pos = s_top_inner_io_swpos;
	else if (y == model->y_height-BOT_INNER_IO)
		sw_pos = s_bot_inner_io_swpos;
	else if (y == model->y_height-BOT_OUTER_IO)
		sw_pos = s_bot_outer_io_swpos;
	else FAIL(EINVAL);

	is_in_row(model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		start_in_frame = (row_pos-1)*64 + 16;
	else
		start_in_frame = row_pos*64;

	rc = get_used_switches(model, y, x, &str_sw, &num_sw);
	if (rc) FAIL(rc);

	for (i = 0; sw_pos[i].to[0]; i++) {
		if (!(num_found = find_switches(&sw_pos[i], str_sw,
			num_sw, &found_i))) continue;
		for (j = 0; sw_pos[i].minor[j] != -1; j++) {
			set_bit(bits, row_num, model->x_major[x],
				sw_pos[i].minor[j], start_in_frame
				+ sw_pos[i].b64[j]);
		}
		// remove switches from 'used' array
		for (j = 0; j < num_found; j++)
			str_sw[found_i[j]].to_str = 0;
	}

	// check whether all switches were found
	for (i = 0; i < num_sw; i++) {
		if (!str_sw[i].to_str)
			continue;
		fprintf(stderr, "#E %s:%i unsupported switch "
			"y%02i x%02i %s -> %s\n", __FILE__, __LINE__,
			y, x, str_sw[i].from_str, str_sw[i].to_str);
	}
	free(str_sw);
	return 0;
fail:
	return rc;
}

static int write_switches(struct fpga_bits* bits, struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y, i, rc;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			if (is_atx(X_ROUTING_COL, model, x)
			    && y >= TOP_IO_TILES
			    && y < model->y_height-BOT_IO_TILES
			    && !is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y)) {
				rc = write_routing_sw(bits, model, y, x);
				if (rc) FAIL(rc);
				continue;
			}
			if (is_atyx(YX_DEV_ILOGIC, model, y, x)) {
				rc = write_iologic_sw(bits, model, y, x);
				if (rc) FAIL(rc);
				continue;
			}
			// todo: there are probably switches in logic and
			//       iob that need bits, and switches in other
			//       tiles that need no bits...
			if (is_atyx(YX_DEV_LOGIC|YX_DEV_IOB, model, y, x))
				continue;
			tile = YX_TILE(model, y, x);
			for (i = 0; i < tile->num_switches; i++) {
				if (!(tile->switches[i] & SWITCH_USED))
					continue;
				fprintf(stderr, "#E %s:%i unsupported switch "
					"y%02i x%02i %s\n", __FILE__, __LINE__,
					y, x,
					fpga_switch_print(model, y, x, i));
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int write_logic(struct fpga_bits* bits, struct fpga_model* model)
{
	int dev_idx, row, row_pos, xm_col, rc;
	int x, y, byte_off;
	struct fpga_device* dev;
	uint64_t u64;
	uint8_t* u8_p;

	for (x = LEFT_SIDE_WIDTH; x < model->x_width-RIGHT_SIDE_WIDTH; x++) {
		xm_col = is_atx(X_FABRIC_LOGIC_XM_COL, model, x);
		if (!xm_col && !is_atx(X_FABRIC_LOGIC_XL_COL, model, x))
			continue;

		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (!has_device(model, y, x, DEV_LOGIC))
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

			if (xm_col) {
				// X device
				dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGX);
				if (dev_idx == NO_DEV) FAIL(EINVAL);
				dev = FPGA_DEV(model, y, x, dev_idx);
				if (dev->instantiated) {
					u64 = 0x000000B000600086ULL;
					frame_set_u64(u8_p + 26*FRAME_SIZE + byte_off, u64);

					if (dev->u.logic.a2d[LUT_A].lut6
					    && dev->u.logic.a2d[LUT_A].lut6[0]) {
						HERE(); // not supported
					}
					if (dev->u.logic.a2d[LUT_B].lut6
					    && dev->u.logic.a2d[LUT_B].lut6[0]) {
						HERE(); // not supported
					}
					if (dev->u.logic.a2d[LUT_C].lut6
					    && dev->u.logic.a2d[LUT_C].lut6[0]) {
						HERE(); // not supported
					}
					if (dev->u.logic.a2d[LUT_D].lut6
					    && dev->u.logic.a2d[LUT_D].lut6[0]) {
						rc = parse_boolexpr(dev->u.logic.a2d[LUT_D].lut6, &u64);
						if (rc) FAIL(rc);
						write_lut64(u8_p + 29*FRAME_SIZE, byte_off*8, u64);
					}
				}

				// M device
				dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGM);
				if (dev_idx == NO_DEV) FAIL(EINVAL);
				dev = FPGA_DEV(model, y, x, dev_idx);
				if (dev->instantiated) {
					HERE();
				}
			} else {
				// X device
				dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGX);
				if (dev_idx == NO_DEV) FAIL(EINVAL);
				dev = FPGA_DEV(model, y, x, dev_idx);
				if (dev->instantiated) {
					HERE();
				}

				// L device
				dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOGL);
				if (dev_idx == NO_DEV) FAIL(EINVAL);
				dev = FPGA_DEV(model, y, x, dev_idx);
				if (dev->instantiated) {
					HERE();
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

int write_model(struct fpga_bits* bits, struct fpga_model* model)
{
	int i, rc;

	for (i = 0; i < sizeof(s_default_bits)/sizeof(s_default_bits[0]); i++)
		set_bitp(bits, &s_default_bits[i]);
	rc = write_switches(bits, model);
	if (rc) FAIL(rc);
	rc = write_iobs(bits, model);
	if (rc) FAIL(rc);
	rc = write_logic(bits, model);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}
