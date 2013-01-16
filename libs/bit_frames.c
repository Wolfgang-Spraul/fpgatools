//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "bit.h"
#include "control.h"

static uint8_t* get_first_minor(struct fpga_bits* bits, int row, int major)
{
	int i, num_frames;

	if (row < 0) { HERE(); return 0; }
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

static int find_es_switch(struct extract_state* es, int y, int x, swidx_t sw)
{
	int i;

	RC_CHECK(es->model);
	if (sw == NO_SWITCH) { HERE(); return 0; }
	for (i = 0; i < es->num_yx_pos; i++) {
		if (es->yx_pos[i].y == y
		    && es->yx_pos[i].x == x
		    && es->yx_pos[i].idx == sw)
			return 1;
	}
	return 0;
}

static int write_type2(struct fpga_bits* bits, struct fpga_model* model)
{
	int i, y, x, type_idx, part_idx, dev_idx, first_iob, rc;
	struct fpga_device* dev;
	uint64_t u64;
	const char* name;

	RC_CHECK(model);
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
			    || dev->u.iob.ostandard[0])
				HERE();

			u64 = XC6_IOB_INPUT | XC6_IOB_INSTANTIATED;

			if (dev->u.iob.I_mux == IMUX_I_B)
				u64 |= XC6_IOB_IMUX_I_B;

			if (!strcmp(dev->u.iob.istandard, IO_LVCMOS33)
			    || !strcmp(dev->u.iob.istandard, IO_LVCMOS25)
			    || !strcmp(dev->u.iob.istandard, IO_LVTTL))
				u64 |= XC6_IOB_INPUT_LVCMOS33_25_LVTTL;
			else if (!strcmp(dev->u.iob.istandard, IO_LVCMOS18)
			    || !strcmp(dev->u.iob.istandard, IO_LVCMOS15)
			    || !strcmp(dev->u.iob.istandard, IO_LVCMOS12))
				u64 |= XC6_IOB_INPUT_LVCMOS18_15_12;
			else if (!strcmp(dev->u.iob.istandard, IO_LVCMOS18_JEDEC)
			    || !strcmp(dev->u.iob.istandard, IO_LVCMOS15_JEDEC)
			    || !strcmp(dev->u.iob.istandard, IO_LVCMOS12_JEDEC))
				u64 |= XC6_IOB_INPUT_LVCMOS18_15_12_JEDEC;
			else if (!strcmp(dev->u.iob.istandard, IO_SSTL2_I))
				u64 |= XC6_IOB_INPUT_SSTL2_I;
			else
				HERE();

			frame_set_u64(&bits->d[IOB_DATA_START
				+ part_idx*IOB_ENTRY_LEN], u64);
		} else if (dev->u.iob.ostandard[0]) {
			if (!dev->u.iob.drive_strength
			    || !dev->u.iob.slew
			    || !dev->u.iob.suspend
			    || dev->u.iob.istandard[0])
				HERE();

			u64 = XC6_IOB_INSTANTIATED;
			// for now we always turn on O_PINW even if no net
			// is connected to the pinw
			u64 |= XC6_IOB_O_PINW;
			if (!strcmp(dev->u.iob.ostandard, IO_LVTTL)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_12; break;
					case 16: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_16; break;
					case 24: u64 |= XC6_IOB_OUTPUT_LVTTL_DRIVE_24; break;
					default: FAIL(EINVAL);
				}
			} else if (!strcmp(dev->u.iob.ostandard, IO_LVCMOS33)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS33_25_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12; break;
					case 16: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16; break;
					case 24: u64 |= XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24; break;
					default: FAIL(EINVAL);
				}
			} else if (!strcmp(dev->u.iob.ostandard, IO_LVCMOS25)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS33_25_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_12; break;
					case 16: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_16; break;
					case 24: u64 |= XC6_IOB_OUTPUT_LVCMOS25_DRIVE_24; break;
					default: FAIL(EINVAL);
				}
			} else if (!strcmp(dev->u.iob.ostandard, IO_LVCMOS18)
				   || !strcmp(dev->u.iob.ostandard, IO_LVCMOS18_JEDEC)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_12; break;
					case 16: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_16; break;
					case 24: u64 |= XC6_IOB_OUTPUT_LVCMOS18_DRIVE_24; break;
					default: FAIL(EINVAL);
				}
			} else if (!strcmp(dev->u.iob.ostandard, IO_LVCMOS15)
				   || !strcmp(dev->u.iob.ostandard, IO_LVCMOS15_JEDEC)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_12; break;
					case 16: u64 |= XC6_IOB_OUTPUT_LVCMOS15_DRIVE_16; break;
					default: FAIL(EINVAL);
				}
			} else if (!strcmp(dev->u.iob.ostandard, IO_LVCMOS12)
				   || !strcmp(dev->u.iob.ostandard, IO_LVCMOS12_JEDEC)) {
				switch (dev->u.iob.drive_strength) {
					case 2: u64 |= XC6_IOB_OUTPUT_LVCMOS12_DRIVE_2; break;
					case 4: u64 |= XC6_IOB_OUTPUT_LVCMOS12_DRIVE_4; break;
					case 6: u64 |= XC6_IOB_OUTPUT_LVCMOS12_DRIVE_6; break;
					case 8: u64 |= XC6_IOB_OUTPUT_LVCMOS12_DRIVE_8; break;
					case 12: u64 |= XC6_IOB_OUTPUT_LVCMOS12_DRIVE_12; break;
					default: FAIL(EINVAL);
				}
			} else FAIL(EINVAL);
			switch (dev->u.iob.slew) {
				case SLEW_SLOW: u64 |= XC6_IOB_SLEW_SLOW; break;
				case SLEW_FAST: u64 |= XC6_IOB_SLEW_FAST; break;
				case SLEW_QUIETIO: u64 |= XC6_IOB_SLEW_QUIETIO; break;
				default: FAIL(EINVAL);
			}
			switch (dev->u.iob.suspend) {
				case SUSP_LAST_VAL: u64 |= XC6_IOB_SUSP_LAST_VAL; break;
				case SUSP_3STATE: u64 |= XC6_IOB_SUSP_3STATE; break;
				case SUSP_3STATE_PULLUP: u64 |= XC6_IOB_SUSP_3STATE_PULLUP; break;
				case SUSP_3STATE_PULLDOWN: u64 |= XC6_IOB_SUSP_3STATE_PULLDOWN; break;
				case SUSP_3STATE_KEEPER: u64 |= XC6_IOB_SUSP_3STATE_KEEPER; break;
				case SUSP_3STATE_OCT_ON: u64 |= XC6_IOB_SUSP_3STATE_OCT_ON; break;
				default: FAIL(EINVAL);
			}

			frame_set_u64(&bits->d[IOB_DATA_START
				+ part_idx*IOB_ENTRY_LEN], u64);
		} else HERE();
	}
	return 0;
fail:
	return rc;
}

static int extract_iobs(struct extract_state* es)
{
	int i, iob_y, iob_x, iob_idx, dev_idx, first_iob, rc;
	uint64_t u64;
	const char* iob_sitename;
	struct fpga_device* dev;
	struct fpgadev_iob cfg;

	RC_CHECK(es->model);
	first_iob = 0;
	for (i = 0; i < es->model->die->num_t2_ios; i++) {
		u64 = frame_get_u64(&es->bits->d[
			IOB_DATA_START + i*IOB_ENTRY_LEN]);
		if (!u64) continue;

		iob_sitename = get_iob_sitename(XC6SLX9, i);
		if (!iob_sitename) {
			// The space for 6 IOBs on all four sides
			// (6*8 = 48 bytes, *4=192 bytes) is used
			// for clocks etc, so we ignore them here.
			continue;
		}
		rc = fpga_find_iob(es->model, iob_sitename, &iob_y, &iob_x, &iob_idx);
		if (rc) FAIL(rc);
		dev_idx = fpga_dev_idx(es->model, iob_y, iob_x, DEV_IOB, iob_idx);
		if (dev_idx == NO_DEV) FAIL(EINVAL);
		dev = FPGA_DEV(es->model, iob_y, iob_x, dev_idx);
		memset(&cfg, 0, sizeof(cfg));

		if (!first_iob) {
			first_iob = 1;
			// todo: is this right on the other sides?
			if (!get_bit(es->bits, /*row*/ 0, get_rightside_major(XC6SLX9),
				/*minor*/ 22, 64*15+XC6_HCLK_BITS+4))
				HERE();
			clear_bit(es->bits, /*row*/ 0, get_rightside_major(XC6SLX9),
				/*minor*/ 22, 64*15+XC6_HCLK_BITS+4);
		}
		if (u64 & XC6_IOB_INSTANTIATED)
			u64 &= ~XC6_IOB_INSTANTIATED;
		else
			HERE();

		switch (u64 & XC6_IOB_MASK_IO) {
			case XC6_IOB_INPUT:
				cfg.bypass_mux = BYPASS_MUX_I;

				if (u64 & XC6_IOB_IMUX_I_B) {
					cfg.I_mux = IMUX_I_B;
					u64 &= ~XC6_IOB_IMUX_I_B;
				} else
					cfg.I_mux = IMUX_I;

				switch (u64 & XC6_IOB_MASK_IN_TYPE) {
					case XC6_IOB_INPUT_LVCMOS33_25_LVTTL:
						u64 &= ~XC6_IOB_MASK_IN_TYPE;
						strcpy(cfg.istandard, IO_LVCMOS25);
						break;
					case XC6_IOB_INPUT_LVCMOS18_15_12:
						u64 &= ~XC6_IOB_MASK_IN_TYPE;
						strcpy(cfg.istandard, IO_LVCMOS12);
						break;
					case XC6_IOB_INPUT_LVCMOS18_15_12_JEDEC:
						u64 &= ~XC6_IOB_MASK_IN_TYPE;
						strcpy(cfg.istandard, IO_LVCMOS12_JEDEC);
						break;
					case XC6_IOB_INPUT_SSTL2_I:
						u64 &= ~XC6_IOB_MASK_IN_TYPE;
						strcpy(cfg.istandard, IO_SSTL2_I);
						break;
					default: HERE(); break;
				}
				break;

			case XC6_IOB_OUTPUT_LVCMOS33_25_DRIVE_2:	
				cfg.drive_strength = 2;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_16:
				cfg.drive_strength = 16;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;
			case XC6_IOB_OUTPUT_LVCMOS33_DRIVE_24:
				cfg.drive_strength = 24;
				strcpy(cfg.ostandard, IO_LVCMOS33);
				break;

			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_16:
				cfg.drive_strength = 16;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;
			case XC6_IOB_OUTPUT_LVCMOS25_DRIVE_24:
				cfg.drive_strength = 24;
				strcpy(cfg.ostandard, IO_LVCMOS25);
				break;

			case XC6_IOB_OUTPUT_LVTTL_DRIVE_2:
				cfg.drive_strength = 2;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_16:
				cfg.drive_strength = 16;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;
			case XC6_IOB_OUTPUT_LVTTL_DRIVE_24:
				cfg.drive_strength = 24;
				strcpy(cfg.ostandard, IO_LVTTL);
				break;

			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_2:
				cfg.drive_strength = 2;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_16:
				cfg.drive_strength = 16;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;
			case XC6_IOB_OUTPUT_LVCMOS18_DRIVE_24:
				cfg.drive_strength = 24;
				strcpy(cfg.ostandard, IO_LVCMOS18);
				break;

			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_2:
				cfg.drive_strength = 2;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;
			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;
			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;
			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;
			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;
			case XC6_IOB_OUTPUT_LVCMOS15_DRIVE_16:
				cfg.drive_strength = 16;
				strcpy(cfg.ostandard, IO_LVCMOS15);
				break;

			case XC6_IOB_OUTPUT_LVCMOS12_DRIVE_2:
				cfg.drive_strength = 2;
				strcpy(cfg.ostandard, IO_LVCMOS12);
				break;
			case XC6_IOB_OUTPUT_LVCMOS12_DRIVE_4:
				cfg.drive_strength = 4;
				strcpy(cfg.ostandard, IO_LVCMOS12);
				break;
			case XC6_IOB_OUTPUT_LVCMOS12_DRIVE_6:
				cfg.drive_strength = 6;
				strcpy(cfg.ostandard, IO_LVCMOS12);
				break;
			case XC6_IOB_OUTPUT_LVCMOS12_DRIVE_8:
				cfg.drive_strength = 8;
				strcpy(cfg.ostandard, IO_LVCMOS12);
				break;
			case XC6_IOB_OUTPUT_LVCMOS12_DRIVE_12:
				cfg.drive_strength = 12;
				strcpy(cfg.ostandard, IO_LVCMOS12);
				break;

			default: HERE(); break;
		}
		if (cfg.istandard[0] || cfg.ostandard[0])
			u64 &= ~XC6_IOB_MASK_IO;
		if (cfg.ostandard[0]) {
			cfg.O_used = 1;
			u64 &= ~XC6_IOB_O_PINW;

			if (!strcmp(cfg.ostandard, IO_LVCMOS12)
			    || !strcmp(cfg.ostandard, IO_LVCMOS15)
			    || !strcmp(cfg.ostandard, IO_LVCMOS18)
			    || !strcmp(cfg.ostandard, IO_LVCMOS25)
			    || !strcmp(cfg.ostandard, IO_LVCMOS33)
			    || !strcmp(cfg.ostandard, IO_LVTTL)) {
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
		}
		if (!u64) {
			frame_set_u64(&es->bits->d[IOB_DATA_START
				+ i*IOB_ENTRY_LEN], 0);
			dev->instantiated = 1;
			dev->u.iob = cfg;
		} else HERE();
	}
	return 0;
fail:
	return rc;
}

static int extract_type2(struct extract_state* es)
{
	int gclk_i, bits_off;
	uint16_t u16;

	RC_CHECK(es->model);
	extract_iobs(es);
	for (gclk_i = 0; gclk_i < es->model->die->num_gclk_pins; gclk_i++) {
		bits_off = IOB_DATA_START
			+ es->model->die->gclk_t2_switches[gclk_i]*XC6_WORD_BYTES
			+ XC6_TYPE2_GCLK_REG_SW/XC6_WORD_BITS;
		u16 = frame_get_u16(&es->bits->d[bits_off]);
		if (!u16)
			continue;
		if (u16 & (1<<(XC6_TYPE2_GCLK_REG_SW%XC6_WORD_BITS))) {
			int t2_io_idx, iob_y, iob_x, iob_type_idx;
			struct fpga_device *iob_dev;
			struct switch_to_yx_l2 switch_to_yx_l2;
			struct switch_to_rel switch_to_rel;

			//
			// find and enable reg-switch for gclk_pin[i]
			// the writing equivalent is in write_inner_term_sw()
			//

			t2_io_idx = es->model->die->gclk_t2_io_idx[gclk_i];
			iob_y = es->model->die->t2_io[t2_io_idx].y;
			iob_x = es->model->die->t2_io[t2_io_idx].x;
			iob_type_idx = es->model->die->t2_io[t2_io_idx].type_idx;
			iob_dev = fdev_p(es->model, iob_y, iob_x, DEV_IOB, iob_type_idx);
			RC_ASSERT(es->model, iob_dev);

			switch_to_yx_l2.l1.yx_req = YX_X_CENTER_CMTPLL | YX_Y_CENTER;
			switch_to_yx_l2.l1.flags = SWTO_YX_CLOSEST;
			switch_to_yx_l2.l1.model = es->model;
			switch_to_yx_l2.l1.y = iob_y;
			switch_to_yx_l2.l1.x = iob_x;
			switch_to_yx_l2.l1.start_switch = iob_dev->pinw[IOB_OUT_I];
			switch_to_yx_l2.l1.from_to = SW_FROM;
			fpga_switch_to_yx_l2(&switch_to_yx_l2);
			RC_CHECK(es->model);
			if (!switch_to_yx_l2.l1.set.len)
				{ HERE(); continue; }

			switch_to_rel.model = es->model;
			switch_to_rel.start_y = switch_to_yx_l2.l1.dest_y;
			switch_to_rel.start_x = switch_to_yx_l2.l1.dest_x;
			switch_to_rel.start_switch = switch_to_yx_l2.l1.dest_connpt;
			switch_to_rel.from_to = SW_FROM;
			switch_to_rel.flags = SWTO_REL_WEAK_TARGET;
			switch_to_rel.rel_y = es->model->center_y - switch_to_rel.start_y;
			switch_to_rel.rel_x = es->model->center_x - switch_to_rel.start_x;
			switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
			fpga_switch_to_rel(&switch_to_rel);
			RC_CHECK(es->model);
			if (!switch_to_rel.set.len)
				{ HERE(); continue; }
			if (switch_to_rel.set.len != 1) HERE();

			RC_ASSERT(es->model, es->num_yx_pos < MAX_YX_SWITCHES);
			es->yx_pos[es->num_yx_pos].y = switch_to_rel.start_y;
			es->yx_pos[es->num_yx_pos].x = switch_to_rel.start_x;
			es->yx_pos[es->num_yx_pos].idx = switch_to_rel.set.sw[0];
			es->num_yx_pos++;

			u16 &= ~(1<<(XC6_TYPE2_GCLK_REG_SW%XC6_WORD_BITS));
		}
		if (u16) HERE();
		frame_set_u16(&es->bits->d[bits_off], u16);
	}
	RC_RETURN(es->model);
}

static int lut2str(uint64_t lut, int lut_pos, int lut5_used,
	char *lut6_buf, char** lut6_p, char *lut5_buf, char** lut5_p)
{
	int lut_map[64], rc;
	uint64_t lut_mapped;
	const char* str;

	if (lut5_used) {
		xc6_lut_bitmap(lut_pos, &lut_map, 32);
		lut_mapped = map_bits(lut, 64, lut_map);

		// lut6
		str = bool_bits2str(ULL_HIGH32(lut_mapped), 32);
		if (!str) FAIL(EINVAL);
		snprintf(lut6_buf, MAX_LUT_LEN, "(A6+~A6)*(%s)", str);
		*lut6_p = lut6_buf;

		// lut5
		str = bool_bits2str(ULL_LOW32(lut_mapped), 32);
		if (!str) FAIL(EINVAL);
		strcpy(lut5_buf, str);
		*lut5_p = lut5_buf;
	} else {
		xc6_lut_bitmap(lut_pos, &lut_map, 64);
		lut_mapped = map_bits(lut, 64, lut_map);
		str = bool_bits2str(lut_mapped, 64);
		if (!str) FAIL(EINVAL);
		strcpy(lut6_buf, str);
		*lut6_p = lut6_buf;
	}
	return 0;
fail:
	return rc;
}

static int extract_logic(struct extract_state* es)
{
	int row, row_pos, x, y, i, byte_off, last_minor, lut5_used, rc;
	int latch_ml, latch_x, l_col, lut;
	struct fpgadev_logic cfg_ml, cfg_x;
	uint64_t lut_X[4], lut_ML[4]; // LUT_A-LUT_D
	uint64_t mi20, mi23_M, mi2526;
	uint8_t* u8_p;
	char lut6_ml[NUM_LUTS][MAX_LUT_LEN];
	char lut5_ml[NUM_LUTS][MAX_LUT_LEN];
	char lut6_x[NUM_LUTS][MAX_LUT_LEN];
	char lut5_x[NUM_LUTS][MAX_LUT_LEN];
	struct fpga_device* dev_ml;

	RC_CHECK(es->model);
	for (x = LEFT_SIDE_WIDTH; x < es->model->x_width-RIGHT_SIDE_WIDTH; x++) {
		if (!is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, es->model, x))
			continue;
		for (y = TOP_IO_TILES; y < es->model->y_height - BOT_IO_TILES; y++) {
			if (!has_device(es->model, y, x, DEV_LOGIC))
				continue;
			row = which_row(y, es->model);
			row_pos = pos_in_row(y, es->model);
			if (row == -1 || row_pos == -1 || row_pos == 8) {
				HERE();
				continue;
			}
			if (row_pos > 8) row_pos--;
			u8_p = get_first_minor(es->bits, row, es->model->x_major[x]);
			byte_off = row_pos * 8;
			if (row_pos >= 8) byte_off += XC6_HCLK_BYTES;

			//
			// Step 1:
			//
			// read all configuration bits for the 2 logic devices
			// into local variables
			//

			mi20 = frame_get_u64(u8_p + 20*FRAME_SIZE + byte_off) & XC6_MI20_LOGIC_MASK;
			if (has_device_type(es->model, y, x, DEV_LOGIC, LOGIC_M)) {
				mi23_M = frame_get_u64(u8_p + 23*FRAME_SIZE + byte_off);
				mi2526 = frame_get_u64(u8_p + 26*FRAME_SIZE + byte_off);
				lut_ML[LUT_A] = frame_get_lut64(u8_p + 24*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_B] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_C] = frame_get_lut64(u8_p + 24*FRAME_SIZE, row_pos*2);
				lut_ML[LUT_D] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2);
				lut_X[LUT_A] = frame_get_lut64(u8_p + 27*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_B] = frame_get_lut64(u8_p + 29*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_C] = frame_get_lut64(u8_p + 27*FRAME_SIZE, row_pos*2);
				lut_X[LUT_D] = frame_get_lut64(u8_p + 29*FRAME_SIZE, row_pos*2);
				l_col = 0;
			} else if (has_device_type(es->model, y, x, DEV_LOGIC, LOGIC_L)) {
				mi23_M = 0;
				mi2526 = frame_get_u64(u8_p + 25*FRAME_SIZE + byte_off);
				lut_ML[LUT_A] = frame_get_lut64(u8_p + 23*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_B] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_C] = frame_get_lut64(u8_p + 23*FRAME_SIZE, row_pos*2);
				lut_ML[LUT_D] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2);
				lut_X[LUT_A] = frame_get_lut64(u8_p + 26*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_B] = frame_get_lut64(u8_p + 28*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_C] = frame_get_lut64(u8_p + 26*FRAME_SIZE, row_pos*2);
				lut_X[LUT_D] = frame_get_lut64(u8_p + 28*FRAME_SIZE, row_pos*2);
				l_col = 1;
			} else {
				HERE();
				continue;
			}

			//
			// Step 2:
			//
			// If all is zero, assume the devices are not instantiated.
			// todo: we could check pin connectivity and infer 0-bit
			//       configured devices 
			//

		   	if (!mi20 && !mi23_M && !mi2526
			    && !lut_X[0] && !lut_X[1] && !lut_X[2] && !lut_X[3]
			    && !lut_ML[0] && !lut_ML[1] && !lut_ML[2] && !lut_ML[3])
				continue;

			//
			// Step 3:
			//
			// Parse all bits from minors 20 and 25/26 into more
			// easily usable cfg_ml and cfg_x structures.
			//

			memset(&cfg_ml, 0, sizeof(cfg_ml));
			memset(&cfg_x, 0, sizeof(cfg_x));
			latch_ml = 0;
			latch_x = 0;

			// minor20
			if (mi20 & (1ULL<<XC6_ML_D5_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_D].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_ML_D5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_X_D5_FFSRINIT_1)) {
				cfg_x.a2d[LUT_D].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_X_D5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_ML_C5_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_C].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_ML_C5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_X_C_FFSRINIT_1)) {
				cfg_x.a2d[LUT_C].ff_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_X_C_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_X_C5_FFSRINIT_1)) {
				cfg_x.a2d[LUT_C].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_X_C5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_ML_B5_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_B].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_ML_B5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_X_B5_FFSRINIT_1)) {
				cfg_x.a2d[LUT_B].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_X_B5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_M_A_FFSRINIT_1)) {
				if (l_col) {
					HERE();
					continue;
				}
				cfg_ml.a2d[LUT_A].ff_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_M_A_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_X_A5_FFSRINIT_1)) {
				cfg_x.a2d[LUT_A].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_X_A5_FFSRINIT_1);
			}
			if (mi20 & (1ULL<<XC6_ML_A5_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_A].ff5_srinit = FF_SRINIT1;
				mi20 &= ~(1ULL<<XC6_ML_A5_FFSRINIT_1);
			}

			// minor 25/26
			if (mi2526 & (1ULL<<XC6_ML_D_CY0_O5)) {
				cfg_ml.a2d[LUT_D].cy0 = CY0_O5;
				mi2526 &= ~(1ULL<<XC6_ML_D_CY0_O5);
			}
			if (mi2526 & (1ULL<<XC6_X_D_OUTMUX_O5)) {
				cfg_x.a2d[LUT_D].out_mux = MUX_O5;
				mi2526 &= ~(1ULL<<XC6_X_D_OUTMUX_O5);
			}
			if (mi2526 & (1ULL<<XC6_X_C_OUTMUX_O5)) {
				cfg_x.a2d[LUT_C].out_mux = MUX_O5;
				mi2526 &= ~(1ULL<<XC6_X_C_OUTMUX_O5);
			}
			if (mi2526 & (1ULL<<XC6_ML_D_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_D].ff_srinit = 1;
				mi2526 &= ~(1ULL<<XC6_ML_D_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_ML_C_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_C].ff_srinit = 1;
				mi2526 &= ~(1ULL<<XC6_ML_C_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_X_D_FFSRINIT_1)) {
				cfg_x.a2d[LUT_D].ff_srinit = 1;
				mi2526 &= ~(1ULL<<XC6_X_D_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_ML_C_CY0_O5)) {
				cfg_ml.a2d[LUT_C].cy0 = CY0_O5;
				mi2526 &= ~(1ULL<<XC6_ML_C_CY0_O5);
			}
			if (mi2526 & (1ULL<<XC6_X_B_OUTMUX_O5)) {
				cfg_x.a2d[LUT_B].out_mux = MUX_O5;
				mi2526 &= ~(1ULL<<XC6_X_B_OUTMUX_O5);
			}
			if (mi2526 & XC6_ML_D_OUTMUX_MASK) {
				switch ((mi2526 & XC6_ML_D_OUTMUX_MASK) >> XC6_ML_D_OUTMUX_O) {
					case XC6_ML_D_OUTMUX_O6:
						cfg_ml.a2d[LUT_D].out_mux = MUX_O6;
						break;
					case XC6_ML_D_OUTMUX_XOR:
						cfg_ml.a2d[LUT_D].out_mux = MUX_XOR;
						break;
					case XC6_ML_D_OUTMUX_O5:
						cfg_ml.a2d[LUT_D].out_mux = MUX_O5;
						break;
					case XC6_ML_D_OUTMUX_CY:
						cfg_ml.a2d[LUT_D].out_mux = MUX_CY;
						break;
					case XC6_ML_D_OUTMUX_5Q:
						cfg_ml.a2d[LUT_D].out_mux = MUX_5Q;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_D_OUTMUX_MASK;
			}
			if (mi2526 & XC6_ML_D_FFMUX_MASK) {
				switch ((mi2526 & XC6_ML_D_FFMUX_MASK) >> XC6_ML_D_FFMUX_O) {
					case XC6_ML_D_FFMUX_O5:
						cfg_ml.a2d[LUT_D].ff_mux = MUX_O5;
						break;
					case XC6_ML_D_FFMUX_X:
						cfg_ml.a2d[LUT_D].ff_mux = MUX_X;
						break;
					case XC6_ML_D_FFMUX_XOR:
						cfg_ml.a2d[LUT_D].ff_mux = MUX_XOR;
						break;
					case XC6_ML_D_FFMUX_CY:
						cfg_ml.a2d[LUT_D].ff_mux = MUX_CY;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_D_FFMUX_MASK;
			}
			if (mi2526 & (1ULL<<XC6_X_CLK_B)) {
				cfg_x.clk_inv = CLKINV_B;
				mi2526 &= ~(1ULL<<XC6_X_CLK_B);
			}
			if (mi2526 & (1ULL<<XC6_ML_ALL_LATCH)) {
				latch_ml = 1;
				mi2526 &= ~(1ULL<<XC6_ML_ALL_LATCH);
			}
			if (mi2526 & (1ULL<<XC6_ML_SR_USED)) {
				cfg_ml.sr_used = 1;
				mi2526 &= ~(1ULL<<XC6_ML_SR_USED);
			}
			if (mi2526 & (1ULL<<XC6_ML_SYNC)) {
				cfg_ml.sync_attr = SYNCATTR_SYNC;
				mi2526 &= ~(1ULL<<XC6_ML_SYNC);
			}
			if (mi2526 & (1ULL<<XC6_ML_CE_USED)) {
				cfg_ml.ce_used = 1;
				mi2526 &= ~(1ULL<<XC6_ML_CE_USED);
			}
			if (mi2526 & (1ULL<<XC6_X_D_FFMUX_X)) {
				cfg_x.a2d[LUT_D].ff_mux = MUX_X;
				mi2526 &= ~(1ULL<<XC6_X_D_FFMUX_X);
			}
			if (mi2526 & (1ULL<<XC6_X_C_FFMUX_X)) {
				cfg_x.a2d[LUT_C].ff_mux = MUX_X;
				mi2526 &= ~(1ULL<<XC6_X_C_FFMUX_X);
			}
			if (mi2526 & (1ULL<<XC6_X_CE_USED)) {
				cfg_x.ce_used = 1;
				mi2526 &= ~(1ULL<<XC6_X_CE_USED);
			}
			if (mi2526 & XC6_ML_C_OUTMUX_MASK) {
				switch ((mi2526 & XC6_ML_C_OUTMUX_MASK) >> XC6_ML_C_OUTMUX_O) {
					case XC6_ML_C_OUTMUX_XOR:
						cfg_ml.a2d[LUT_C].out_mux = MUX_XOR;
						break;
					case XC6_ML_C_OUTMUX_O6:
						cfg_ml.a2d[LUT_C].out_mux = MUX_O6;
						break;
					case XC6_ML_C_OUTMUX_5Q:
						cfg_ml.a2d[LUT_C].out_mux = MUX_5Q;
						break;
					case XC6_ML_C_OUTMUX_CY:
						cfg_ml.a2d[LUT_C].out_mux = MUX_CY;
						break;
					case XC6_ML_C_OUTMUX_O5:
						cfg_ml.a2d[LUT_C].out_mux = MUX_O5;
						break;
					case XC6_ML_C_OUTMUX_F7:
						cfg_ml.a2d[LUT_C].out_mux = MUX_F7;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_C_OUTMUX_MASK;
			}
			if (mi2526 & XC6_ML_C_FFMUX_MASK) {
				switch ((mi2526 & XC6_ML_C_FFMUX_MASK) >> XC6_ML_C_FFMUX_O) {
					case XC6_ML_C_FFMUX_O5:
						cfg_ml.a2d[LUT_C].ff_mux = MUX_O5;
						break;
					case XC6_ML_C_FFMUX_X:
						cfg_ml.a2d[LUT_C].ff_mux = MUX_X;
						break;
					case XC6_ML_C_FFMUX_F7:
						cfg_ml.a2d[LUT_C].ff_mux = MUX_F7;
						break;
					case XC6_ML_C_FFMUX_XOR:
						cfg_ml.a2d[LUT_C].ff_mux = MUX_XOR;
						break;
					case XC6_ML_C_FFMUX_CY:
						cfg_ml.a2d[LUT_C].ff_mux = MUX_CY;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_C_FFMUX_MASK;
			}
			if (mi2526 & XC6_ML_B_OUTMUX_MASK) {
				switch ((mi2526 & XC6_ML_B_OUTMUX_MASK) >> XC6_ML_B_OUTMUX_O) {
					case XC6_ML_B_OUTMUX_5Q:
						cfg_ml.a2d[LUT_B].out_mux = MUX_5Q;
						break;
					case XC6_ML_B_OUTMUX_F8:
						cfg_ml.a2d[LUT_B].out_mux = MUX_F8;
						break;
					case XC6_ML_B_OUTMUX_XOR:
						cfg_ml.a2d[LUT_B].out_mux = MUX_XOR;
						break;
					case XC6_ML_B_OUTMUX_CY:
						cfg_ml.a2d[LUT_B].out_mux = MUX_CY;
						break;
					case XC6_ML_B_OUTMUX_O6:
						cfg_ml.a2d[LUT_B].out_mux = MUX_O6;
						break;
					case XC6_ML_B_OUTMUX_O5:
						cfg_ml.a2d[LUT_B].out_mux = MUX_O5;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_B_OUTMUX_MASK;
			}
			if (mi2526 & (1ULL<<XC6_X_B_FFMUX_X)) {
				cfg_x.a2d[LUT_B].ff_mux = MUX_X;
				mi2526 &= ~(1ULL<<XC6_X_B_FFMUX_X);
			}
			if (mi2526 & (1ULL<<XC6_X_A_FFMUX_X)) {
				cfg_x.a2d[LUT_A].ff_mux = MUX_X;
				mi2526 &= ~(1ULL<<XC6_X_A_FFMUX_X);
			}
			if (mi2526 & (1ULL<<XC6_X_B_FFSRINIT_1)) {
				cfg_x.a2d[LUT_B].ff_srinit = FF_SRINIT1;
				mi2526 &= ~(1ULL<<XC6_X_B_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_X_A_OUTMUX_O5)) {
				cfg_x.a2d[LUT_A].out_mux = MUX_O5;
				mi2526 &= ~(1ULL<<XC6_X_A_OUTMUX_O5);
			}
			if (mi2526 & (1ULL<<XC6_X_SR_USED)) {
				cfg_x.sr_used = 1;
				mi2526 &= ~(1ULL<<XC6_X_SR_USED);
			}
			if (mi2526 & (1ULL<<XC6_X_SYNC)) {
				cfg_x.sync_attr = SYNCATTR_SYNC;
				mi2526 &= ~(1ULL<<XC6_X_SYNC);
			}
			if (mi2526 & (1ULL<<XC6_X_ALL_LATCH)) {
				latch_x = 1;
				mi2526 &= ~(1ULL<<XC6_X_ALL_LATCH);
			}
			if (mi2526 & (1ULL<<XC6_ML_CLK_B)) {
				cfg_ml.clk_inv = CLKINV_B;
				mi2526 &= ~(1ULL<<XC6_ML_CLK_B);
			}
			if (mi2526 & XC6_ML_B_FFMUX_MASK) {
				switch ((mi2526 & XC6_ML_B_FFMUX_MASK) >> XC6_ML_B_FFMUX_O) {
					case XC6_ML_B_FFMUX_XOR:
						cfg_ml.a2d[LUT_B].ff_mux = MUX_XOR;
						break;
					case XC6_ML_B_FFMUX_O5:
						cfg_ml.a2d[LUT_B].ff_mux = MUX_O5;
						break;
					case XC6_ML_B_FFMUX_CY:
						cfg_ml.a2d[LUT_B].ff_mux = MUX_CY;
						break;
					case XC6_ML_B_FFMUX_X:
						cfg_ml.a2d[LUT_B].ff_mux = MUX_X;
						break;
					case XC6_ML_B_FFMUX_F8:
						cfg_ml.a2d[LUT_B].ff_mux = MUX_F8;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_B_FFMUX_MASK;
			}
			if (mi2526 & XC6_ML_A_FFMUX_MASK) {
				switch ((mi2526 & XC6_ML_A_FFMUX_MASK) >> XC6_ML_A_FFMUX_O) {
					case XC6_ML_A_FFMUX_XOR:
						cfg_ml.a2d[LUT_A].ff_mux = MUX_XOR;
						break;
					case XC6_ML_A_FFMUX_X:
						cfg_ml.a2d[LUT_A].ff_mux = MUX_X;
						break;
					case XC6_ML_A_FFMUX_O5:
						cfg_ml.a2d[LUT_A].ff_mux = MUX_O5;
						break;
					case XC6_ML_A_FFMUX_CY:
						cfg_ml.a2d[LUT_A].ff_mux = MUX_CY;
						break;
					case XC6_ML_A_FFMUX_F7:
						cfg_ml.a2d[LUT_A].ff_mux = MUX_F7;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_A_FFMUX_MASK;
			}
			if (mi2526 & XC6_ML_A_OUTMUX_MASK) {
				switch ((mi2526 & XC6_ML_A_OUTMUX_MASK) >> XC6_ML_A_OUTMUX_O) {
					case XC6_ML_A_OUTMUX_5Q:
						cfg_ml.a2d[LUT_A].out_mux = MUX_5Q;
						break;
					case XC6_ML_A_OUTMUX_F7:
						cfg_ml.a2d[LUT_A].out_mux = MUX_F7;
						break;
					case XC6_ML_A_OUTMUX_XOR:
						cfg_ml.a2d[LUT_A].out_mux = MUX_XOR;
						break;
					case XC6_ML_A_OUTMUX_CY:
						cfg_ml.a2d[LUT_A].out_mux = MUX_CY;
						break;
					case XC6_ML_A_OUTMUX_O6:
						cfg_ml.a2d[LUT_A].out_mux = MUX_O6;
						break;
					case XC6_ML_A_OUTMUX_O5:
						cfg_ml.a2d[LUT_A].out_mux = MUX_O5;
						break;
					default: HERE(); continue;
				}
				mi2526 &= ~XC6_ML_A_OUTMUX_MASK;
			}
			if (mi2526 & (1ULL<<XC6_ML_B_CY0_O5)) {
				cfg_ml.a2d[LUT_B].cy0 = CY0_O5;
				mi2526 &= ~(1ULL<<XC6_ML_B_CY0_O5);
			}
			if (mi2526 & (1ULL<<XC6_ML_PRECYINIT_AX)) {
				cfg_ml.precyinit = PRECYINIT_AX;
				mi2526 &= ~(1ULL<<XC6_ML_PRECYINIT_AX);
			}
			if (mi2526 & (1ULL<<XC6_X_A_FFSRINIT_1)) {
				cfg_x.a2d[LUT_A].ff_srinit = FF_SRINIT1;
				mi2526 &= ~(1ULL<<XC6_X_A_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_ML_PRECYINIT_1)) {
				cfg_ml.precyinit = PRECYINIT_1;
				mi2526 &= ~(1ULL<<XC6_ML_PRECYINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_ML_B_FFSRINIT_1)) {
				cfg_ml.a2d[LUT_B].ff_srinit = FF_SRINIT1;
				mi2526 &= ~(1ULL<<XC6_ML_B_FFSRINIT_1);
			}
			if (mi2526 & (1ULL<<XC6_ML_A_CY0_O5)) {
				cfg_ml.a2d[LUT_A].cy0 = CY0_O5;
				mi2526 &= ~(1ULL<<XC6_ML_A_CY0_O5);
			}
			if (mi2526 & (1ULL<<XC6_L_A_FFSRINIT_1)) {
				if (!l_col) {
					HERE();
					continue;
				}
				cfg_ml.a2d[LUT_A].ff_srinit = FF_SRINIT1;
				mi2526 &= ~(1ULL<<XC6_L_A_FFSRINIT_1);
			}

			//
			// Step 4:
			//
			// If any bits remain in the configuration, do not
			// instantiate the logic devices.
			//

		   	if (mi20) {
				fprintf(stderr, "#E %s:%i y%i x%i l%i "
				  "mi20 0x%016lX\n",
				  __FILE__, __LINE__, y, x, l_col, mi20);
				continue;
			}
		   	if (mi23_M) {
				fprintf(stderr, "#E %s:%i y%i x%i l%i "
				  "mi23_M 0x%016lX\n",
				  __FILE__, __LINE__, y, x, l_col, mi23_M);
				continue;
			}
		   	if (mi2526) {
				fprintf(stderr, "#E %s:%i y%i x%i l%i "
				  "mi2526 0x%016lX\n",
				  __FILE__, __LINE__, y, x, l_col, mi2526);
				continue;
			}

			//
			// Step 5:
			//
			// Do device-global sanity checking pre-LUT
			//

			// cfg_ml.cout_used
			dev_ml = fdev_p(es->model, y, x, DEV_LOGIC, DEV_LOG_M_OR_L);
			if (!dev_ml) FAIL(EINVAL);
			if (find_es_switch(es, y, x, fpga_switch_first(
				es->model, y, x, dev_ml->pinw[LO_COUT], SW_FROM)))
				cfg_ml.cout_used = 1;

// todo: if srinit=1, the matching ff/latch must be on
// todo: handle all_latch
// todo: precyinit=0 has no bits
// todo: cy0=X has no bits, check connectivity
// todo: the x-device ffmux must not be left as MUX_X unless the ff is really in use

			//
			// Step 6:
			//
			// Determine lut5_usage and read the luts.
			//

// todo: in ML, if srinit=0 cannot decide between ff_mux=O6 or
//    direct-out, need to check pin connectivity, is_pin_connected()
// todo: in ML devs, ffmux=O6 has no bits set
// todo: 5Q-ff in X devices

			// ML-A
			if (lut_ML[LUT_A]
			    || !all_zero(&cfg_ml.a2d[LUT_A], sizeof(cfg_ml.a2d[LUT_A]))) {
				if (lut_ML[LUT_A]
				    && cfg_ml.a2d[LUT_A].out_mux != MUX_O6
				    && cfg_ml.a2d[LUT_A].out_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_A].out_mux != MUX_CY
				    && cfg_ml.a2d[LUT_A].out_mux != MUX_F7
				    && cfg_ml.a2d[LUT_A].ff_mux != MUX_O6
				    && cfg_ml.a2d[LUT_A].ff_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_A].ff_mux != MUX_CY
				    && cfg_ml.a2d[LUT_A].ff_mux != MUX_F7)
					cfg_ml.a2d[LUT_A].out_used = 1;

				lut5_used = (cfg_ml.a2d[LUT_A].ff_mux == MUX_O5
					|| cfg_ml.a2d[LUT_A].out_mux == MUX_5Q
					|| cfg_ml.a2d[LUT_A].out_mux == MUX_O5
					|| cfg_ml.a2d[LUT_A].cy0 == CY0_O5);
				rc = lut2str(lut_ML[LUT_A], l_col
					  ? XC6_LMAP_XL_L_A : XC6_LMAP_XM_M_A,
					lut5_used,
					lut6_ml[LUT_A], &cfg_ml.a2d[LUT_A].lut6,
					lut5_ml[LUT_A], &cfg_ml.a2d[LUT_A].lut5);
				if (rc) FAIL(rc);
			}
			// ML-B
			if (lut_ML[LUT_B]
			    || !all_zero(&cfg_ml.a2d[LUT_B], sizeof(cfg_ml.a2d[LUT_B]))) {
				if (lut_ML[LUT_B]
				    && cfg_ml.a2d[LUT_B].out_mux != MUX_O6
				    && cfg_ml.a2d[LUT_B].out_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_B].out_mux != MUX_CY
				    && cfg_ml.a2d[LUT_B].out_mux != MUX_F7
				    && cfg_ml.a2d[LUT_B].ff_mux != MUX_O6
				    && cfg_ml.a2d[LUT_B].ff_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_B].ff_mux != MUX_CY
				    && cfg_ml.a2d[LUT_B].ff_mux != MUX_F7)
					cfg_ml.a2d[LUT_B].out_used = 1;

				lut5_used = (cfg_ml.a2d[LUT_B].ff_mux == MUX_O5
					|| cfg_ml.a2d[LUT_B].out_mux == MUX_5Q
					|| cfg_ml.a2d[LUT_B].out_mux == MUX_O5
					|| cfg_ml.a2d[LUT_B].cy0 == CY0_O5);
				rc = lut2str(lut_ML[LUT_B], l_col
					  ? XC6_LMAP_XL_L_B : XC6_LMAP_XM_M_B,
					lut5_used,
					lut6_ml[LUT_B], &cfg_ml.a2d[LUT_B].lut6,
					lut5_ml[LUT_B], &cfg_ml.a2d[LUT_B].lut5);
				if (rc) FAIL(rc);
			}
			// ML-C
			if (lut_ML[LUT_C]
			    || !all_zero(&cfg_ml.a2d[LUT_C], sizeof(cfg_ml.a2d[LUT_C]))) {
				if (lut_ML[LUT_C]
				    && cfg_ml.a2d[LUT_C].out_mux != MUX_O6
				    && cfg_ml.a2d[LUT_C].out_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_C].out_mux != MUX_CY
				    && cfg_ml.a2d[LUT_C].out_mux != MUX_F7
				    && cfg_ml.a2d[LUT_C].ff_mux != MUX_O6
				    && cfg_ml.a2d[LUT_C].ff_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_C].ff_mux != MUX_CY
				    && cfg_ml.a2d[LUT_C].ff_mux != MUX_F7)
					cfg_ml.a2d[LUT_C].out_used = 1;

				lut5_used = (cfg_ml.a2d[LUT_C].ff_mux == MUX_O5
					|| cfg_ml.a2d[LUT_C].out_mux == MUX_5Q
					|| cfg_ml.a2d[LUT_C].out_mux == MUX_O5
					|| cfg_ml.a2d[LUT_C].cy0 == CY0_O5);
				rc = lut2str(lut_ML[LUT_C], l_col
					  ? XC6_LMAP_XL_L_C : XC6_LMAP_XM_M_C,
					lut5_used,
					lut6_ml[LUT_C], &cfg_ml.a2d[LUT_C].lut6,
					lut5_ml[LUT_C], &cfg_ml.a2d[LUT_C].lut5);
				if (rc) FAIL(rc);
			}
			// ML-D
			if (lut_ML[LUT_D]
			    || !all_zero(&cfg_ml.a2d[LUT_D], sizeof(cfg_ml.a2d[LUT_D]))) {
				if (lut_ML[LUT_D]
				    && cfg_ml.a2d[LUT_D].out_mux != MUX_O6
				    && cfg_ml.a2d[LUT_D].out_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_D].out_mux != MUX_CY
				    && cfg_ml.a2d[LUT_D].out_mux != MUX_F7
				    && cfg_ml.a2d[LUT_D].ff_mux != MUX_O6
				    && cfg_ml.a2d[LUT_D].ff_mux != MUX_XOR
				    && cfg_ml.a2d[LUT_D].ff_mux != MUX_CY
				    && cfg_ml.a2d[LUT_D].ff_mux != MUX_F7)
					cfg_ml.a2d[LUT_D].out_used = 1;

				lut5_used = (cfg_ml.a2d[LUT_D].ff_mux == MUX_O5
					|| cfg_ml.a2d[LUT_D].out_mux == MUX_5Q
					|| cfg_ml.a2d[LUT_D].out_mux == MUX_O5
					|| cfg_ml.a2d[LUT_D].cy0 == CY0_O5);
				rc = lut2str(lut_ML[LUT_D], l_col
					  ? XC6_LMAP_XL_L_D : XC6_LMAP_XM_M_D,
					lut5_used,
					lut6_ml[LUT_D], &cfg_ml.a2d[LUT_D].lut6,
					lut5_ml[LUT_D], &cfg_ml.a2d[LUT_D].lut5);
				if (rc) FAIL(rc);
			}
			// X-A
			if (lut_X[LUT_A]
			    || !all_zero(&cfg_x.a2d[LUT_A], sizeof(cfg_x.a2d[LUT_A]))) {
				if (lut_X[LUT_A]
				    && cfg_x.a2d[LUT_A].ff_mux != MUX_O6)
					cfg_x.a2d[LUT_A].out_used = 1;
				lut5_used = cfg_x.a2d[LUT_A].out_mux != 0;
				rc = lut2str(lut_X[LUT_A], l_col
					  ? XC6_LMAP_XL_X_A : XC6_LMAP_XM_X_A,
					lut5_used,
					lut6_x[LUT_A], &cfg_x.a2d[LUT_A].lut6,
					lut5_x[LUT_A], &cfg_x.a2d[LUT_A].lut5);
				if (rc) FAIL(rc);
			
			}
			// X-B
			if (lut_X[LUT_B]
			    || !all_zero(&cfg_x.a2d[LUT_B], sizeof(cfg_x.a2d[LUT_B]))) {
				if (lut_X[LUT_B]
				    && cfg_x.a2d[LUT_B].ff_mux != MUX_O6)
					cfg_x.a2d[LUT_B].out_used = 1;
				lut5_used = cfg_x.a2d[LUT_B].out_mux != 0;
				rc = lut2str(lut_X[LUT_B], l_col
					  ? XC6_LMAP_XL_X_B : XC6_LMAP_XM_X_B,
					lut5_used,
					lut6_x[LUT_B], &cfg_x.a2d[LUT_B].lut6,
					lut5_x[LUT_B], &cfg_x.a2d[LUT_B].lut5);
				if (rc) FAIL(rc);
			
			}
			// X-C
			if (lut_X[LUT_C]
			    || !all_zero(&cfg_x.a2d[LUT_C], sizeof(cfg_x.a2d[LUT_C]))) {
				if (lut_X[LUT_C]
				    && cfg_x.a2d[LUT_C].ff_mux != MUX_O6)
					cfg_x.a2d[LUT_C].out_used = 1;
				lut5_used = cfg_x.a2d[LUT_C].out_mux != 0;
				rc = lut2str(lut_X[LUT_C], l_col
					  ? XC6_LMAP_XL_X_C : XC6_LMAP_XM_X_C,
					lut5_used,
					lut6_x[LUT_C], &cfg_x.a2d[LUT_C].lut6,
					lut5_x[LUT_C], &cfg_x.a2d[LUT_C].lut5);
				if (rc) FAIL(rc);
			
			}
			// X-D
			if (lut_X[LUT_D]
			    || !all_zero(&cfg_x.a2d[LUT_D], sizeof(cfg_x.a2d[LUT_D]))) {
				if (lut_X[LUT_D]
				    && cfg_x.a2d[LUT_D].ff_mux != MUX_O6)
					cfg_x.a2d[LUT_D].out_used = 1;
				lut5_used = cfg_x.a2d[LUT_D].out_mux != 0;
				rc = lut2str(lut_X[LUT_D], l_col
					  ? XC6_LMAP_XL_X_D : XC6_LMAP_XM_X_D,
					lut5_used,
					lut6_x[LUT_D], &cfg_x.a2d[LUT_D].lut6,
					lut5_x[LUT_D], &cfg_x.a2d[LUT_D].lut5);
				if (rc) FAIL(rc);
			
			}

			//
			// Step 7:
			//
			// Do more and final sanity checks before instantiation.
			//

			// todo: latch cannot be combined with out_mux=5Q or ff5_srinit
			if (latch_ml
			    && !cfg_ml.a2d[LUT_A].ff_mux
			    && !cfg_ml.a2d[LUT_B].ff_mux
			    && !cfg_ml.a2d[LUT_C].ff_mux
			    && !cfg_ml.a2d[LUT_D].ff_mux) {
				HERE();
				continue;
			}
			if (latch_x
			    && !cfg_x.a2d[LUT_A].ff_mux
			    && !cfg_x.a2d[LUT_B].ff_mux
			    && !cfg_x.a2d[LUT_C].ff_mux
			    && !cfg_x.a2d[LUT_D].ff_mux) {
				HERE();
				continue;
			}
			// todo: latch and2l and or2l need to be determined
			//       from vcc connectivity, srinit etc.
			for (lut = LUT_A; lut <= LUT_D; lut++) {
				if (cfg_ml.a2d[lut].ff_mux) {
					cfg_ml.a2d[lut].ff = latch_ml ? FF_LATCH : FF_FF;
					if (!cfg_ml.a2d[lut].ff_srinit)
						cfg_ml.a2d[lut].ff_srinit = FF_SRINIT0;
					if (!cfg_ml.clk_inv)
						cfg_ml.clk_inv = CLKINV_CLK;
					if (!cfg_ml.sync_attr)
						cfg_ml.sync_attr = SYNCATTR_ASYNC;
				}
				if (cfg_x.a2d[lut].ff_mux) {
					cfg_x.a2d[lut].ff = latch_x ? FF_LATCH : FF_FF;
					if (!cfg_x.a2d[lut].ff_srinit)
						cfg_x.a2d[lut].ff_srinit = FF_SRINIT0;
					if (!cfg_x.clk_inv)
						cfg_x.clk_inv = CLKINV_CLK;
					if (!cfg_x.sync_attr)
						cfg_x.sync_attr = SYNCATTR_ASYNC;
				}
				// todo: ff5_srinit
				// todo: 5Q ff also needs clock/sync check
			}

			// Check whether we should default to PRECYINIT_0
			if (!cfg_ml.precyinit
			    && (cfg_ml.a2d[LUT_A].out_mux == MUX_XOR
				|| cfg_ml.a2d[LUT_A].ff_mux == MUX_XOR
				|| cfg_ml.a2d[LUT_A].cy0)) {
				int connpt_dests_o, num_dests, cout_y, cout_x;
				str16_t cout_str;

				if ((fpga_connpt_find(es->model, y, x,
					dev_ml->pinw[LI_CIN], &connpt_dests_o,
					&num_dests) == NO_CONN)
				    || num_dests != 1) {
					HERE();
				} else {
					fpga_conn_dest(es->model, y, x, connpt_dests_o,
						&cout_y, &cout_x, &cout_str);
					if (find_es_switch(es, cout_y, cout_x,fpga_switch_first(
						es->model, cout_y, cout_x, cout_str, SW_TO)))
						cfg_ml.precyinit = PRECYINIT_0;
				}
			}

			//
			// Step 8:
			//
			// Remove all bits.
			//
			frame_set_u64(u8_p + 20*FRAME_SIZE + byte_off,
				frame_get_u64(u8_p + 20*FRAME_SIZE + byte_off)
					& ~XC6_MI20_LOGIC_MASK);
			last_minor = l_col ? 29 : 30;
			for (i = 21; i <= last_minor; i++)
				frame_set_u64(u8_p + i*FRAME_SIZE + byte_off, 0);
		
			//
			// Step 9:
			//
			// Instantiate configuration.
			//

			if (!all_zero(&cfg_ml, sizeof(cfg_ml))) {
				rc = fdev_logic_setconf(es->model, y, x, DEV_LOG_M_OR_L, &cfg_ml);
				if (rc) FAIL(rc);
			}
			if (!all_zero(&cfg_x, sizeof(cfg_x))) {
				rc = fdev_logic_setconf(es->model, y, x, DEV_LOG_X, &cfg_x);
				if (rc) FAIL(rc);
			}
		}
	}
	return 0;
fail:
	return rc;
}

static int bitpos_is_set(struct extract_state *es, int y, int x,
	struct xc6_routing_bitpos *swpos, int *is_set)
{
	int row_num, row_pos, start_in_frame, two_bits_val, rc;

	RC_CHECK(es->model);
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

	RC_CHECK(es->model);
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

	RC_CHECK(model);
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

	RC_CHECK(es->model);
	tile = YX_TILE(es->model, y, x);

	for (i = 0; i < es->model->num_bitpos; i++) {
		rc = bitpos_is_set(es, y, x, &es->model->sw_bitpos[i], &is_set);
		if (rc) RC_FAIL(es->model, rc);
		if (!is_set) continue;

		sw_idx = fpga_switch_lookup(es->model, y, x,
			fpga_wire2str_i(es->model, es->model->sw_bitpos[i].from),
			fpga_wire2str_i(es->model, es->model->sw_bitpos[i].to));
		if (sw_idx == NO_SWITCH) RC_FAIL(es->model, EINVAL);
		// todo: es->model->sw_bitpos[i].bidir handling

		if (tile->switches[sw_idx] & SWITCH_BIDIRECTIONAL)
			HERE();
		if (tile->switches[sw_idx] & SWITCH_USED)
			HERE();
		if (es->num_yx_pos >= MAX_YX_SWITCHES)
			{ RC_FAIL(es->model, ENOTSUP); }
		es->yx_pos[es->num_yx_pos].y = y;
		es->yx_pos[es->num_yx_pos].x = x;
		es->yx_pos[es->num_yx_pos].idx = sw_idx;
		es->num_yx_pos++;
		rc = bitpos_clear_bits(es, y, x, &es->model->sw_bitpos[i]);
		if (rc) RC_FAIL(es->model, rc);
	}
	RC_RETURN(es->model);
}

static int extract_logic_switches(struct extract_state* es, int y, int x)
{
	int row, row_pos, byte_off, minor, rc;
	uint8_t* u8_p;

	RC_CHECK(es->model);
	row = which_row(y, es->model);
	row_pos = pos_in_row(y, es->model);
	if (row == -1 || row_pos == -1 || row_pos == 8) FAIL(EINVAL);
	if (row_pos > 8) row_pos--;
	u8_p = get_first_minor(es->bits, row, es->model->x_major[x]);
	byte_off = row_pos * 8;
	if (row_pos >= 8) byte_off += XC6_HCLK_BYTES;

	if (has_device_type(es->model, y, x, DEV_LOGIC, LOGIC_M))
		minor = 26;
	else if (has_device_type(es->model, y, x, DEV_LOGIC, LOGIC_L))
		minor = 25;
	else
		FAIL(EINVAL);

	if (frame_get_bit(u8_p + minor*FRAME_SIZE, byte_off*8 + XC6_ML_CIN_USED)) {
		struct fpga_device* dev;
		int connpt_dests_o, num_dests, cout_y, cout_x;
		str16_t cout_str;
		swidx_t cout_sw;

		dev = fdev_p(es->model, y, x, DEV_LOGIC, DEV_LOG_M_OR_L);
		if (!dev) FAIL(EINVAL);

		if ((fpga_connpt_find(es->model, y, x,
			dev->pinw[LI_CIN], &connpt_dests_o,
			&num_dests) == NO_CONN)
		    || num_dests != 1) {
			HERE();
		} else {
			fpga_conn_dest(es->model, y, x, connpt_dests_o,
				&cout_y, &cout_x, &cout_str);
			cout_sw = fpga_switch_first(es->model, cout_y,
				cout_x, cout_str, SW_TO);
			if (cout_sw == NO_SWITCH) HERE();
			else {
				if (es->num_yx_pos >= MAX_YX_SWITCHES)
					{ FAIL(ENOTSUP); }
				es->yx_pos[es->num_yx_pos].y = cout_y;
				es->yx_pos[es->num_yx_pos].x = cout_x;
				es->yx_pos[es->num_yx_pos].idx = cout_sw;
				es->num_yx_pos++;

				frame_clear_bit(u8_p + minor*FRAME_SIZE,
					byte_off*8 + XC6_ML_CIN_USED);
			}
		}
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

// todo: can we assume the maximum range for iologic
//       minors to be 21..29? that would be a total of
//	 9*64=675 bits for ilogic/ologic/iodelay?
struct iologic_sw_pos s_left_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_right_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_top_outer_io_swpos[] = { {{0}} };
struct iologic_sw_pos s_top_inner_io_swpos[] = { {{0}} };

struct iologic_sw_pos s_bot_inner_io_swpos[] =
{
	// input
	{{"D_ILOGIC_IDATAIN_IODELAY_S"},
	 {"BIOI_INNER_IBUF0"},
 	 {23, 23, -1},
	 {28, 29}},

	{{"D_ILOGIC_SITE"},
	 {"D_ILOGIC_IDATAIN_IODELAY"},
	 {26, -1},
	 {20}},

	{{"D_ILOGIC_SITE_S"},
	 {"D_ILOGIC_IDATAIN_IODELAY_S"},
	 {26, -1},
	 {23}},

	{{"DFB_ILOGIC_SITE"},
	 {"D_ILOGIC_SITE"},
	 {28, -1},
	 {63}},

	{{"DFB_ILOGIC_SITE_S"},
	 {"D_ILOGIC_SITE_S"},
	 {28, -1},
	 {0}},

	{{"FABRICOUT_ILOGIC_SITE"},
	 {"D_ILOGIC_SITE"},
	 {29, -1},
	 {49}},

	{{"FABRICOUT_ILOGIC_SITE_S"},
	 {"D_ILOGIC_SITE_S"},
	 {29, -1},
	 {14}},

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
	{{"D_ILOGIC_IDATAIN_IODELAY_S"},
	 {"BIOI_OUTER_IBUF0"},
 	 {23, 23, -1},
	 {28, 29}},

	{{"D_ILOGIC_SITE"},
	 {"D_ILOGIC_IDATAIN_IODELAY"},
	 {26, -1},
	 {20}},

	{{"D_ILOGIC_SITE_S"},
	 {"D_ILOGIC_IDATAIN_IODELAY_S"},
	 {26, -1},
	 {23}},

	{{"DFB_ILOGIC_SITE"},
	 {"D_ILOGIC_SITE"},
	 {28, -1},
	 {63}},

	{{"DFB_ILOGIC_SITE_S"},
	 {"D_ILOGIC_SITE_S"},
	 {28, -1},
	 {0}},

	{{"FABRICOUT_ILOGIC_SITE"},
	 {"D_ILOGIC_SITE"},
	 {29, -1},
	 {49}},

	{{"FABRICOUT_ILOGIC_SITE_S"},
	 {"D_ILOGIC_SITE_S"},
	 {29, -1},
	 {14}},

	// output
	{{"OQ_OLOGIC_SITE", "BIOI_OUTER_O0"},
	 {"D1_OLOGIC_SITE", "OQ_OLOGIC_SITE"},
	 {26, 27, 28, -1},
	 {40, 21, 57}},

	{{"OQ_OLOGIC_SITE_S", "BIOI_OUTER_O1"},
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

static int add_yx_switch(struct extract_state *es,
	int y, int x, const char *from, const char *to)
{
	str16_t from_str_i, to_str_i;
	swidx_t sw_idx;

	RC_CHECK(es->model);

	from_str_i = strarray_find(&es->model->str, from);
	to_str_i = strarray_find(&es->model->str, to);
	RC_ASSERT(es->model, from_str_i != STRIDX_NO_ENTRY
		&& to_str_i != STRIDX_NO_ENTRY);

	sw_idx = fpga_switch_lookup(es->model, y, x,
		from_str_i, to_str_i);
	RC_ASSERT(es->model, sw_idx != NO_SWITCH);

	RC_ASSERT(es->model, es->num_yx_pos < MAX_YX_SWITCHES);
	es->yx_pos[es->num_yx_pos].y = y;
	es->yx_pos[es->num_yx_pos].x = x;
	es->yx_pos[es->num_yx_pos].idx = sw_idx;
	es->num_yx_pos++;

	RC_RETURN(es->model);
}

static int extract_iologic_switches(struct extract_state* es, int y, int x)
{
	int row_num, row_pos, bit_in_frame, i, j, rc;
	uint8_t* minor0_p;
	struct iologic_sw_pos* sw_pos;

	RC_CHECK(es->model);
	// From y/x coordinate, determine major, row, bit offset
	// in frame (v64_i) and pointer to first minor.
	is_in_row(es->model, y, &row_num, &row_pos);
	if (row_num == -1 || row_pos == -1
	    || row_pos == HCLK_POS) FAIL(EINVAL);
	if (row_pos > HCLK_POS)
		bit_in_frame = (row_pos-1)*64 + XC6_HCLK_BITS;
	else
		bit_in_frame = row_pos*64;
	minor0_p = get_first_minor(es->bits, row_num, es->model->x_major[x]);

	if (x < LEFT_SIDE_WIDTH) {
		if (x != LEFT_IO_DEVS) FAIL(EINVAL);
		sw_pos = s_left_io_swpos;
	} else if (x >= es->model->x_width-RIGHT_SIDE_WIDTH) {
		if (x != es->model->x_width - RIGHT_IO_DEVS_O) FAIL(EINVAL);
		sw_pos = s_right_io_swpos;
	} else if (y == TOP_OUTER_IO)
		sw_pos = s_top_outer_io_swpos;
	else if (y == TOP_INNER_IO)
		sw_pos = s_top_inner_io_swpos;
	else if (y == es->model->y_height-BOT_INNER_IO)
		sw_pos = s_bot_inner_io_swpos;
	else if (y == es->model->y_height-BOT_OUTER_IO)
		sw_pos = s_bot_outer_io_swpos;
	else FAIL(EINVAL);

	// Go through switches
	for (i = 0; sw_pos[i].to[0]; i++) {
		for (j = 0; sw_pos[i].minor[j] != -1; j++) {
			if (!frame_get_bit(&minor0_p[sw_pos[i].minor[j]
				*FRAME_SIZE], bit_in_frame+sw_pos[i].b64[j]))
				break;
		}
		if (!j || sw_pos[i].minor[j] != -1)
			continue;
		for (j = 0; j < MAX_IOLOGIC_SWBLOCK && sw_pos[i].to[j]; j++)
			add_yx_switch(es, y, x, sw_pos[i].from[j], sw_pos[i].to[j]);
		for (j = 0; sw_pos[i].minor[j] != -1; j++)
			frame_clear_bit(&minor0_p[sw_pos[i].minor[j]
			  *FRAME_SIZE], bit_in_frame+sw_pos[i].b64[j]);
	}
	// todo: we could implement an all-or-nothing system where
	//       the device bits are first copied into an u64 array,
	//	 and switches rewound by resetting num_yx_pos - unless
	//	 all bits have been processed...
	return 0;
fail:
	return rc;
}

static int extract_center_switches(struct extract_state *es)
{
	int center_row, center_major, word, i;
	uint8_t *minor_p;

	RC_CHECK(es->model);
	center_row = es->model->die->num_rows/2;
	center_major = xc_die_center_major(es->model->die);
	minor_p = get_first_minor(es->bits, center_row,
		center_major) + XC6_CENTER_GCLK_MINOR*FRAME_SIZE;
	word = frame_get_pinword(minor_p + 15*8+XC6_HCLK_BYTES);
	if (word) {
		for (i = 0; i < 16; i++) {
			if (!(word & (1<<i))) continue;
			add_yx_switch(es,
				es->model->center_y, es->model->center_x,
				pf("CLKC_CKLR%i", i), pf("CLKC_GCLK%i", i));
		}
		frame_set_pinword(minor_p + 15*8+XC6_HCLK_BYTES, 0);
	}
	word = frame_get_pinword(minor_p + 15*8+XC6_HCLK_BYTES + XC6_WORD_BYTES);
	if (word) {
		for (i = 0; i < 16; i++) {
			if (!(word & (1<<i))) continue;
			add_yx_switch(es,
				es->model->center_y, es->model->center_x,
				pf("CLKC_CKTB%i", i), pf("CLKC_GCLK%i", i));
		}
		frame_set_pinword(minor_p + 15*8+XC6_HCLK_BYTES + XC6_WORD_BYTES, 0);
	}
	RC_RETURN(es->model);
}

static int write_center_sw(struct fpga_bits *bits,
	struct fpga_model *model, int y, int x)
{
	struct fpga_tile *tile;
	const char *from_str, *to_str;
	int center_row, center_major, word;
	int i, j, gclk_pin_from, gclk_pin_to;
	uint8_t *minor_p;

	RC_CHECK(model);

	center_row = model->die->num_rows/2;
	center_major = xc_die_center_major(model->die);
	minor_p = get_first_minor(bits, center_row,
		center_major) + XC6_CENTER_GCLK_MINOR*FRAME_SIZE;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;

		from_str = fpga_switch_str(model, y, x, i, SW_FROM);
		to_str = fpga_switch_str(model, y, x, i, SW_TO);
		RC_ASSERT(model, from_str && to_str);

		j = sscanf(from_str, "CLKC_CKLR%i", &gclk_pin_from);
		if (j == 1) {
			j = sscanf(to_str, "CLKC_GCLK%i", &gclk_pin_to);
			RC_ASSERT(model, j == 1 && gclk_pin_to == gclk_pin_from);
			word = frame_get_pinword(minor_p + 15*8+XC6_HCLK_BYTES);
			word |= 1 << gclk_pin_from;
			frame_set_pinword(minor_p + 15*8+XC6_HCLK_BYTES, word);
			continue;
		}
		j = sscanf(from_str, "CLKC_CKTB%i", &gclk_pin_from);
		if (j == 1) {
			j = sscanf(to_str, "CLKC_GCLK%i", &gclk_pin_to);
			RC_ASSERT(model, j == 1 && gclk_pin_to == gclk_pin_from);
			word = frame_get_pinword(minor_p + 15*8+XC6_HCLK_BYTES + XC6_WORD_BYTES);
			word |= 1 << gclk_pin_from;
			frame_set_pinword(minor_p + 15*8+XC6_HCLK_BYTES + XC6_WORD_BYTES, word);
			continue;
		}
		// todo: it's possible that other switches need bits
		// todo: we can manually detect used switches that do not
		//       need bits, such as
		// I0_GCLK_SITE0:15 -> O_GCLK_SITE0:15
		// O_GCLK_SITE0:15 -> CLKC_GCLK_MAIN0:15
		// CLKC_GCLK%i -> I0_GCLK_SITE%i
	}
	RC_RETURN(model);
}

static int extract_gclk_center_vert_sw(struct extract_state *es)
{
	int word, cur_row, cur_minor, cur_pin, i, hclk_y;
	uint8_t *ma0_bits;

	RC_CHECK(es->model);
	// Switches in the vertical center column that are routing gclk
	// signals to the left and right side of the chip.
	for (cur_row = 0; cur_row < es->model->die->num_rows; cur_row++) {
		hclk_y = row_to_hclk(cur_row, es->model);
		RC_ASSERT(es->model, hclk_y != -1);
		ma0_bits = get_first_minor(es->bits, cur_row, XC6_NULL_MAJOR);
		for (cur_minor = 0; cur_minor <= 2; cur_minor++) {
			// left
			word = frame_get_pinword(ma0_bits + cur_minor*FRAME_SIZE + 8*8+XC6_HCLK_BYTES);
			if (word) {
				for (i = 0; i <= 16/3; i++) { // 0-5
					cur_pin = cur_minor + i*3;
					if (cur_pin > 15) break;
					if (!(word & (1<<cur_pin))) continue;
					add_yx_switch(es, hclk_y, es->model->center_x,
						pf("CLKV_GCLKH_MAIN%i_FOLD", cur_pin),
						pf("CLKV_GCLKH_L%i", cur_pin));
					word &= ~(1<<cur_pin);
				}
				if (word) HERE();
				frame_set_pinword(ma0_bits + cur_minor*FRAME_SIZE + 8*8+XC6_HCLK_BYTES, word);
			}
			// right
			word = frame_get_pinword(ma0_bits + cur_minor*FRAME_SIZE + 8*8+XC6_HCLK_BYTES + 4);
			if (word) {
				for (i = 0; i <= 16/3; i++) { // 0-5
					cur_pin = cur_minor + i*3;
					if (cur_pin > 15) break;
					if (!(word & (1<<cur_pin))) continue;
					add_yx_switch(es, hclk_y, es->model->center_x,
						pf("CLKV_GCLKH_MAIN%i_FOLD", cur_pin),
						pf("CLKV_GCLKH_R%i", cur_pin));
					word &= ~(1<<cur_pin);
				}
				if (word) HERE();
				frame_set_pinword(ma0_bits + cur_minor*FRAME_SIZE + 8*8+XC6_HCLK_BYTES + 4, word);
			}
		}
	}
	RC_RETURN(es->model);
}

static int write_gclk_center_vert_sw(struct fpga_bits *bits,
	struct fpga_model *model, int y, int x)
{
	struct fpga_tile *tile;
	const char *from_str, *to_str;
	int i, j, gclk_pin_from, gclk_pin_to, cur_pin, cur_minor, minor_found, word;
	uint8_t *ma0_bits;

	RC_CHECK(model);
	ma0_bits = get_first_minor(bits, which_row(y, model), XC6_NULL_MAJOR);
	RC_ASSERT(model, ma0_bits);
	tile = YX_TILE(model, y, x);

	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;

		from_str = fpga_switch_str(model, y, x, i, SW_FROM);
		to_str = fpga_switch_str(model, y, x, i, SW_TO);
		RC_ASSERT(model, from_str && to_str);

		// left
		j = sscanf(from_str, "CLKV_GCLKH_MAIN%i_FOLD", &gclk_pin_from);
		if (j == 1) {
			j = sscanf(to_str, "CLKV_GCLKH_L%i", &gclk_pin_to);
			RC_ASSERT(model, j == 1 && gclk_pin_to == gclk_pin_from);

			minor_found = -1;
			for (cur_minor = 0; minor_found == -1 && cur_minor <= 2; cur_minor++) {
				for (j = 0; j <= 16/3; j++) { // 0-5
					cur_pin = cur_minor + j*3;
					if (cur_pin > 15) break;
					if (cur_pin == gclk_pin_from) {
						minor_found = cur_minor;
						break;
					}
				}
			}
			if (minor_found != -1) {
				word = frame_get_pinword(ma0_bits + minor_found*FRAME_SIZE + 8*8+XC6_HCLK_BYTES);
				word |= 1 << gclk_pin_from;
				frame_set_pinword(ma0_bits + minor_found*FRAME_SIZE + 8*8+XC6_HCLK_BYTES, word);
				continue;
			}
			HERE(); // fall-through to unsupported
		}
		// right 
		j = sscanf(from_str, "CLKV_GCLKH_MAIN%i_FOLD", &gclk_pin_from);
		if (j == 1) {
			j = sscanf(to_str, "CLKV_GCLKH_R%i", &gclk_pin_to);
			RC_ASSERT(model, j == 1 && gclk_pin_to == gclk_pin_from);

			minor_found = -1;
			for (cur_minor = 0; minor_found == -1 && cur_minor <= 2; cur_minor++) {
				for (j = 0; j <= 16/3; j++) { // 0-5
					cur_pin = cur_minor + j*3;
					if (cur_pin > 15) break;
					if (cur_pin == gclk_pin_from) {
						minor_found = cur_minor;
						break;
					}
				}
			}
			if (minor_found != -1) {
				word = frame_get_pinword(ma0_bits + minor_found*FRAME_SIZE + 8*8+XC6_HCLK_BYTES + 4);
				word |= 1 << gclk_pin_from;
				frame_set_pinword(ma0_bits + minor_found*FRAME_SIZE + 8*8+XC6_HCLK_BYTES + 4, word);
				continue;
			}
			HERE(); // fall-through to unsupported
		}
		// todo: we can manually detect used switches that do not
		//       need bits, such as
		//	CLKV_GCLKH_L0:15 -> I_BUFH_LEFT_SITE0:15
		//	I_BUFH_LEFT_SITE0:15 -> O_BUFH_LEFT_SITE0:15
		// 	O_BUFH_LEFT_SITE0:15 -> CLKV_BUFH_LEFT_L0:15
	}
	RC_RETURN(model);
}

static int extract_gclk_hclk_updown_sw(struct extract_state *es)
{
	int word, cur_row, x, gclk_pin, hclk_y;
	uint8_t *mi0_bits;

	RC_CHECK(es->model);
	// Switches in each horizontal hclk row that are routing
	// gclk signals and and down to the clocked devices.

	// todo: clk access in left and right IO devs, center devs,
	// 	 bram and macc devs as well as special devs not tested
	//       yet (only XM logic devs tested).
	for (cur_row = 0; cur_row < es->model->die->num_rows; cur_row++) {
		hclk_y = row_to_hclk(cur_row, es->model);
		RC_ASSERT(es->model, hclk_y != -1);
		for (x = LEFT_IO_ROUTING; x <= es->model->x_width-RIGHT_IO_ROUTING_O; x++) {
			if (!is_atx(X_ROUTING_COL, es->model, x))
				continue;
			mi0_bits = get_first_minor(es->bits, cur_row, es->model->x_major[x]);
			// each minor (0:15) stores the configuration bits for one gclk
			// pin (in the hclk bytes of the minor)
			for (gclk_pin = 0; gclk_pin <= 15; gclk_pin++) {
				word = frame_get_pinword(mi0_bits + gclk_pin*FRAME_SIZE + XC6_HCLK_POS);
				if (word & (1<<XC6_HCLK_GCLK_UP_PIN)) {
					add_yx_switch(es, hclk_y, x,
						pf("HCLK_GCLK%i_INT", gclk_pin),
						pf("HCLK_GCLK_UP%i", gclk_pin));
					word &= ~(1<<XC6_HCLK_GCLK_UP_PIN);
				}
				if (word & (1<<XC6_HCLK_GCLK_DOWN_PIN)) {
					add_yx_switch(es, hclk_y, x,
						pf("HCLK_GCLK%i_INT", gclk_pin),
						pf("HCLK_GCLK%i", gclk_pin));
					word &= ~(1<<XC6_HCLK_GCLK_DOWN_PIN);
				}
				if (word) HERE();
				frame_set_pinword(mi0_bits + gclk_pin*FRAME_SIZE + XC6_HCLK_POS, word);
			}
		}
	}
	RC_RETURN(es->model);
}

static int write_hclk_sw(struct fpga_bits *bits, struct fpga_model *model,
	int y, int x)
{
	uint8_t *mi0_bits;
	struct fpga_tile *tile;
	const char *from_str, *to_str;
	int i, j, gclk_pin_from, gclk_pin_to, up, word;

	RC_CHECK(model);

	mi0_bits = get_first_minor(bits, which_row(y, model), model->x_major[x]);
	RC_ASSERT(model, mi0_bits);

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;
		from_str = fpga_switch_str(model, y, x, i, SW_FROM);
		to_str = fpga_switch_str(model, y, x, i, SW_TO);

		j = sscanf(from_str, "HCLK_GCLK%i_INT", &gclk_pin_from);
		RC_ASSERT(model, j == 1);
		j = sscanf(to_str, "HCLK_GCLK%i", &gclk_pin_to);
		if (j == 1)
			up = 0; // down
		else {
			j = sscanf(to_str, "HCLK_GCLK_UP%i", &gclk_pin_to);
			RC_ASSERT(model, j == 1);
			up = 1;
		}
		RC_ASSERT(model, gclk_pin_from == gclk_pin_to);

		word = frame_get_pinword(mi0_bits + gclk_pin_from*FRAME_SIZE + XC6_HCLK_POS);
		word |= 1 << (up ? XC6_HCLK_GCLK_UP_PIN : XC6_HCLK_GCLK_DOWN_PIN);
		frame_set_pinword(mi0_bits + gclk_pin_from*FRAME_SIZE + XC6_HCLK_POS, word);
	}
	RC_RETURN(model);
}

static int extract_switches(struct extract_state *es)
{
	int x, y;

	RC_CHECK(es->model);
	for (x = 0; x < es->model->x_width; x++) {
		for (y = 0; y < es->model->y_height; y++) {
			// routing switches
			if (is_atx(X_ROUTING_COL, es->model, x)
			    && y >= TOP_IO_TILES
			    && y < es->model->y_height-BOT_IO_TILES
			    && !is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					es->model, y)) {
				extract_routing_switches(es, y, x);
			}
			// logic switches
			if (has_device(es->model, y, x, DEV_LOGIC)) {
				extract_logic_switches(es, y, x);
			}
			// iologic switches
			if (has_device(es->model, y, x, DEV_ILOGIC)) {
				extract_iologic_switches(es, y, x);
			}
		}
	}
	extract_center_switches(es);
	extract_gclk_center_vert_sw(es);
	extract_gclk_hclk_updown_sw(es);
	RC_RETURN(es->model);
}

static int construct_extract_state(struct extract_state* es,
	struct fpga_model* model)
{
	RC_CHECK(model);
	memset(es, 0, sizeof(*es));
	es->model = model;
	return 0;
}

int extract_model(struct fpga_model* model, struct fpga_bits* bits)
{
	struct extract_state es;
	net_idx_t net_idx;
	int i, rc;

	RC_CHECK(model);
	rc = construct_extract_state(&es, model);
	if (rc) RC_FAIL(model, rc);
	es.bits = bits;
	for (i = 0; i < sizeof(s_default_bits)/sizeof(s_default_bits[0]); i++) {
		if (!get_bitp(bits, &s_default_bits[i]))
			RC_FAIL(model, EINVAL);
		clear_bitp(bits, &s_default_bits[i]);
	}

	rc = extract_switches(&es);
	if (rc) RC_FAIL(model, rc);
	rc = extract_type2(&es);
	if (rc) RC_FAIL(model, rc);
	rc = extract_logic(&es);
	if (rc) RC_FAIL(model, rc);

	// turn switches into nets
	if (model->nets)
		HERE(); // should be empty here
	for (i = 0; i < es.num_yx_pos; i++) {
		rc = fnet_new(model, &net_idx);
		if (rc) RC_FAIL(model, rc);
		rc = fnet_add_sw(model, net_idx, es.yx_pos[i].y,
			es.yx_pos[i].x, &es.yx_pos[i].idx, 1);
		if (rc) RC_FAIL(model, rc);
	}
	RC_RETURN(model);
}

int printf_swbits(struct fpga_model* model)
{
	char bit_str[129];
	int i, j, width;

	RC_CHECK(model);
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
	RC_RETURN(model);
}

static int find_bitpos(struct fpga_model* model, int y, int x, swidx_t sw)
{
	enum extra_wires from_w, to_w;
	const char* from_str, *to_str;
	int i;

	RC_CHECK(model);
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

	RC_CHECK(model);
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

	RC_CHECK(model);
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

	RC_CHECK(model);
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
			"y%i x%i %s -> %s\n", __FILE__, __LINE__,
			y, x, str_sw[i].from_str, str_sw[i].to_str);
	}
	free(str_sw);
	return 0;
fail:
	return rc;
}

static int write_inner_term_sw(struct fpga_bits *bits,
	struct fpga_model *model, int y, int x)
{
	struct fpga_tile *tile;
	const char *from_str, *from_found, *to_str, *to_found;
	int i, j, from_idx, to_idx;

	RC_CHECK(model);

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;

		from_str = fpga_switch_str(model, y, x, i, SW_FROM);
		to_str = fpga_switch_str(model, y, x, i, SW_TO);
		RC_ASSERT(model, from_str && to_str);

		if (strstr(from_str, "IBUF")
		    && strstr(to_str, "CLKPIN"))
			continue;
		if ((from_found = strstr(from_str, "CLKPIN"))
		    && (to_found = strstr(to_str, "CKPIN"))) {
			struct switch_to_yx_l2 switch_to_yx_l2;
			int t2_io_idx, iob_y, iob_x, iob_type_idx;
			struct fpga_device *iob_dev;

			from_idx = atoi(&from_found[6]);
			to_idx = atoi(&to_found[5]);
			RC_ASSERT(model, from_idx == to_idx);

			// follow switches backwards to IOB
			switch_to_yx_l2.l1.yx_req = YX_DEV_IOB;
			switch_to_yx_l2.l1.flags = SWTO_YX_DEF;
			switch_to_yx_l2.l1.model = model;
			switch_to_yx_l2.l1.y = y;
			switch_to_yx_l2.l1.x = x;
			switch_to_yx_l2.l1.start_switch = fpga_switch_str_i(model, y, x, i, SW_TO);
			switch_to_yx_l2.l1.from_to = SW_TO;
			fpga_switch_to_yx_l2(&switch_to_yx_l2);
			RC_ASSERT(model, switch_to_yx_l2.l1.set.len);

			// find matching gclk pin
			for (j = 0; j < model->die->num_gclk_pins; j++) {
				t2_io_idx = model->die->gclk_t2_io_idx[j];
				iob_y = model->die->t2_io[t2_io_idx].y;
				iob_x = model->die->t2_io[t2_io_idx].x;
				iob_type_idx = model->die->t2_io[t2_io_idx].type_idx;

				if (iob_y != switch_to_yx_l2.l1.dest_y
				    || iob_x != switch_to_yx_l2.l1.dest_x)
					continue;
				iob_dev = fdev_p(model, iob_y, iob_x, DEV_IOB, iob_type_idx);
				RC_ASSERT(model, iob_dev);

				if (fpga_switch_lookup(model, iob_y, iob_x,
					iob_dev->pinw[IOB_OUT_I],
					switch_to_yx_l2.l1.dest_connpt) != NO_SWITCH)
					break;
			}
			// set bit
			if (j < model->die->num_gclk_pins) {
				uint16_t u16;
				int bits_off;

				bits_off = IOB_DATA_START
					+ model->die->gclk_t2_switches[j]*XC6_WORD_BYTES
					+ XC6_TYPE2_GCLK_REG_SW/XC6_WORD_BITS;
				u16 = frame_get_u16(&bits->d[bits_off]);
				u16 |= 1<<(XC6_TYPE2_GCLK_REG_SW%XC6_WORD_BITS);
				frame_set_u16(&bits->d[bits_off], u16);
				continue;
			}
			// fall-through to unsupported
		}
		fprintf(stderr, "#E %s:%i unsupported switch "
			"y%i x%i %s -> %s\n", __FILE__, __LINE__,
			y, x, from_str, to_str);
	}
	RC_RETURN(model);
}

static int write_logic_sw(struct fpga_bits *bits,
	struct fpga_model *model, int y, int x)
{
	struct fpga_tile *tile;
	const char *from_str, *to_str;
	int i, row, row_pos, xm_col, byte_off, frame_off;
	uint64_t mi2526;
	uint8_t* u8_p;

	RC_CHECK(model);

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (!(tile->switches[i] & SWITCH_USED))
			continue;
		from_str = fpga_switch_str(model, y, x, i, SW_FROM);
		to_str = fpga_switch_str(model, y, x, i, SW_TO);
		RC_ASSERT(model, from_str && to_str);

		xm_col = has_device_type(model, y, x, DEV_LOGIC, LOGIC_M);
		if ((xm_col
		     && !strcmp(from_str, "M_COUT")
		     && !strcmp(to_str, "M_COUT_N"))
		    || (!xm_col
			&& !strcmp(from_str, "XL_COUT")
			&& !strcmp(to_str, "XL_COUT_N"))) {

			is_in_row(model, regular_row_up(y, model), &row, &row_pos);
			RC_ASSERT(model, row != -1 && row_pos != -1 && row_pos != HCLK_POS);
			if (row_pos > HCLK_POS) row_pos--;

			u8_p = get_first_minor(bits, row, model->x_major[x]);
			byte_off = row_pos * 8;
			if (row_pos >= 8) byte_off += XC6_HCLK_BYTES;

			frame_off = (xm_col?26:25)*FRAME_SIZE + byte_off;
			mi2526 = frame_get_u64(u8_p + frame_off);
			RC_ASSERT(model, !(mi2526 & (1ULL << XC6_ML_CIN_USED)));
			mi2526 |= 1ULL << XC6_ML_CIN_USED;
			frame_set_u64(u8_p + frame_off, mi2526);
		}
	}
	RC_RETURN(model);
}

static int write_switches(struct fpga_bits* bits, struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y, i;

	RC_CHECK(model);
	// We need to identify and take out each enabled switch, whether it
	// leads to enabled bits or not. That way we can print unsupported
	// switches at the end and keep our model alive and maintainable
	// over time.
	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);
			if (is_atx(X_ROUTING_COL, model, x)
			    && y >= TOP_IO_TILES
			    && y < model->y_height-BOT_IO_TILES
			    && !is_aty(Y_ROW_HORIZ_AXSYMM|Y_CHIP_HORIZ_REGS,
					model, y)) {
				write_routing_sw(bits, model, y, x);
				continue;
			}
			if (is_atyx(YX_DEV_ILOGIC, model, y, x)) {
				write_iologic_sw(bits, model, y, x);
				continue;
			}
			if (is_atyx(YX_DEV_LOGIC, model, y, x)) {
				write_logic_sw(bits, model, y, x);
				continue;
			}
			if (is_atyx(YX_DEV_IOB, model, y, x))
				continue;
			if (is_atx(X_ROUTING_COL, model, x)
			    && is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
				write_hclk_sw(bits, model, y, x);
				continue;
			}
			if (is_atyx(YX_INNER_TERM, model, y, x)) {
				write_inner_term_sw(bits, model, y, x);
				continue;
			}
			if (is_atyx(YX_CENTER, model, y, x)) {
				write_center_sw(bits, model, y, x);
				continue;
			}
			if (is_atx(X_CENTER_REGS_COL, model, x)) {
				if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y)) {
					write_gclk_center_vert_sw(bits, model, y, x);
					continue;
				}
				if (tile->flags & TF_CENTER_MIDBUF
				    || (y < model->center_y
				     && YX_TILE(model, y+1, x)->flags & TF_CENTER_MIDBUF)
				    || (y > model->center_y
				        && YX_TILE(model, y-1, x)->flags & TF_CENTER_MIDBUF))
					continue;
			}
			// print unsupported switches
			for (i = 0; i < tile->num_switches; i++) {
				if (!(tile->switches[i] & SWITCH_USED))
					continue;
				fprintf(stderr, "#E %s:%i unsupported switch "
					"y%i x%i %s\n", __FILE__, __LINE__,
					y, x,
					fpga_switch_print(model, y, x, i));
			}
		}
	}
	RC_RETURN(model);
}

static int is_latch(struct fpga_device *dev)
{
	int i;
	// todo: there are a lot more checks we can do to determine whether
	//       the entire device is properly configured as a latch or not...
	for (i = LUT_A; i <= LUT_D; i++) {
		if (dev->u.logic.a2d[i].ff == FF_LATCH
		    || dev->u.logic.a2d[i].ff == FF_OR2L
		    || dev->u.logic.a2d[i].ff == FF_AND2L)
			return 1;
	}
	return 0;
}

static int str2lut(uint64_t *lut, int lut_pos, const struct fpgadev_logic_a2d *a2d)
{
	int lut6_used, lut5_used, lut_map[64], rc;
	uint64_t u64;

	lut6_used = a2d->lut6 && a2d->lut6[0];
	lut5_used = a2d->lut5 && a2d->lut5[0];
	if (!lut6_used && !lut5_used)
		return 0;

	if (lut5_used) {
		if (!lut6_used) u64 = 0;
		else {
			rc = bool_str2bits(a2d->lut6, &u64, 32);
			if (rc) FAIL(rc);
			u64 <<= 32;
		}
		rc = bool_str2bits(a2d->lut5, &u64, 32);
		if (rc) FAIL(rc);
		xc6_lut_bitmap(lut_pos, &lut_map, 32);
	} else {
		// lut6_used only
		rc = bool_str2bits(a2d->lut6, &u64, 64);
		if (rc) FAIL(rc);
		xc6_lut_bitmap(lut_pos, &lut_map, 64);
	}
	*lut = map_bits(u64, 64, lut_map);
	return 0;
fail:
	return rc;
}
	
static int write_logic(struct fpga_bits* bits, struct fpga_model* model)
{
	int dev_idx, row, row_pos, xm_col;
	int x, y, byte_off, rc;
	uint64_t lut_X[4], lut_ML[4]; // LUT_A-LUT_D
	uint64_t mi20, mi23_M, mi2526;
	uint8_t* u8_p;
	struct fpga_device* dev_ml, *dev_x;

	RC_CHECK(model);
	for (x = LEFT_SIDE_WIDTH; x < model->x_width-RIGHT_SIDE_WIDTH; x++) {
		xm_col = is_atx(X_FABRIC_LOGIC_XM_COL, model, x);
		if (!xm_col && !is_atx(X_FABRIC_LOGIC_XL_COL, model, x))
			continue;
		for (y = TOP_IO_TILES; y < model->y_height - BOT_IO_TILES; y++) {
			if (!has_device(model, y, x, DEV_LOGIC))
				continue;

			is_in_row(model, y, &row, &row_pos);
			RC_ASSERT(model, row != -1 && row_pos != -1 && row_pos != HCLK_POS);
			if (row_pos > HCLK_POS) row_pos--;

			u8_p = get_first_minor(bits, row, model->x_major[x]);
			byte_off = row_pos * 8;
			if (row_pos >= 8) byte_off += XC6_HCLK_BYTES;

			//
			// 1) check current bits
			//

			mi20 = frame_get_u64(u8_p + 20*FRAME_SIZE + byte_off) & XC6_MI20_LOGIC_MASK;
			if (xm_col) {
				mi23_M = frame_get_u64(u8_p + 23*FRAME_SIZE + byte_off);
				mi2526 = frame_get_u64(u8_p + 26*FRAME_SIZE + byte_off);
				lut_ML[LUT_A] = frame_get_lut64(u8_p + 24*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_B] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_C] = frame_get_lut64(u8_p + 24*FRAME_SIZE, row_pos*2);
				lut_ML[LUT_D] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2);
				lut_X[LUT_A] = frame_get_lut64(u8_p + 27*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_B] = frame_get_lut64(u8_p + 29*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_C] = frame_get_lut64(u8_p + 27*FRAME_SIZE, row_pos*2);
				lut_X[LUT_D] = frame_get_lut64(u8_p + 29*FRAME_SIZE, row_pos*2);
			} else { // xl
				mi23_M = 0;
				mi2526 = frame_get_u64(u8_p + 25*FRAME_SIZE + byte_off);
				lut_ML[LUT_A] = frame_get_lut64(u8_p + 23*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_B] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1);
				lut_ML[LUT_C] = frame_get_lut64(u8_p + 23*FRAME_SIZE, row_pos*2);
				lut_ML[LUT_D] = frame_get_lut64(u8_p + 21*FRAME_SIZE, row_pos*2);
				lut_X[LUT_A] = frame_get_lut64(u8_p + 26*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_B] = frame_get_lut64(u8_p + 28*FRAME_SIZE, row_pos*2+1);
				lut_X[LUT_C] = frame_get_lut64(u8_p + 26*FRAME_SIZE, row_pos*2);
				lut_X[LUT_D] = frame_get_lut64(u8_p + 28*FRAME_SIZE, row_pos*2);
			}
			// Except for XC6_ML_CIN_USED (which is set by a switch elsewhere),
			// everything else should be 0.
			if (mi20 || mi23_M || (mi2526 & ~(1ULL<<XC6_ML_CIN_USED))
			    || lut_ML[LUT_A] || lut_ML[LUT_B] || lut_ML[LUT_C] || lut_ML[LUT_D]
			    || lut_X[LUT_A] || lut_X[LUT_B] || lut_X[LUT_C] || lut_X[LUT_D]) {
				HERE();
				continue;
			}

			//
			// 2) go through config to assemble bits
			//

			dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOG_X);
			dev_x = FPGA_DEV(model, y, x, dev_idx);
			RC_ASSERT(model, dev_x);
			dev_idx = fpga_dev_idx(model, y, x, DEV_LOGIC, DEV_LOG_M_OR_L);
			dev_ml = FPGA_DEV(model, y, x, dev_idx);
			RC_ASSERT(model, dev_ml);

			//
			// 2.1) mi20
			//

			// X device
			if (dev_x->instantiated) {
				if (dev_x->u.logic.a2d[LUT_A].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_X_A5_FFSRINIT_1;
				if (dev_x->u.logic.a2d[LUT_B].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_X_B5_FFSRINIT_1;
				if (dev_x->u.logic.a2d[LUT_C].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_X_C5_FFSRINIT_1;
				if (dev_x->u.logic.a2d[LUT_D].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_X_D5_FFSRINIT_1;

				if (dev_x->u.logic.a2d[LUT_C].ff_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_X_C_FFSRINIT_1;
			}

			// M or L device
			if (dev_ml->instantiated) {
				if (dev_ml->u.logic.a2d[LUT_A].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_ML_A5_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_B].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_ML_B5_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_C].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_ML_C5_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_D].ff5_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_ML_D5_FFSRINIT_1;

				if (xm_col // M-device only
				    && dev_ml->u.logic.a2d[LUT_A].ff_srinit == FF_SRINIT1)
					mi20 |= 1ULL << XC6_M_A_FFSRINIT_1;
			}

			//
			// 2.2) mi2526
			//

			// X device
			if (dev_x->instantiated) {
				if (dev_x->u.logic.a2d[LUT_D].out_mux != MUX_5Q)
					mi2526 |= 1ULL << XC6_X_D_OUTMUX_O5; // default-set
				if (dev_x->u.logic.a2d[LUT_C].out_mux != MUX_5Q)
					mi2526 |= 1ULL << XC6_X_C_OUTMUX_O5; // default-set
				if (dev_x->u.logic.a2d[LUT_D].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_X_D_FFSRINIT_1;
				if (dev_x->u.logic.a2d[LUT_B].out_mux != MUX_5Q)
					mi2526 |= 1ULL << XC6_X_B_OUTMUX_O5; // default-set
				if (dev_x->u.logic.clk_inv == CLKINV_B)
					mi2526 |= 1ULL << XC6_X_CLK_B;
				if (dev_x->u.logic.a2d[LUT_D].ff_mux != MUX_O6)
					mi2526 |= 1ULL << XC6_X_D_FFMUX_X; // default-set
				if (dev_x->u.logic.a2d[LUT_C].ff_mux != MUX_O6)
					mi2526 |= 1ULL << XC6_X_C_FFMUX_X; // default-set
				if (dev_x->u.logic.ce_used)
					mi2526 |= 1ULL << XC6_X_CE_USED;
				if (dev_x->u.logic.a2d[LUT_B].ff_mux != MUX_O6)
					mi2526 |= 1ULL << XC6_X_B_FFMUX_X; // default-set
				if (dev_x->u.logic.a2d[LUT_A].ff_mux != MUX_O6)
					mi2526 |= 1ULL << XC6_X_A_FFMUX_X; // default-set
				if (dev_x->u.logic.a2d[LUT_B].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_X_B_FFSRINIT_1;
				if (dev_x->u.logic.a2d[LUT_A].out_mux != MUX_5Q)
					mi2526 |= 1ULL << XC6_X_A_OUTMUX_O5; // default-set
				if (dev_x->u.logic.sr_used)
					mi2526 |= 1ULL << XC6_X_SR_USED;
				if (dev_x->u.logic.sync_attr == SYNCATTR_SYNC)
					mi2526 |= 1ULL << XC6_X_SYNC;
				if (is_latch(dev_x))
					mi2526 |= 1ULL << XC6_X_ALL_LATCH;
				if (dev_x->u.logic.a2d[LUT_A].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_X_A_FFSRINIT_1;
			}

			// M or L device
			if (dev_ml->instantiated) {
				if (dev_ml->u.logic.a2d[LUT_D].cy0 == CY0_O5)
					mi2526 |= 1ULL << XC6_ML_D_CY0_O5;
				if (dev_ml->u.logic.a2d[LUT_D].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_ML_D_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_C].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_ML_C_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_C].cy0 == CY0_O5)
					mi2526 |= 1ULL << XC6_ML_C_CY0_O5;
				switch (dev_ml->u.logic.a2d[LUT_D].out_mux) {
					case MUX_O6: mi2526 |= XC6_ML_D_OUTMUX_O6 << XC6_ML_D_OUTMUX_O; break;
					case MUX_XOR: mi2526 |= XC6_ML_D_OUTMUX_XOR << XC6_ML_D_OUTMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_D_OUTMUX_O5 << XC6_ML_D_OUTMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_D_OUTMUX_CY << XC6_ML_D_OUTMUX_O; break;
					case MUX_5Q: mi2526 |= XC6_ML_D_OUTMUX_5Q << XC6_ML_D_OUTMUX_O; break;
				}
				switch (dev_ml->u.logic.a2d[LUT_D].ff_mux) {
					// XC6_ML_D_FFMUX_O6 is 0
					case MUX_O5: mi2526 |= XC6_ML_D_FFMUX_O5 << XC6_ML_D_FFMUX_O; break;
					case MUX_X: mi2526 |= XC6_ML_D_FFMUX_X << XC6_ML_D_FFMUX_O; break;
					case MUX_XOR: mi2526 |= XC6_ML_D_FFMUX_XOR << XC6_ML_D_FFMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_D_FFMUX_CY << XC6_ML_D_FFMUX_O; break;
				}
				if (is_latch(dev_ml))
					mi2526 |= 1ULL << XC6_ML_ALL_LATCH;
				if (dev_ml->u.logic.sr_used)
					mi2526 |= 1ULL << XC6_ML_SR_USED;
				if (dev_ml->u.logic.sync_attr == SYNCATTR_SYNC)
					mi2526 |= 1ULL << XC6_ML_SYNC;
				if (dev_ml->u.logic.ce_used)
					mi2526 |= 1ULL << XC6_ML_CE_USED;
				switch (dev_ml->u.logic.a2d[LUT_C].out_mux) {
					case MUX_XOR: mi2526 |= XC6_ML_C_OUTMUX_XOR << XC6_ML_C_OUTMUX_O; break;
					case MUX_O6: mi2526 |= XC6_ML_C_OUTMUX_O6 << XC6_ML_C_OUTMUX_O; break;
					case MUX_5Q: mi2526 |= XC6_ML_C_OUTMUX_5Q << XC6_ML_C_OUTMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_C_OUTMUX_CY << XC6_ML_C_OUTMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_C_OUTMUX_O5 << XC6_ML_C_OUTMUX_O; break;
					case MUX_F7: mi2526 |= XC6_ML_C_OUTMUX_F7 << XC6_ML_C_OUTMUX_O; break;
				}
				switch (dev_ml->u.logic.a2d[LUT_C].ff_mux) {
					// XC6_ML_C_FFMUX_O6 is 0
					case MUX_O5: mi2526 |= XC6_ML_C_FFMUX_O5 << XC6_ML_C_FFMUX_O; break;
					case MUX_X: mi2526 |= XC6_ML_C_FFMUX_X << XC6_ML_C_FFMUX_O; break;
					case MUX_F7: mi2526 |= XC6_ML_C_FFMUX_F7 << XC6_ML_C_FFMUX_O; break;
					case MUX_XOR: mi2526 |= XC6_ML_C_FFMUX_XOR << XC6_ML_C_FFMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_C_FFMUX_CY << XC6_ML_C_FFMUX_O; break;
				}
				switch (dev_ml->u.logic.a2d[LUT_B].out_mux) {
					case MUX_5Q: mi2526 |= XC6_ML_B_OUTMUX_5Q << XC6_ML_B_OUTMUX_O; break;
					case MUX_F8: mi2526 |= XC6_ML_B_OUTMUX_F8 << XC6_ML_B_OUTMUX_O; break;
					case MUX_XOR: mi2526 |= XC6_ML_B_OUTMUX_XOR << XC6_ML_B_OUTMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_B_OUTMUX_CY << XC6_ML_B_OUTMUX_O; break;
					case MUX_O6: mi2526 |= XC6_ML_B_OUTMUX_O6 << XC6_ML_B_OUTMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_B_OUTMUX_O5 << XC6_ML_B_OUTMUX_O; break;
				}
				if (dev_ml->u.logic.clk_inv == CLKINV_B)
					mi2526 |= 1ULL << XC6_ML_CLK_B;
				switch (dev_ml->u.logic.a2d[LUT_B].ff_mux) {
					// XC6_ML_B_FFMUX_O6 is 0
					case MUX_XOR: mi2526 |= XC6_ML_B_FFMUX_XOR << XC6_ML_B_FFMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_B_FFMUX_O5 << XC6_ML_B_FFMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_B_FFMUX_CY << XC6_ML_B_FFMUX_O; break;
					case MUX_X: mi2526 |= XC6_ML_B_FFMUX_X << XC6_ML_B_FFMUX_O; break;
					case MUX_F8: mi2526 |= XC6_ML_B_FFMUX_F8 << XC6_ML_B_FFMUX_O; break;
				}
				switch (dev_ml->u.logic.a2d[LUT_A].ff_mux) {
					// XC6_ML_A_FFMUX_O6 is 0
					case MUX_XOR: mi2526 |= XC6_ML_A_FFMUX_XOR << XC6_ML_A_FFMUX_O; break;
					case MUX_X: mi2526 |= XC6_ML_A_FFMUX_X << XC6_ML_A_FFMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_A_FFMUX_O5 << XC6_ML_A_FFMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_A_FFMUX_CY << XC6_ML_A_FFMUX_O; break;
					case MUX_F7: mi2526 |= XC6_ML_A_FFMUX_F7 << XC6_ML_A_FFMUX_O; break;
				}
				switch (dev_ml->u.logic.a2d[LUT_A].out_mux) {
					case MUX_5Q: mi2526 |= XC6_ML_A_OUTMUX_5Q << XC6_ML_A_OUTMUX_O; break;
					case MUX_F7: mi2526 |= XC6_ML_A_OUTMUX_F7 << XC6_ML_A_OUTMUX_O; break;
					case MUX_XOR: mi2526 |= XC6_ML_A_OUTMUX_XOR << XC6_ML_A_OUTMUX_O; break;
					case MUX_CY: mi2526 |= XC6_ML_A_OUTMUX_CY << XC6_ML_A_OUTMUX_O; break;
					case MUX_O6: mi2526 |= XC6_ML_A_OUTMUX_O6 << XC6_ML_A_OUTMUX_O; break;
					case MUX_O5: mi2526 |= XC6_ML_A_OUTMUX_O5 << XC6_ML_A_OUTMUX_O; break;
				}
				if (dev_ml->u.logic.a2d[LUT_B].cy0 == CY0_O5)
					mi2526 |= 1ULL << XC6_ML_B_CY0_O5;
				if (dev_ml->u.logic.precyinit == PRECYINIT_AX)
					mi2526 |= 1ULL << XC6_ML_PRECYINIT_AX;
				else if (dev_ml->u.logic.precyinit == PRECYINIT_1)
					mi2526 |= 1ULL << XC6_ML_PRECYINIT_1;
				if (dev_ml->u.logic.a2d[LUT_B].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_ML_B_FFSRINIT_1;
				if (dev_ml->u.logic.a2d[LUT_A].cy0 == CY0_O5)
					mi2526 |= 1ULL << XC6_ML_A_CY0_O5;
				if (!xm_col && dev_ml->u.logic.a2d[LUT_A].ff_srinit == FF_SRINIT1)
					mi2526 |= 1ULL << XC6_L_A_FFSRINIT_1;
			}

			//
			// 2.3) luts
			//

			// X device
			if (dev_x->instantiated) {
				rc = str2lut(&lut_X[LUT_A], xm_col
					? XC6_LMAP_XM_X_A : XC6_LMAP_XL_X_A,
					&dev_x->u.logic.a2d[LUT_A]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_X[LUT_B], xm_col
					? XC6_LMAP_XM_X_B : XC6_LMAP_XL_X_B,
					&dev_x->u.logic.a2d[LUT_B]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_X[LUT_C], xm_col
					? XC6_LMAP_XM_X_C : XC6_LMAP_XL_X_C,
					&dev_x->u.logic.a2d[LUT_C]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_X[LUT_D], xm_col
					? XC6_LMAP_XM_X_D : XC6_LMAP_XL_X_D,
					&dev_x->u.logic.a2d[LUT_D]);
				RC_ASSERT(model, !rc);
			}
			// M or L device
			if (dev_ml->instantiated) {
				rc = str2lut(&lut_ML[LUT_A], xm_col
					? XC6_LMAP_XM_M_A : XC6_LMAP_XL_L_A,
					&dev_ml->u.logic.a2d[LUT_A]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_ML[LUT_B], xm_col
					? XC6_LMAP_XM_M_B : XC6_LMAP_XL_L_B,
					&dev_ml->u.logic.a2d[LUT_B]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_ML[LUT_C], xm_col
					? XC6_LMAP_XM_M_C : XC6_LMAP_XL_L_C,
					&dev_ml->u.logic.a2d[LUT_C]);
				RC_ASSERT(model, !rc);
				rc = str2lut(&lut_ML[LUT_D], xm_col
					? XC6_LMAP_XM_M_D : XC6_LMAP_XL_L_D,
					&dev_ml->u.logic.a2d[LUT_D]);
				RC_ASSERT(model, !rc);
			}

			//
			// 3) write bits
			//

			// logic devs occupy only some bits in mi20, so we merge
			// with the others (switches).
			frame_set_u64(u8_p + 20*FRAME_SIZE + byte_off,
				(frame_get_u64(u8_p + 20*FRAME_SIZE + byte_off) & ~XC6_MI20_LOGIC_MASK)
					| mi20);
			if (xm_col) {
				frame_set_u64(u8_p + 23*FRAME_SIZE + byte_off, mi23_M);
				frame_set_u64(u8_p + 26*FRAME_SIZE + byte_off, mi2526);
				frame_set_lut64(u8_p + 24*FRAME_SIZE, row_pos*2+1, lut_ML[LUT_A]);
				frame_set_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1, lut_ML[LUT_B]);
				frame_set_lut64(u8_p + 24*FRAME_SIZE, row_pos*2, lut_ML[LUT_C]);
				frame_set_lut64(u8_p + 21*FRAME_SIZE, row_pos*2, lut_ML[LUT_D]);
				frame_set_lut64(u8_p + 27*FRAME_SIZE, row_pos*2+1, lut_X[LUT_A]);
				frame_set_lut64(u8_p + 29*FRAME_SIZE, row_pos*2+1, lut_X[LUT_B]);
				frame_set_lut64(u8_p + 27*FRAME_SIZE, row_pos*2, lut_X[LUT_C]);
				frame_set_lut64(u8_p + 29*FRAME_SIZE, row_pos*2, lut_X[LUT_D]);
			} else { // xl
				// mi23 is unused in xl cols - no need to write it
				RC_ASSERT(model, !mi23_M);
				frame_set_u64(u8_p + 25*FRAME_SIZE + byte_off, mi2526);
				frame_set_lut64(u8_p + 23*FRAME_SIZE, row_pos*2+1, lut_ML[LUT_A]);
				frame_set_lut64(u8_p + 21*FRAME_SIZE, row_pos*2+1, lut_ML[LUT_B]);
				frame_set_lut64(u8_p + 23*FRAME_SIZE, row_pos*2, lut_ML[LUT_C]);
				frame_set_lut64(u8_p + 21*FRAME_SIZE, row_pos*2, lut_ML[LUT_D]);
				frame_set_lut64(u8_p + 26*FRAME_SIZE, row_pos*2+1, lut_X[LUT_A]);
				frame_set_lut64(u8_p + 28*FRAME_SIZE, row_pos*2+1, lut_X[LUT_B]);
				frame_set_lut64(u8_p + 26*FRAME_SIZE, row_pos*2, lut_X[LUT_C]);
				frame_set_lut64(u8_p + 28*FRAME_SIZE, row_pos*2, lut_X[LUT_D]);
			}
		}
	}
	RC_RETURN(model);
}

int write_model(struct fpga_bits* bits, struct fpga_model* model)
{
	int i;

	RC_CHECK(model);

	for (i = 0; i < sizeof(s_default_bits)/sizeof(s_default_bits[0]); i++)
		set_bitp(bits, &s_default_bits[i]);
	write_switches(bits, model);
	write_type2(bits, model);
	write_logic(bits, model);

	RC_RETURN(model);
}
