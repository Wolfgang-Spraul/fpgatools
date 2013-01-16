//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "model.h"
#include "control.h"
 
#undef DBG_ENUM_SWITCH
#undef DBG_SWITCH_CONNS
#undef DBG_SWITCH_TO_YX
#undef DBG_SWITCH_TO_REL
#undef DBG_SWITCH_2SETS

int fpga_find_iob(struct fpga_model *model, const char *sitename,
	int *y, int *x, dev_type_idx_t *idx)
{
	int i, j;

	RC_CHECK(model);
	for (i = 0; i < model->pkg->num_pins; i++) {
		if (!strcmp(model->pkg->pin[i].name, sitename))
			break;
	}
	if (i >= model->pkg->num_pins) {
		HERE();
		return -1;
	}
	for (j = 0; j < model->die->num_t2_ios; j++) {
		if (!model->die->t2_io[j].pair)
			continue;
		if (model->die->t2_io[j].pair == model->pkg->pin[i].pair
		    && model->die->t2_io[j].pos_side == model->pkg->pin[i].pos_side
		    && model->die->t2_io[j].bank == model->pkg->pin[i].bank)
			break;
	}
	if (j >= model->die->num_t2_ios) {
		HERE();
		return -1;
	}
	*y = model->die->t2_io[j].y;
	*x = model->die->t2_io[j].x;
	*idx = model->die->t2_io[j].type_idx;
	return 0;
}

const char *fpga_iob_sitename(struct fpga_model *model,
	int y, int x, dev_type_idx_t type_idx)
{
	int i, j;

	for (i = 0; i < model->die->num_t2_ios; i++) {
		if (!model->die->t2_io[i].pair)
			continue;
		if (model->die->t2_io[i].y == y
		    && model->die->t2_io[i].x == x
		    && model->die->t2_io[i].type_idx == type_idx)
			break;
	}
	if (i >= model->die->num_t2_ios) {
		HERE();
		return 0;
	}
	for (j = 0; j < model->pkg->num_pins; j++) {
		if (model->pkg->pin[j].bank == model->die->t2_io[i].bank
		    && model->pkg->pin[j].pair == model->die->t2_io[i].pair
		    && model->pkg->pin[j].pos_side == model->die->t2_io[i].pos_side)
			break;
	}
	if (j >= model->pkg->num_pins) {
		HERE();
		return 0;
	}
	return model->pkg->pin[j].name;
}

static void enum_x(struct fpga_model *model, enum fpgadev_type type,
	int enum_i, int *y, int x, int *type_idx)
{
	int type_count, i, _y;
	struct fpga_tile* tile;

	type_count = 0;
	for (_y = 0; _y < model->y_height; _y++) {
		tile = YX_TILE(model, _y, x);
		for (i = 0; i < tile->num_devs; i++) {
			if (tile->devs[i].type != type)
				continue;
			if (type_count == enum_i) {
				*y = _y;
				*type_idx = type_count;
				return;
			}
			type_count++;
		}
	}
	*y = -1;
}

int fdev_enum(struct fpga_model* model, enum fpgadev_type type, int enum_i,
	int *y, int *x, int *type_idx)
{
	struct fpga_tile* tile;
	int i, j, type_count;

	RC_CHECK(model);
	switch (type) {
		case DEV_BUFGMUX:
			tile = YX_TILE(model, model->center_y, model->center_x);
			if (!tile) RC_FAIL(model, EINVAL);
			type_count = 0;
			for (i = 0; i < tile->num_devs; i++) {
				if (tile->devs[i].type != DEV_BUFGMUX)
					continue;
				if (type_count == enum_i) {
					*y = model->center_y;
					*x = model->center_x;
					*type_idx = type_count;
					RC_RETURN(model);
				}
				type_count++;
			}
			*y = -1;
			RC_RETURN(model);
		case DEV_BUFIO: {
			int yx_pairs[] = {
			  TOP_OUTER_ROW, model->center_x-CENTER_CMTPLL_O,
			  model->center_y, LEFT_OUTER_COL,
			  model->center_y, model->x_width-RIGHT_OUTER_O,
			  model->y_height-BOT_OUTER_ROW, model->center_x-CENTER_CMTPLL_O };
		
			type_count = 0;
			for (i = 0; i < sizeof(yx_pairs)/sizeof(*yx_pairs)/2; i++) {
				tile = YX_TILE(model, yx_pairs[i*2], yx_pairs[i*2+1]);
				for (j = 0; j < tile->num_devs; j++) {
					if (tile->devs[j].type != DEV_BUFIO)
						continue;
					if (type_count == enum_i) {
						*y = yx_pairs[i*2];
						*x = yx_pairs[i*2+1];
						*type_idx = type_count;
						RC_RETURN(model);
					}
					type_count++;
				}
			}
			*y = -1;
			RC_RETURN(model);
		}
		case DEV_PLL:
		case DEV_DCM:
			enum_x(model, type, enum_i, y, model->center_x
				- CENTER_CMTPLL_O, type_idx);
			RC_RETURN(model);
		case DEV_BSCAN:
			enum_x(model, type, enum_i, y, model->x_width
				- RIGHT_IO_DEVS_O, type_idx);
			RC_RETURN(model);
		default: break;
	}
	HERE();
	*y = -1;
	RC_RETURN(model);
}

static const char* dev_str[] = FPGA_DEV_STR;

const char* fdev_type2str(enum fpgadev_type type)
{
	if (type < 0 || type >= sizeof(dev_str)/sizeof(*dev_str))
		{ HERE(); return 0; }
	return dev_str[type];
}

enum fpgadev_type fdev_str2type(const char* str, int len)
{
	int i;
	for (i = 0; i < sizeof(dev_str)/sizeof(*dev_str); i++) {
		if (dev_str[i]
		    && strlen(dev_str[i]) == len
		    && !str_cmp(dev_str[i], len, str, len))
			return i;
	}
	return DEV_NONE;
}

struct fpga_device* fdev_p(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx)
{
	dev_idx_t dev_idx = fpga_dev_idx(model, y, x, type, type_idx);
	if (dev_idx == NO_DEV) {
		fprintf(stderr, "#E %s:%i fdev_p() y%i x%i type %i/%i not"
			" found\n", __FILE__, __LINE__, y, x, type, type_idx);
		return 0;
	}
	return FPGA_DEV(model, y, x, dev_idx);
}

dev_idx_t fpga_dev_idx(struct fpga_model* model,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx)
{
	struct fpga_tile* tile;
	dev_type_idx_t type_count;
	dev_idx_t i;

	// function must work even if model->rc is set
	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == type) {
			if (type_count == type_idx)
				return i;
			type_count++;
		}
	}
	return NO_DEV;
}

dev_type_idx_t fdev_typeidx(struct fpga_model* model, int y, int x,
	dev_idx_t dev_idx)
{
	struct fpga_tile* tile;
	dev_type_idx_t type_count, i;

	// function must work even if model->rc is set
	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < dev_idx; i++) {
		if (tile->devs[i].type == tile->devs[dev_idx].type)
			type_count++;
	}
	return type_count;
}

static const char* iob_pinw_str[] = IOB_PINW_STR;
static const char* logic_pinw_str[] = LOGIC_PINW_STR;

pinw_idx_t fdev_pinw_str2idx(int devtype, const char* str, int len)
{
	int i;

	if (devtype == DEV_IOB) {
		for (i = 0; i < sizeof(iob_pinw_str)/sizeof(*iob_pinw_str); i++) {
			if (strlen(iob_pinw_str[i]) == len
			    && !str_cmp(iob_pinw_str[i], len, str, len))
				return i;
		}
		HERE();
		return PINW_NO_IDX;
	}
	if (devtype == DEV_LOGIC) {
		for (i = 0; i < sizeof(logic_pinw_str)/sizeof(*logic_pinw_str); i++) {
			if (strlen(logic_pinw_str[i]) == len
			    && !str_cmp(logic_pinw_str[i], len, str, len))
				return i;
		}
		HERE();
		return PINW_NO_IDX;
	}
	HERE();
	return PINW_NO_IDX;
}

const char* fdev_pinw_idx2str(int devtype, pinw_idx_t idx)
{
	if (devtype == DEV_IOB) {
		if (idx < 0 || idx >= sizeof(iob_pinw_str)/sizeof(*iob_pinw_str)) {
			HERE();
			return 0;
		}
		return iob_pinw_str[idx];
	}
	if (devtype == DEV_LOGIC) {
		if (idx < 0 || idx >= sizeof(logic_pinw_str)/sizeof(*logic_pinw_str)) {
			HERE();
			return 0;
		}
		return logic_pinw_str[idx];
	}
	HERE();
	return 0;
}

const char* fdev_logic_pinstr(pinw_idx_t idx, int ld1_type)
{
 	enum { NUM_BUFS = 16, BUF_SIZE = 16 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;

	last_buf = (last_buf+1)%NUM_BUFS;
	if (ld1_type == LOGIC_M)
		snprintf(buf[last_buf], sizeof(*buf), "%s%s",
			(idx & LD1) ? "M_" : "X_",
			logic_pinw_str[idx&(~LD1)]);
	else if (ld1_type == LOGIC_L)
		snprintf(buf[last_buf], sizeof(*buf), "%s%s",
			(idx & LD1) ? "L_" : "XX_",
			logic_pinw_str[idx&(~LD1)]);
	else {
		HERE();
		buf[last_buf][0] = 0;
	}
	return buf[last_buf];
}

str16_t fdev_logic_pinstr_i(struct fpga_model* model, pinw_idx_t idx, int ld1_type)
{
	int str_i;

	RC_CHECK(model);
	str_i = strarray_find(&model->str, fdev_logic_pinstr(idx, ld1_type));
	if (OUT_OF_U16(str_i))
		{ HERE(); return STRIDX_NO_ENTRY; }
	return str_i;
}

static int reset_required_pins(struct fpga_device* dev)
{
	int rc;

	if (!dev->pinw_req_for_cfg) {
		dev->pinw_req_for_cfg = malloc(dev->num_pinw_total
			* sizeof(*dev->pinw_req_for_cfg));
		if (!dev->pinw_req_for_cfg) FAIL(ENOMEM);
	}
	dev->pinw_req_total = 0;
	dev->pinw_req_in = 0;
	return 0;
fail:
	return rc;
}

void fdev_print_required_pins(struct fpga_model* model, int y, int x,
	int type, int type_idx)
{
	struct fpga_device* dev;
	int i;

	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) { HERE(); return; }

	// We don't want to reset or write the required pins in this
	// function because it is mainly used for debugging purposes
	// and the caller should not suddenly be working with old
	// required pins when the print() function is not called.

	printf("y%i x%i %s %i inpin", y, x, fdev_type2str(type), type_idx);
	if (!dev->pinw_req_in)
		printf(" -\n");
	else {
		for (i = 0; i < dev->pinw_req_in; i++)
			printf(" %s", fdev_pinw_idx2str(type, dev->pinw_req_for_cfg[i]));
		printf("\n");
	}

	printf("y%i x%i %s %i outpin", y, x, fdev_type2str(type), type_idx);
	if (dev->pinw_req_total <= dev->pinw_req_in)
		printf(" -\n");
	else {
		for (i = dev->pinw_req_in; i < dev->pinw_req_total; i++)
			printf(" %s", fdev_pinw_idx2str(type, dev->pinw_req_for_cfg[i]));
		printf("\n");
	}
}

static void add_req_inpin(struct fpga_device* dev, pinw_idx_t pinw_i)
{
	int i;

	// check for duplicate
	for (i = 0; i < dev->pinw_req_in; i++) {
		if (dev->pinw_req_for_cfg[i] == pinw_i)
			return;
	}
	if (dev->pinw_req_total > dev->pinw_req_in) {
		memmove(&dev->pinw_req_for_cfg[dev->pinw_req_in+1],
			&dev->pinw_req_for_cfg[dev->pinw_req_in],
			(dev->pinw_req_total-dev->pinw_req_in)
			*sizeof(*dev->pinw_req_for_cfg));
	}
	dev->pinw_req_for_cfg[dev->pinw_req_in] = pinw_i;
	dev->pinw_req_in++;
	dev->pinw_req_total++;
}

static void add_req_outpin(struct fpga_device* dev, pinw_idx_t pinw_i)
{
	int i;

	// check for duplicate
	for (i = dev->pinw_req_in; i < dev->pinw_req_total; i++) {
		if (dev->pinw_req_for_cfg[i] == pinw_i)
			return;
	}
	dev->pinw_req_for_cfg[dev->pinw_req_total] = pinw_i;
	dev->pinw_req_total++;
}

//
// logic device
//

int fdev_logic_setconf(struct fpga_model* model, int y, int x,
	int type_idx, const struct fpgadev_logic* logic_cfg)
{
	struct fpga_device* dev;
	int lut, rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	for (lut = LUT_A; lut <= LUT_D; lut++) {
		if (logic_cfg->a2d[lut].out_used)
			dev->u.logic.a2d[lut].out_used = 1;
		if (logic_cfg->a2d[lut].lut6) {
			rc = fdev_logic_a2d_lut(model, y, x, type_idx,
				lut, 6, logic_cfg->a2d[lut].lut6, ZTERM);
			if (rc) FAIL(rc);
		}
		if (logic_cfg->a2d[lut].lut5) {
			rc = fdev_logic_a2d_lut(model, y, x, type_idx,
				lut, 5, logic_cfg->a2d[lut].lut5, ZTERM);
			if (rc) FAIL(rc);
		}
		if (logic_cfg->a2d[lut].ff) {
			if (!logic_cfg->a2d[lut].ff_mux
			    || !logic_cfg->a2d[lut].ff_srinit)
				FAIL(EINVAL);
			dev->u.logic.a2d[lut].ff = logic_cfg->a2d[lut].ff;
			dev->u.logic.a2d[lut].ff_mux = logic_cfg->a2d[lut].ff_mux;
			dev->u.logic.a2d[lut].ff_srinit = logic_cfg->a2d[lut].ff_srinit;
		}
		if (logic_cfg->a2d[lut].out_mux)
			dev->u.logic.a2d[lut].out_mux = logic_cfg->a2d[lut].out_mux;
		if (logic_cfg->a2d[lut].ff5_srinit)
			dev->u.logic.a2d[lut].ff5_srinit = logic_cfg->a2d[lut].ff5_srinit;
		if (logic_cfg->a2d[lut].cy0)
			dev->u.logic.a2d[lut].cy0 = logic_cfg->a2d[lut].cy0;
	}
	if (logic_cfg->clk_inv)
		dev->u.logic.clk_inv = logic_cfg->clk_inv;
	if (logic_cfg->sync_attr)
		dev->u.logic.sync_attr = logic_cfg->sync_attr;
	if (logic_cfg->ce_used)
		dev->u.logic.ce_used = logic_cfg->ce_used;
	if (logic_cfg->sr_used)
		dev->u.logic.sr_used = logic_cfg->sr_used;
	if (logic_cfg->we_mux)
		dev->u.logic.we_mux = logic_cfg->we_mux;
	if (logic_cfg->cout_used)
		dev->u.logic.cout_used = logic_cfg->cout_used;
	if (logic_cfg->precyinit)
		dev->u.logic.precyinit = logic_cfg->precyinit;

	dev->instantiated = 1;
	rc = fdev_set_required_pins(model, y, x, DEV_LOGIC, type_idx);
	if (rc) FAIL(rc);
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_out_used(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int used)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].out_used = (used != 0);
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_lut(struct fpga_model* model, int y, int x, int type_idx,
	int lut_a2d, int lut_5or6, const char* lut_str, int lut_len)
{
	struct fpga_device* dev;
	char** lut_ptr;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	lut_ptr = (lut_5or6 == 5)
		? &dev->u.logic.a2d[lut_a2d].lut5
		: &dev->u.logic.a2d[lut_a2d].lut6;
	if (*lut_ptr == 0) {
		*lut_ptr = malloc(MAX_LUT_LEN);
		if (!(*lut_ptr)) FAIL(ENOMEM);
	}
	if (lut_len == ZTERM) lut_len = strlen(lut_str);
	memcpy(*lut_ptr, lut_str, lut_len);
	(*lut_ptr)[lut_len] = 0;

	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_ff(struct fpga_model* model, int y, int x, int type_idx,
	int lut_a2d, int ff_mux, int srinit)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].ff = FF_FF;
	dev->u.logic.a2d[lut_a2d].ff_mux = ff_mux;
	dev->u.logic.a2d[lut_a2d].ff_srinit = srinit;
	// A flip-flop also needs a clock (and sync attribute) to operate.
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_ff5_srinit(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int srinit)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].ff5_srinit = srinit;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_out_mux(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int out_mux)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].out_mux = out_mux;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_a2d_cy0(struct fpga_model* model, int y, int x,
	int type_idx, int lut_a2d, int cy0)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.a2d[lut_a2d].cy0 = cy0;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_clk(struct fpga_model* model, int y, int x, int type_idx,
	int clk)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.clk_inv = clk;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_sync(struct fpga_model* model, int y, int x, int type_idx,
	int sync_attr)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.sync_attr = sync_attr;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_ce_used(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.ce_used = 1;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_sr_used(struct fpga_model* model, int y, int x, int type_idx)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.sr_used = 1;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_we_mux(struct fpga_model* model, int y, int x,
	int type_idx, int we_mux)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.we_mux = we_mux;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_cout_used(struct fpga_model* model, int y, int x,
	int type_idx, int used)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.cout_used = (used != 0);
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_logic_precyinit(struct fpga_model* model, int y, int x,
	int type_idx, int precyinit)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_LOGIC, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.logic.precyinit = precyinit;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

//
// iob device
//

int fdev_iob_input(struct fpga_model* model, int y, int x, int type_idx,
	const char* io_std)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	strcpy(dev->u.iob.istandard, io_std);
	dev->u.iob.bypass_mux = BYPASS_MUX_I;
	dev->u.iob.I_mux = IMUX_I;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_iob_output(struct fpga_model* model, int y, int x, int type_idx,
	const char* io_std)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	strcpy(dev->u.iob.ostandard, io_std);
	dev->u.iob.O_used = 1;
	dev->u.iob.suspend = SUSP_3STATE;
	if (strcmp(io_std, IO_SSTL2_I)) {
		// also see ug381 page 31
		if (!strcmp(io_std, IO_LVCMOS33)
		    || !strcmp(io_std, IO_LVCMOS25))
			dev->u.iob.drive_strength = 12;
		else if (!strcmp(io_std, IO_LVCMOS12)
			 || !strcmp(io_std, IO_LVCMOS12_JEDEC))
			dev->u.iob.drive_strength = 6;
		else
			dev->u.iob.drive_strength = 8;
		dev->u.iob.slew = SLEW_SLOW;
	}
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_iob_IMUX(struct fpga_model* model, int y, int x,
	int type_idx, int mux)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.iob.I_mux = mux;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_iob_slew(struct fpga_model* model, int y, int x,
	int type_idx, int slew)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.iob.slew = slew;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_iob_drive(struct fpga_model* model, int y, int x,
	int type_idx, int drive_strength)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_IOB, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.iob.drive_strength = drive_strength;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

int fdev_bufgmux(struct fpga_model* model, int y, int x,
	int type_idx, int clk, int disable_attr, int s_inv)
{
	struct fpga_device* dev;
	int rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, DEV_BUFGMUX, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);

	dev->u.bufgmux.clk = clk;
	dev->u.bufgmux.disable_attr = disable_attr;
	dev->u.bufgmux.s_inv = s_inv;
	dev->instantiated = 1;
	return 0;
fail:
	return rc;
}

static void scan_lut_digits(const char* s, int* digits)
{
	int i;
	
	for (i = 0; i < 6; i++)
		digits[i] = 0;
	if (!s) return;
	// Note special cases "0" and "1" for a lut that
	// always results in 0 or 1.
	for (i = 0; s[i]; i++) {
		if (s[i] != 'A' && s[i] != 'a')
			continue;
		if (s[i+1] >= '1' && s[i+1] <= '6')
			digits[s[++i]-'1']++;
	}
}

int fdev_set_required_pins(struct fpga_model* model, int y, int x, int type,
	int type_idx)
{
	struct fpga_device* dev;
	int digits[6];
	int i, j, rc;

	RC_CHECK(model);
	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) FAIL(EINVAL);
	rc = reset_required_pins(dev);
	if (rc) FAIL(rc);
	if (type == DEV_LOGIC) {
		if (dev->u.logic.clk_inv)
			add_req_inpin(dev, LI_CLK);
		if (dev->u.logic.ce_used)
			add_req_inpin(dev, LI_CE);
		if (dev->u.logic.sr_used)
			add_req_inpin(dev, LI_SR);
		if (dev->u.logic.cout_used)
			add_req_outpin(dev, LO_COUT);
		if (dev->u.logic.precyinit == PRECYINIT_AX)
			add_req_inpin(dev, LI_AX);
		if (dev->u.logic.a2d[LUT_A].out_mux == MUX_F7
		    || dev->u.logic.a2d[LUT_A].ff_mux == MUX_F7)
			add_req_inpin(dev, LI_AX);
		if (dev->u.logic.a2d[LUT_C].out_mux == MUX_F7
		    || dev->u.logic.a2d[LUT_C].ff_mux == MUX_F7)
			add_req_inpin(dev, LI_CX);
		if (dev->u.logic.a2d[LUT_B].out_mux == MUX_F8
		    || dev->u.logic.a2d[LUT_B].ff_mux == MUX_F8) {
			add_req_inpin(dev, LI_AX);
			add_req_inpin(dev, LI_BX);
			add_req_inpin(dev, LI_CX);
		}
		for (i = LUT_A; i <= LUT_D; i++) {
			if (dev->u.logic.a2d[i].out_used) {
				// LO_A..LO_D are in sequence
				add_req_outpin(dev, LO_A+i);
			}
			if (dev->u.logic.a2d[i].out_mux) {
				// LO_AMUX..LO_DMUX are in sequence
				add_req_outpin(dev, LO_AMUX+i);
			}
			if (dev->u.logic.a2d[i].ff) {
				// LO_AQ..LO_DQ are in sequence
				add_req_outpin(dev, LO_AQ+i);
			}
			if (dev->u.logic.a2d[i].ff_mux == MUX_X
			    || dev->u.logic.a2d[i].cy0 == CY0_X) {
				// LI_AX..LI_DX are in sequence
				add_req_inpin(dev, LI_AX+i);
			}
			if (dev->u.logic.a2d[i].lut6) {
				scan_lut_digits(dev->u.logic.a2d[i].lut6, digits);
				for (j = 0; j < 6; j++) {
					if (!digits[j]) continue;
					add_req_inpin(dev, LI_A1+i*6+j);
				}
			}
			if (dev->u.logic.a2d[i].lut5) {
				// A6 must be high/vcc if lut5 is used
				add_req_inpin(dev, LI_A6+i*6);
				scan_lut_digits(dev->u.logic.a2d[i].lut5, digits);
				for (j = 0; j < 6; j++) {
					if (!digits[j]) continue;
					add_req_inpin(dev, LI_A1+i*6+j);
				}
			}
			if ((dev->u.logic.a2d[i].ff_mux == MUX_XOR
			     || dev->u.logic.a2d[i].out_mux == MUX_XOR)
			    && !dev->u.logic.precyinit)
				add_req_inpin(dev, LI_CIN);
		}
		return 0;
	}
	if (type == DEV_IOB) {
		if (dev->u.iob.I_mux)
			add_req_outpin(dev, IOB_OUT_I);
		if (dev->u.iob.O_used)
			add_req_inpin(dev, IOB_IN_O);
	}
	return 0;
fail:
	return rc;
}

void fdev_delete(struct fpga_model* model, int y, int x, int type, int type_idx)
{
	struct fpga_device* dev;
	int i;

	dev = fdev_p(model, y, x, type, type_idx);
	if (!dev) { HERE(); return; }
	if (!dev->instantiated) return;
	free(dev->pinw_req_for_cfg);
	dev->pinw_req_for_cfg = 0;
	dev->pinw_req_total = 0;
	dev->pinw_req_in = 0;
	if (dev->type == DEV_LOGIC) {
		for (i = LUT_A; i <= LUT_D; i++) {
			free(dev->u.logic.a2d[i].lut6);
			dev->u.logic.a2d[i].lut6 = 0;
			free(dev->u.logic.a2d[i].lut5);
			dev->u.logic.a2d[i].lut5 = 0;
		}
	}
	dev->instantiated = 0;
	memset(&dev->u, 0, sizeof(dev->u));
}

int fpga_connpt_find(struct fpga_model* model, int y, int x,
	str16_t name_i, int* connpt_dests_o, int* num_dests)
{
	struct fpga_tile* tile;
	int i;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == name_i)
			break;
	}
	if (i >= tile->num_conn_point_names)
		{ HERE(); goto fail; }
	if (num_dests) {
		*num_dests = (i < tile->num_conn_point_names-1)
			? tile->conn_point_names[(i+1)*2]
				- tile->conn_point_names[i*2]
			: tile->num_conn_point_dests
				- tile->conn_point_names[i*2];
	}
	if (connpt_dests_o)
		*connpt_dests_o = tile->conn_point_names[i*2];
	return i;
fail:
	return NO_CONN;
}

void fpga_conn_dest(struct fpga_model* model, int y, int x,
	int connpt_dest_idx, int* dest_y, int* dest_x, str16_t* str_i)
{
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	if (connpt_dest_idx < 0
	    || connpt_dest_idx >= tile->num_conn_point_dests) {
		HERE();
		return;
	}
	*dest_x = tile->conn_point_dests[connpt_dest_idx*3];
	*dest_y = tile->conn_point_dests[connpt_dest_idx*3+1];
	*str_i = tile->conn_point_dests[connpt_dest_idx*3+2];
}

int fpga_find_conn(struct fpga_model* model, int search_y, int search_x,
	str16_t* pt, int target_y, int target_x, str16_t target_pt)
{
	struct fpga_tile* tile;
	int i, j, dests_end;

	RC_CHECK(model);
	tile = YX_TILE(model, search_y, search_x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		dests_end = (i < tile->num_conn_point_names-1)
			? tile->conn_point_names[(i+1)*2]
			: tile->num_conn_point_dests;
		for (j = tile->conn_point_names[i*2]; j < dests_end; j++) {
			if (tile->conn_point_dests[j*3] == target_x
			    && tile->conn_point_dests[j*3+1] == target_y
			    && tile->conn_point_dests[j*3+2] == target_pt) {
				*pt = tile->conn_point_names[i*2+1];
				return 0;
			}
		}
	}
	*pt = STRIDX_NO_ENTRY;
	return 0;

}

swidx_t fpga_switch_first(struct fpga_model* model, int y, int x,
	str16_t name_i, int from_to)
{
	struct fpga_tile* tile;
	int i, connpt_o;

	RC_CHECK(model);
	// Finds the first switch either from or to the name given.
	if (name_i == STRIDX_NO_ENTRY) { HERE(); return NO_SWITCH; }
	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		connpt_o = SW_I(tile->switches[i], from_to);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
}

static swidx_t fpga_switch_search(struct fpga_model* model, int y, int x,
	swidx_t last, swidx_t search_beg, int from_to)
{
	struct fpga_tile* tile;
	int connpt_o, name_i, i;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	if (last & FIRST_SW) {
		connpt_o = fpga_connpt_find(model, y, x, last & ~FIRST_SW,
			/*dests_o*/ 0, /*num_dests*/ 0);
		if (connpt_o == NO_CONN) { HERE(); return NO_SWITCH; }
	} else
		connpt_o = SW_I(tile->switches[last], from_to);
	name_i = tile->conn_point_names[connpt_o*2+1];

	for (i = search_beg; i < tile->num_switches; i++) {
		connpt_o = SW_I(tile->switches[i], from_to);
		if (tile->conn_point_names[connpt_o*2+1] == name_i)
			break;
	}
	return (i >= tile->num_switches) ? NO_SWITCH : i;
}

swidx_t fpga_switch_next(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to)
{
	return fpga_switch_search(model, y, x, last,
		(last & FIRST_SW) ? 0 : last+1, from_to);
}

swidx_t fpga_switch_backtofirst(struct fpga_model* model, int y, int x,
	swidx_t last, int from_to)
{
	return fpga_switch_search(model, y, x, last, /*search_beg*/ 0, from_to);
}

int fpga_swset_fromto(struct fpga_model* model, int y, int x,
	str16_t start_switch, int from_to, struct sw_set* set)
{
	swidx_t idx;

	RC_CHECK(model);
	set->len = 0;
	idx = fpga_switch_first(model, y, x, start_switch, from_to);
	while (idx != NO_SWITCH) {
		if (set->len >= SW_SET_SIZE)
			{ HERE(); return 0; }
		set->sw[set->len++] = idx;
		idx = fpga_switch_next(model, y, x, idx, from_to);
	}
	return 0;
}

int fpga_swset_contains(struct fpga_model* model, int y, int x,
	const struct sw_set* set, int from_to, str16_t connpt)
{
	struct fpga_tile* tile;
	int i;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);
	for (i = 0; i < set->len; i++) {
		if (CONNPT_STR16(tile, SW_I(set->sw[i], from_to)) == connpt)
			return i;
	}
	return -1;
}

void fpga_swset_remove_connpt(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to, str16_t connpt)
{
	int i;
	while ((i = fpga_swset_contains(model, y, x, set,
			from_to, connpt)) != -1) {
		if (set->len > i+1)
			memmove(&set->sw[i], &set->sw[i+1],
				(set->len-i-1)*sizeof(set->sw[0]));
		set->len--;
	}
}

void fpga_swset_remove_loop(struct fpga_model* model, int y, int x,
	struct sw_set* set, const struct sw_set* parents, int from_to)
{
	int i;
	struct fpga_tile* tile;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < parents->len; i++) {
		fpga_swset_remove_connpt(model, y, x, set, !from_to,
			CONNPT_STR16(tile, SW_I(parents->sw[i], from_to)));
	}
}

void fpga_swset_remove_sw(struct fpga_model* model, int y, int x,
	struct sw_set* set, swidx_t sw)
{
	int sw_from_connpt, sw_to_connpt, cur_from_connpt, cur_to_connpt, i;

	sw_from_connpt = SW_I(sw, SW_FROM);
	sw_to_connpt = SW_I(sw, SW_TO);

	for (i = 0; i < set->len; i++) {
		cur_from_connpt = SW_I(set->sw[i], SW_FROM);
		cur_to_connpt = SW_I(set->sw[i], SW_TO);
		if (cur_from_connpt == sw_from_connpt
		    && cur_to_connpt == sw_to_connpt) {
			if ((sw & SWITCH_BIDIRECTIONAL)
				!= (set->sw[i] & SWITCH_BIDIRECTIONAL))
				HERE();
			if (set->len > i+1)
				memmove(&set->sw[i], &set->sw[i+1],
					(set->len-i-1)*sizeof(set->sw[0]));
			set->len--;
			return;
		}
		if (cur_from_connpt == sw_to_connpt
		    && cur_to_connpt == sw_from_connpt)
			HERE();
	}
}

int fpga_swset_level_down(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	int i;

	RC_CHECK(model);
	i = 0;
	while (i < set->len) {
		set->sw[i] = fpga_switch_first(model, y, x,fpga_switch_str_i(
			model, y, x, set->sw[i], !from_to), from_to);
		if (set->sw[i] == NO_SWITCH) {
			if (set->len > i+1)
				memmove(&set->sw[i], &set->sw[i+1],
					(set->len-i-1)*sizeof(set->sw[0]));
			set->len--;
		} else
			i++;
	}
	return 0;
}

void fpga_swset_print(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	int i;
	for (i = 0; i < set->len; i++) {
		printf("swset %i %s\n", i, fpga_switch_print(model,
			y, x, set->sw[i]));
	}
}

int fpga_swset_is_used(struct fpga_model* model, int y, int x,
	swidx_t* sw, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (fpga_switch_is_used(model, y, x, sw[i]))
			return 1;
	}
	return 0;
}

int fpga_switch_same_fromto(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to, swidx_t* same_sw, int *same_len)
{
	swidx_t cur_sw;
	int max_len, rc;

	RC_CHECK(model);
	max_len = *same_len;
	*same_len = 0;
	if (max_len < 1) FAIL(EINVAL);
	cur_sw = fpga_switch_search(model, y, x, sw, /*search_beg*/ 0, from_to);
	// We should at least find sw itself, if not something is wrong...
	if (cur_sw == NO_SWITCH) FAIL(EINVAL);

	same_sw[(*same_len)++] = cur_sw;
	while ((cur_sw = fpga_switch_search(model, y, x, sw, cur_sw+1, from_to)) != NO_SWITCH) {
		same_sw[(*same_len)++] = cur_sw;
		if ((*same_len) >= max_len) FAIL(EINVAL);
	}
	return 0;
fail:
	return rc;
}

swidx_t fpga_switch_lookup(struct fpga_model* model, int y, int x,
	str16_t from_str_i, str16_t to_str_i)
{
	int from_connpt_o, to_connpt_o, i;
	struct fpga_tile* tile;

	from_connpt_o = fpga_connpt_find(model, y, x, from_str_i,
		/*dests_o*/ 0, /*num_dests*/ 0);
	to_connpt_o = fpga_connpt_find(model, y, x, to_str_i,
		/*dests_o*/ 0, /*num_dests*/ 0);
	if (from_connpt_o == NO_CONN || to_connpt_o == NO_CONN)
		return NO_SWITCH;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_switches; i++) {
		if (SW_FROM_I(tile->switches[i]) == from_connpt_o
		    && SW_TO_I(tile->switches[i]) == to_connpt_o)
			return i;
	}
	return NO_SWITCH;
}

#define NUM_CONNPT_BUFS	64
#define CONNPT_BUF_SIZE	128

static const char* connpt_str(struct fpga_model* model, int y, int x, int connpt_o)
{
	// We have a little local ringbuffer to make passing
	// around pointers with unknown lifetime and possible
	// overlap with writing functions more stable.
	static char switch_get_buf[NUM_CONNPT_BUFS][CONNPT_BUF_SIZE];
	static int last_buf = 0;

	const char* hash_str;
	int str_i;

	str_i = YX_TILE(model, y, x)->conn_point_names[connpt_o*2+1];
	hash_str = strarray_lookup(&model->str, str_i);
	if (!hash_str || (strlen(hash_str) >= CONNPT_BUF_SIZE)) {
		HERE();
		return 0;
	}
	last_buf = (last_buf+1)%NUM_CONNPT_BUFS;
	return strcpy(switch_get_buf[last_buf], hash_str);
}

const char* fpga_switch_str(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to)
{
	uint32_t sw = YX_TILE(model, y, x)->switches[swidx];
	return connpt_str(model, y, x, SW_I(sw, from_to));
}

str16_t fpga_switch_str_i(struct fpga_model* model, int y, int x,
	swidx_t swidx, int from_to)
{
	struct fpga_tile* tile;
	uint32_t sw;
	int connpt_o;

	tile = YX_TILE(model, y, x);
	sw = tile->switches[swidx];
	connpt_o = SW_I(sw, from_to);
	return tile->conn_point_names[connpt_o*2+1];
}

const char* fpga_switch_print(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
 	enum { NUM_BUFS = 16, BUF_SIZE = 128 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;
	uint32_t sw;

	sw = YX_TILE(model, y, x)->switches[swidx];
	last_buf = (last_buf+1)%NUM_BUFS;

	snprintf(buf[last_buf], sizeof(*buf), "%s %s %s",
		connpt_str(model, y, x, SW_FROM_I(sw)),
		sw & SWITCH_BIDIRECTIONAL ? "<->" : "->",
		connpt_str(model, y, x, SW_TO_I(sw)));
	return buf[last_buf];
}

int fpga_switch_is_bidir(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_BIDIRECTIONAL) != 0;
}

int fpga_switch_is_used(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	return (YX_TILE(model, y, x)->switches[swidx] & SWITCH_USED) != 0;
}

void fpga_switch_enable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] |= SWITCH_USED;
}

int fpga_switch_set_enable(struct fpga_model* model, int y, int x,
	struct sw_set* set)
{
	int i;
	for (i = 0; i < set->len; i++)
		fpga_switch_enable(model, y, x, set->sw[i]);
	return 0;
}

void fpga_switch_disable(struct fpga_model* model, int y, int x,
	swidx_t swidx)
{
	YX_TILE(model, y, x)->switches[swidx] &= ~SWITCH_USED;
}

#define SW_BUF_SIZE	256
#define NUM_SW_BUFS	64

static const char* fmt_swset_el(struct fpga_model* model, int y, int x,
	swidx_t sw, int from_to)
{
	static char sw_buf[NUM_SW_BUFS][SW_BUF_SIZE];
	static int last_buf = 0;
	char midstr[64];

	last_buf = (last_buf+1)%NUM_SW_BUFS;
	if (fpga_switch_is_bidir(model, y, x, sw))
		strcpy(midstr, "<->");
	else
		strcpy(midstr, "->");
	// fmt_swset_el() prints only the destination side of the
	// switch (!from_to), because it is the significant one in
	// a chain of switches, and if the caller wants the source
	// side they can add it outside.
	snprintf(sw_buf[last_buf], sizeof(sw_buf[0]), "%s%s%s",
		(from_to == SW_FROM) ? ""
			: fpga_switch_str(model, y, x, sw, SW_FROM),
		midstr,
		(from_to == SW_TO) ? ""
			: fpga_switch_str(model, y, x, sw, SW_TO));

	return sw_buf[last_buf];
}

#define FMT_SWSET_BUF_SIZE	2048
#define FMT_SWSET_NUM_BUFS	8

const char* fmt_swset(struct fpga_model* model, int y, int x,
	struct sw_set* set, int from_to)
{
	static char buf[FMT_SWSET_NUM_BUFS][FMT_SWSET_BUF_SIZE];
	static int last_buf = 0;
	int i, o;

	last_buf = (last_buf+1)%FMT_SWSET_NUM_BUFS;
	o = 0;
	if (set->len) {
		if (from_to == SW_FROM) {
			strcpy(&buf[last_buf][o], fpga_switch_str(model, y, x, set->sw[0], SW_FROM));
			o += strlen(&buf[last_buf][o]);
			for (i = 0; i < set->len; i++) {
				buf[last_buf][o++] = ' ';
				strcpy(&buf[last_buf][o],
					fmt_swset_el(model, y, x, set->sw[i], SW_FROM));
				o += strlen(&buf[last_buf][o]);
			}
		} else { // SW_TO
			for (i = set->len-1; i >= 0; i--) {
				if (i < set->len-1) buf[last_buf][o++] = ' ';
				strcpy(&buf[last_buf][o],
					fmt_swset_el(model, y, x, set->sw[i], SW_TO));
				o += strlen(&buf[last_buf][o]);
			}
			buf[last_buf][o++] = ' ';
			strcpy(&buf[last_buf][o], fpga_switch_str(model, y, x, set->sw[0], SW_TO));
			o += strlen(&buf[last_buf][o]);
		}
	}
	buf[last_buf][o] = 0;
	return buf[last_buf];
}

int construct_sw_chain(struct sw_chain* chain, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth,
	net_idx_t exclusive_net, swidx_t* block_list, int block_list_len)
{
	RC_CHECK(model);
#ifdef DBG_ENUM_SWITCH
	printf("construct_sw_chain() %s (%s)\n",
		strarray_lookup(&model->str, start_switch),
		(from_to == SW_FROM) ? "SW_FROM" : "SW_TO");
#endif
	memset(chain, 0, sizeof(*chain));
	chain->model = model;
	chain->y = y;
	chain->x = x;
	chain->from_to = from_to;
	chain->max_depth = (max_depth < 0) ? SW_SET_SIZE : max_depth;
	chain->exclusive_net = exclusive_net;
	if (block_list) {
		chain->block_list = block_list;
		chain->block_list_len = block_list_len;
		// internal_block_list is 0 from memset()
	} else {
		chain->internal_block_list = malloc(
			MAX_SWITCHBOX_SIZE * sizeof(*chain->block_list));
		if (!chain->internal_block_list)
			RC_FAIL(model, ENOMEM);
		chain->block_list = chain->internal_block_list;
		chain->block_list_len = 0;
	}
	// at every level, the first round returns all members
	// at that level, then the second round tries to go
	// one level down for each member. This sorts the
	// returned switches in a nice way.
	chain->first_round = 1;
	chain->set.len = 1;
	chain->set.sw[0] = FIRST_SW | start_switch;
	RC_RETURN(model);
}

void destruct_sw_chain(struct sw_chain* chain)
{
	free(chain->internal_block_list);
	memset(chain, 0, sizeof(*chain));
}

// returns NO_SWITCH if sw is not found in list, otherwise
// the offset in list where it is found.
static int switch_list_contains(struct fpga_model* model, int y, int x,
	swidx_t* list, int list_len, swidx_t sw)
{
	int sw_from_connpt, sw_to_connpt, cur_from_connpt, cur_to_connpt, i;

	sw_from_connpt = SW_I(sw, SW_FROM);
	sw_to_connpt = SW_I(sw, SW_TO);

	for (i = 0; i < list_len; i++) {
		cur_from_connpt = SW_I(list[i], SW_FROM);
		cur_to_connpt = SW_I(list[i], SW_TO);
		if (cur_from_connpt == sw_from_connpt
		    && cur_to_connpt == sw_to_connpt) {
			if ((sw & SWITCH_BIDIRECTIONAL)
				!= (list[i] & SWITCH_BIDIRECTIONAL))
				HERE();
			return i;
		}
		if (cur_from_connpt == sw_to_connpt
		    && cur_to_connpt == sw_from_connpt)
			HERE();
	}
	return NO_SWITCH;
}

int fpga_switch_chain(struct sw_chain* ch)
{
	swidx_t idx;
	struct fpga_tile* tile;
	int child_from_to, level_down, i;

	if (!ch->set.len)
		{ HERE(); goto internal_error; }
	if (ch->first_round) {
		// first go through all members at present level
		while (1) {
			idx = fpga_switch_next(ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1], ch->from_to);
			if (idx == NO_SWITCH)
				break;
			ch->set.sw[ch->set.len-1] = idx;
			if ((ch->exclusive_net == NO_NET
			     || !fpga_swset_in_other_net(ch->model, ch->y,
					ch->x, &idx, /*len*/ 1,
					ch->exclusive_net))
			    && switch_list_contains(ch->model, ch->y, ch->x,
				ch->block_list, ch->block_list_len, idx)
					== NO_SWITCH)
				return 0;
		}
		// if there are no more, start child round
		ch->first_round = 0;
		idx = fpga_switch_backtofirst(ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], ch->from_to);
		if (idx == NO_SWITCH) {
			ch->set.len = 0;
			return NO_SWITCH;
		}
#ifdef DBG_ENUM_SWITCH
		printf("back_to_first from %s to %s\n",
			fpga_switch_print(ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1]),
			fpga_switch_print(ch->model, ch->y, ch->x,
				idx));
#endif
		ch->set.sw[ch->set.len-1] = idx;
	}
	// look for children
	tile = YX_TILE(ch->model, ch->y, ch->x);
	while (1) {
		idx = fpga_switch_first(ch->model, ch->y, ch->x,
			fpga_switch_str_i(ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], !ch->from_to),
			ch->from_to);
#ifdef DBG_ENUM_SWITCH
		printf("child_check %s result %s\n", fpga_switch_str(
			ch->model, ch->y, ch->x,
			ch->set.sw[ch->set.len-1], !ch->from_to),
			idx == NO_SWITCH ? "NO_SWITCH" : fpga_switch_print(
			ch->model, ch->y, ch->x, idx));
#endif
		if (idx != NO_SWITCH) {
			// There are four reasons why we don't want to
			// go down one level:
			// 1) switch is used
			// 2) switch loop back to parent of chain
			// 3) switch is blocked
			// 4) max depth reached
			level_down = 1;

			child_from_to = SW_I(tile->switches[ch->set.sw[
				ch->set.len-1]], !ch->from_to);
#ifdef DBG_ENUM_SWITCH
			printf(" child_from_to %s\n", connpt_str(
				ch->model, ch->y, ch->x,
				child_from_to));
#endif
			if (ch->exclusive_net != NO_NET
			    && fpga_swset_in_other_net(ch->model, ch->y, ch->x,
				&idx, /*len*/ 1, ch->exclusive_net))
				level_down = 0;

			if (level_down) {
				// If we have the same from-switch already
				// among the parents, don't enter into a loop.
				for (i = 0; i < ch->set.len; i++) {
					int parent_connpt = SW_I(tile->switches[
						ch->set.sw[i]], ch->from_to);
#ifdef DBG_ENUM_SWITCH
					printf("  parent connpt %s%s\n", connpt_str(
						ch->model, ch->y, ch->x,
						parent_connpt), parent_connpt
						== child_from_to ? " (match)" : "");
#endif
					if (parent_connpt == child_from_to) {
						level_down = 0;
						break;
					}
				}
			}
			if (level_down) {
				// only go down if not blocked
				level_down = switch_list_contains(ch->model,
					ch->y, ch->x, ch->block_list,
					ch->block_list_len, idx) == NO_SWITCH;
			}
			if (level_down) {
				if (ch->set.len >= SW_SET_SIZE)
					{ HERE(); goto internal_error; }
				if (ch->max_depth < 1
				    || ch->set.len < ch->max_depth) {
					// back to first round at next level
#ifdef DBG_ENUM_SWITCH
					printf(" level_down %s\n",
					  fpga_switch_print(ch->model,
					  ch->y, ch->x, idx));
#endif
					ch->first_round = 1;
					ch->set.sw[ch->set.len] = idx;
					ch->set.len++;
					return 0;
				}
			}
		}
		while (1) {
			// todo: don't know why we catch blocklist duplicates
			//       if ch->set.len > 1 - needs more debugging
			if (switch_list_contains(ch->model, ch->y, ch->x,
				ch->block_list, ch->block_list_len,
				ch->set.sw[ch->set.len-1]) == NO_SWITCH) {
#ifdef DBG_ENUM_SWITCH
				printf(" block_list_add %s\n", fpga_switch_print(
					ch->model, ch->y, ch->x,
					ch->set.sw[ch->set.len-1]));
#endif
				if (ch->block_list_len >= MAX_SWITCHBOX_SIZE)
					{ HERE(); goto internal_error; }
				ch->block_list[ch->block_list_len++]
					= ch->set.sw[ch->set.len-1];
			}

			ch->set.sw[ch->set.len-1] = fpga_switch_next(
				ch->model, ch->y, ch->x,
				ch->set.sw[ch->set.len-1], ch->from_to);
			if (ch->set.sw[ch->set.len-1] != NO_SWITCH) {
				if (ch->exclusive_net != NO_NET
				    && fpga_swset_in_other_net(ch->model, ch->y, ch->x,
					&ch->set.sw[ch->set.len-1], /*len*/ 1,
					ch->exclusive_net)) {
#ifdef DBG_ENUM_SWITCH
					printf(" skipping used %s\n",
					  fpga_switch_print(ch->model, ch->y,
					  ch->x, ch->set.sw[ch->set.len-1]));
#endif
					continue;
				}
#ifdef DBG_ENUM_SWITCH
				printf(" found %s\n", fpga_switch_print(
				  ch->model, ch->y, ch->x,
				  ch->set.sw[ch->set.len-1]));
#endif
				break;
			}

			if (ch->set.len <= 1) {
				ch->set.len = 0;
				return NO_SWITCH;
			}
#ifdef DBG_ENUM_SWITCH
			printf(" level_up\n");
#endif
			ch->set.len--;
		}
	}
internal_error:
	ch->set.len = 0;
	return NO_SWITCH;
}

int construct_sw_conns(struct sw_conns* conns, struct fpga_model* model,
	int y, int x, str16_t start_switch, int from_to, int max_depth,
	net_idx_t exclusive_net)
{
	RC_CHECK(model);
	memset(conns, 0, sizeof(*conns));
	construct_sw_chain(&conns->chain, model, y, x, start_switch,
		from_to, max_depth, exclusive_net,
		/*block_list*/ 0, /*block_list_len*/ 0);
	RC_RETURN(model);
}

void destruct_sw_conns(struct sw_conns* conns)
{
	destruct_sw_chain(&conns->chain);
	memset(conns, 0, sizeof(*conns));
}

int fpga_multi_switch_lookup(struct fpga_model *model, int y, int x,
	str16_t from_sw, str16_t to_sw, int max_depth, net_idx_t exclusive_net,
	struct sw_set *sw_set)
{
	struct sw_chain sw_chain;

	sw_set->len = 0;
	construct_sw_chain(&sw_chain, model, y, x, from_sw, SW_FROM, max_depth,
		exclusive_net, /*block_list*/ 0, /*block_list_len*/ 0);
	RC_CHECK(model);

	while (fpga_switch_chain(&sw_chain) != NO_CONN) {
		RC_ASSERT(model, sw_chain.set.len);
		if (fpga_switch_str_i(model, y, x, sw_chain.set.sw[sw_chain.set.len-1],
			SW_TO) == to_sw) {
			*sw_set = sw_chain.set;
			break;
		}
	}
	destruct_sw_chain(&sw_chain);
	RC_RETURN(model);
}

int fpga_switch_conns(struct sw_conns* conns)
{
	str16_t end_of_chain_str;

	if (conns->chain.model->rc
	    || !conns->chain.set.len)
		{ HERE(); goto internal_error; }

#ifdef DBG_SWITCH_TO_YX
	printf("fpga_switch_conns() dest_i %i num_dests %i\n", conns->dest_i, conns->num_dests);
#endif
	// on the first call, both dest_i and num_dests are 0
	while (conns->dest_i >= conns->num_dests) {
		fpga_switch_chain(&conns->chain);
		if (conns->chain.set.len == 0) {
#ifdef DBG_SWITCH_TO_YX
			printf(" no more switches\n");
#endif
			return NO_CONN;
		}
		end_of_chain_str = fpga_switch_str_i(conns->chain.model,
			conns->chain.y, conns->chain.x,
			conns->chain.set.sw[conns->chain.set.len-1],
			!conns->chain.from_to);
		if (end_of_chain_str == STRIDX_NO_ENTRY)
			{ HERE(); goto internal_error; }
		conns->dest_i = 0;
		fpga_connpt_find(conns->chain.model, conns->chain.y,
			conns->chain.x, end_of_chain_str,
			&conns->connpt_dest_start, &conns->num_dests);
		if (conns->num_dests) {
#ifdef DBG_SWITCH_TO_YX
			printf(" %s: %i conns\n", strarray_lookup(&conns->chain.model->str,
				end_of_chain_str), conns->num_dests);
#endif
			break;
		}
#ifdef DBG_SWITCH_TO_YX
		printf(" %s: no conns\n", strarray_lookup(
			&conns->chain.model->str, end_of_chain_str));
#endif
	}
	fpga_conn_dest(conns->chain.model, conns->chain.y, conns->chain.x,
		conns->connpt_dest_start + conns->dest_i,
		&conns->dest_y, &conns->dest_x, &conns->dest_str_i);
	if (conns->dest_str_i == STRIDX_NO_ENTRY)
		{ HERE(); goto internal_error; }
	conns->dest_i++;
	return 0;
		
internal_error:
	conns->chain.set.len = 0;
	return NO_CONN;
}

int fpga_first_conn(struct fpga_model *model, int sw_y, int sw_x, str16_t sw_str,
	int from_to, int max_depth, net_idx_t exclusive_net,
	struct sw_set *sw_set, int *dest_y, int *dest_x, str16_t *dest_connpt)
{
	struct sw_conns sw_conns;

	construct_sw_conns(&sw_conns, model, sw_y, sw_x, sw_str, from_to,
		max_depth, exclusive_net);
	RC_CHECK(model);
	if (fpga_switch_conns(&sw_conns) == NO_CONN)
		RC_FAIL(model, EINVAL);

	*sw_set = sw_conns.chain.set;
	*dest_y = sw_conns.dest_y;
	*dest_x = sw_conns.dest_x;
	*dest_connpt = sw_conns.dest_str_i;
	destruct_sw_conns(&sw_conns);
	RC_RETURN(model);
}

void printf_swchain(struct fpga_model* model, int y, int x,
	str16_t sw, int from_to, int max_depth, swidx_t* block_list,
	int* block_list_len)
{
	struct sw_chain chain;
	int count = 0;

	printf("printf_swchain() y%i x%i %s blist_len %i\n",
		y, x, strarray_lookup(&model->str, sw),
		block_list_len ? *block_list_len : 0);
	if (construct_sw_chain(&chain, model, y, x, sw, from_to, max_depth,
		NO_NET, block_list, block_list_len ? *block_list_len : 0))
		{ HERE(); return; }
	while (fpga_switch_chain(&chain) != NO_CONN) {
		printf("sw %i %s\n", count++, fmt_swset(model, y, x,
			&chain.set, from_to));
	}
	printf("sw - block_list_len %i\n", chain.block_list_len);
	if (block_list_len)
		*block_list_len = chain.block_list_len;
	destruct_sw_chain(&chain);
}

void printf_swconns(struct fpga_model* model, int y, int x,
	str16_t sw, int from_to, int max_depth)
{
	struct sw_conns conns;

	if (construct_sw_conns(&conns, model, y, x, sw, from_to,
			max_depth, NO_NET))
		{ HERE(); return; }
	while (fpga_switch_conns(&conns) != NO_CONN) {
		printf("sw %s conn y%i x%i %s\n", fmt_swset(model, y, x,
			&conns.chain.set, from_to),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&model->str, conns.dest_str_i));
	}
	printf("sw -\n");
	destruct_sw_conns(&conns);
}

int fpga_switch_to_yx(struct switch_to_yx* p)
{
	struct sw_conns conns;
	struct sw_set best_set;
	int best_y, best_x, best_distance, distance;
	int best_num_dests;
	str16_t best_connpt;

	RC_CHECK(p->model);
#ifdef DBG_SWITCH_TO_YX
	printf("fpga_switch_to_yx() %s y%i-x%i-%s yx_req %Xh flags %Xh excl_net %i\n",
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO",
		p->y, p->x, strarray_lookup(&p->model->str, p->start_switch),
		p->yx_req, p->flags, p->exclusive_net);
#endif
	construct_sw_conns(&conns, p->model, p->y, p->x, p->start_switch,
		p->from_to, SW_SET_SIZE, p->exclusive_net);
	RC_CHECK(p->model);

	best_y = -1;
	while (fpga_switch_conns(&conns) != NO_CONN) {
		if (!is_atyx(p->yx_req, p->model, conns.dest_y, conns.dest_x))
			continue;
#ifdef DBG_SWITCH_TO_YX
		printf(" sw %s conn y%i x%i %s\n",
			fmt_swset(p->model, p->y, p->x,
				&conns.chain.set, p->from_to),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&p->model->str, conns.dest_str_i));
#endif
		if (best_y != -1) {
			distance = abs(conns.dest_y-p->y)
				+abs(conns.dest_x-p->x);
			if (distance > best_distance)
				continue;
			else if (conns.num_dests > best_num_dests)
				continue;
			else if (conns.chain.set.len > best_set.len)
				continue;
		}
		best_set = conns.chain.set;
		best_y = conns.dest_y;
		best_x = conns.dest_x;
		best_num_dests = conns.num_dests;
		best_connpt = conns.dest_str_i;
		best_distance = abs(conns.dest_y-p->y)
			+abs(conns.dest_x-p->x);
		if (!p->flags & SWTO_YX_CLOSEST)
			break;
	}
	if (best_y == -1)
		p->set.len = 0;
	else {
		p->set = best_set;
		p->dest_y = best_y;
		p->dest_x = best_x;
		p->dest_connpt = best_connpt;
	}
	destruct_sw_conns(&conns);
	RC_RETURN(p->model);
}

void printf_switch_to_yx_result(struct switch_to_yx* p)
{
	printf("switch_to_yx() from y%i x%i connpt %s (%s)\n",
		p->y, p->x, strarray_lookup(&p->model->str, p->start_switch),
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO");
	printf(" %s y%i x%i %s via %s\n",
		p->from_to == SW_FROM ? "to" : "from",
		p->dest_y, p->dest_x,
		strarray_lookup(&p->model->str, p->dest_connpt),
		fmt_swset(p->model, p->y, p->x, &p->set, p->from_to));
}

int fpga_switch_to_yx_l2(struct switch_to_yx_l2 *p)
{
	RC_CHECK(p->l1.model);
	fpga_switch_to_yx(&p->l1);
	RC_CHECK(p->l1.model);
	p->l2_set.len = 0;
	if (!p->l1.set.len) {
		struct sw_conns conns;
		struct switch_to_yx l2 = p->l1;

		construct_sw_conns(&conns, p->l1.model, p->l1.y, p->l1.x, p->l1.start_switch,
			p->l1.from_to, SW_SET_SIZE, p->l1.exclusive_net);
		RC_CHECK(p->l1.model);
		while (fpga_switch_conns(&conns) != NO_CONN) {
			l2.y = conns.dest_y;
			l2.x = conns.dest_x;
			l2.start_switch = conns.dest_str_i;
			fpga_switch_to_yx(&l2);
			RC_CHECK(l2.model);
			if (l2.set.len) {
				p->l1.set = conns.chain.set;
				p->l1.dest_y = l2.dest_y;
				p->l1.dest_x = l2.dest_x;
				p->l1.dest_connpt = l2.dest_connpt;
				p->l2_set = l2.set;
				p->l2_y = l2.y;
				p->l2_x = l2.x;
				break;
			}
		}
		destruct_sw_conns(&conns);
	}
	RC_RETURN(p->l1.model);
}

int fpga_switch_to_rel(struct switch_to_rel *p)
{
	struct sw_conns conns;
	struct sw_set best_set;
	int best_y, best_x, best_distance;
	str16_t best_connpt;

	RC_CHECK(p->model);
#ifdef DBG_SWITCH_TO_REL
	printf("fpga_switch_to_rel() %s y%i-x%i-%s flags %Xh rel_y %i rel_x %i target_connpt %s\n",
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO",
		p->start_y, p->start_x, strarray_lookup(&p->model->str, p->start_switch),
		p->flags, p->rel_y, p->rel_x,
		p->target_connpt == STRIDX_NO_ENTRY ? "-" :
			strarray_lookup(&p->model->str, p->target_connpt));
#endif
	construct_sw_conns(&conns, p->model, p->start_y, p->start_x,
		p->start_switch, p->from_to, SW_SET_SIZE, NO_NET);
	RC_CHECK(p->model);

	best_y = -1;
	while (fpga_switch_conns(&conns) != NO_CONN) {
#ifdef DBG_SWITCH_TO_REL
		printf(" sw %s conn y%i x%i %s\n",
			fmt_swset(p->model, p->start_y, p->start_x,
				&conns.chain.set, p->from_to),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&p->model->str, conns.dest_str_i));
#endif
		// Do not continue with connections that do not lead to
		// a switch.
		if (fpga_switch_first(p->model, conns.dest_y, conns.dest_x,
			conns.dest_str_i, p->from_to) == NO_SWITCH) {
#ifdef DBG_SWITCH_TO_REL
			printf(" no_switch\n");
#endif
			continue;
		}
		if (conns.dest_y != p->start_y + p->rel_y
		    || conns.dest_x != p->start_x + p->rel_x) {
			if (!(p->flags & SWTO_REL_WEAK_TARGET))
				continue;
			if (best_y != -1) {
				int distance = abs(conns.dest_y - (p->start_y+p->rel_y))
					+ abs(conns.dest_x - (p->start_x+p->rel_x));
				if (distance > best_distance)
					continue;
			}
			best_set = conns.chain.set;
			best_y = conns.dest_y;
			best_x = conns.dest_x;
			best_connpt = conns.dest_str_i;
			best_distance = abs(conns.dest_y - (p->start_y+p->rel_y))
				+ abs(conns.dest_x - (p->start_x+p->rel_x));
			continue;
		}
		if (p->target_connpt != STRIDX_NO_ENTRY
		    && conns.dest_str_i != p->target_connpt)
			continue;
		best_set = conns.chain.set;
		best_y = conns.dest_y;
		best_x = conns.dest_x;
		best_connpt = conns.dest_str_i;
		break;
	}
	if (best_y == -1)
		p->set.len = 0;
	else {
#ifdef DBG_SWITCH_TO_REL
		printf(" sw %s\n", fmt_swset(p->model, p->start_y, p->start_x,
			&best_set, p->from_to));
		printf(" dest y%i-x%i-%s\n", best_y, best_x,
			strarray_lookup(&p->model->str, best_connpt));
#endif
		p->set = best_set;
		p->dest_y = best_y;
		p->dest_x = best_x;
		p->dest_connpt = best_connpt;
	}
	destruct_sw_conns(&conns);
	RC_RETURN(p->model);
}

void printf_switch_to_rel_result(struct switch_to_rel* p)
{
	printf("switch_to_rel() from y%i x%i connpt %s (%s)\n",
		p->start_y, p->start_x, strarray_lookup(&p->model->str, p->start_switch),
		p->from_to == SW_FROM ? "SW_FROM" : "SW_TO");
	printf(" %s y%i x%i %s via %s\n",
		p->from_to == SW_FROM ? "to" : "from",
		p->dest_y, p->dest_x,
		strarray_lookup(&p->model->str, p->dest_connpt),
		fmt_swset(p->model, p->start_y, p->start_x, &p->set, p->from_to));
}

#define NET_ALLOC_INCREMENT 64

static int fnet_useidx(struct fpga_model* model, net_idx_t new_idx)
{
	void* new_ptr;
	int new_array_size;

	RC_CHECK(model);
	RC_ASSERT(model, new_idx > NO_NET);
	if (new_idx > model->highest_used_net)
		model->highest_used_net = new_idx;

	if ((new_idx-1) < model->nets_array_size)
		return 0;

	new_array_size = ((new_idx-1)/NET_ALLOC_INCREMENT+1)*NET_ALLOC_INCREMENT;
	new_ptr = realloc(model->nets, new_array_size*sizeof(*model->nets));
	if (!new_ptr) RC_FAIL(model, ENOMEM);
	// the memset will set the 'len' of each new net to 0
	memset(new_ptr + model->nets_array_size*sizeof(*model->nets), 0,
		(new_array_size - model->nets_array_size)*sizeof(*model->nets));

	model->nets = new_ptr;
	model->nets_array_size = new_array_size;
	RC_RETURN(model);
}

int fnet_new(struct fpga_model* model, net_idx_t* new_idx)
{
	int rc;

	RC_CHECK(model);
	// highest_used_net is initialized to NO_NET which becomes 1
	rc = fnet_useidx(model, model->highest_used_net+1);
	if (rc) return rc;
	*new_idx = model->highest_used_net;
	return 0;
}

void fnet_delete(struct fpga_model* model, net_idx_t net_idx)
{
	struct fpga_net* net;
	int i;

	net = &model->nets[net_idx-1];
	for (i = 0; i < net->len; i++) {
		if (net->el[i].idx & NET_IDX_IS_PINW)
			continue;
		if (!fpga_switch_is_used(model, net->el[i].y, net->el[i].x,
			net->el[i].idx))
			HERE();
		fpga_switch_disable(model, net->el[i].y, net->el[i].x,
			net->el[i].idx);
	}
	model->nets[net_idx-1].len = 0;
	if (model->highest_used_net == net_idx)
		model->highest_used_net--;
}

int fnet_enum(struct fpga_model* model, net_idx_t last, net_idx_t* next)
{
	int i;

	RC_CHECK(model);
	// last can be NO_NET which becomes 1 = the first net index
	for (i = last+1; i <= model->highest_used_net; i++) {
		if (model->nets[i-1].len) {
			*next = i;
			return 0;
		}
	}
	*next = NO_NET;
	return 0;
}

struct fpga_net* fnet_get(struct fpga_model* model, net_idx_t net_i)
{
	// function must work even if model->rc is set
	if (net_i <= NO_NET
	    || net_i > model->highest_used_net) {
		fprintf(stderr, "%s:%i net_i %i highest_used %i\n", __FILE__,
			__LINE__, net_i, model->highest_used_net);
		return 0;
	}
	return &model->nets[net_i-1];
}

void fnet_free_all(struct fpga_model* model)
{
	free(model->nets);
	model->nets = 0;
	model->nets_array_size = 0;
	model->highest_used_net = 0;
}

int fpga_swset_in_other_net(struct fpga_model *model, int y, int x,
	const swidx_t* sw, int len, net_idx_t our_net)
{
	struct fpga_net* net_p;
	int i, j;

	net_p = fnet_get(model, our_net);
	if (!net_p) { HERE(); return 0; }
	for (i = 0; i < len; i++) {
		if (!fpga_switch_is_used(model, y, x, sw[i]))
			continue;
		for (j = 0; j < net_p->len; j++) {
			if (net_p->el[j].idx & NET_IDX_IS_PINW)
				continue;
			if (net_p->el[j].y == y
			    && net_p->el[j].x == x
			    && net_p->el[j].idx == sw[i])
				break;
		}
		if (j >= net_p->len) {
			// switch must be in other net
			return 1;
		}
	}
	return 0;
}

int fnet_add_port(struct fpga_model* model, net_idx_t net_i,
	int y, int x, enum fpgadev_type type, dev_type_idx_t type_idx,
	pinw_idx_t pinw_idx)
{
	struct fpga_net* net;

	fnet_useidx(model, net_i);
	RC_CHECK(model);
	
	net = &model->nets[net_i-1];
	RC_ASSERT(model, net->len < MAX_NET_LEN);

	net->el[net->len].y = y;
	net->el[net->len].x = x;
	net->el[net->len].idx = pinw_idx | NET_IDX_IS_PINW;
	net->el[net->len].dev_idx = fpga_dev_idx(model, y, x, type, type_idx);
	RC_ASSERT(model, net->el[net->len].dev_idx != NO_DEV);

	net->len++;
	RC_RETURN(model);
}

int fnet_add_sw(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const swidx_t* switches, int num_sw)
{
	struct fpga_net* net;
	int i, j;

	fnet_useidx(model, net_i);
	RC_CHECK(model);

	// Check that none of the switches isn't already in use
	// in another net.
	if (fpga_swset_in_other_net(model, y, x, switches, num_sw, net_i))
		RC_FAIL(model, EINVAL);

	net = &model->nets[net_i-1];
	for (i = 0; i < num_sw; i++) {
		if (switches[i] == NO_SWITCH)
			{ HERE(); continue; }

		// check whether the switch is already in the net
		for (j = 0; j < net->len; j++) {
			if (net->el[j].y == y
			    && net->el[j].x == x
			    && net->el[j].idx == switches[i])
				break;
		}
		if (j < net->len) continue;

		// add the switch
		RC_ASSERT(model, net->len < MAX_NET_LEN);
		net->el[net->len].y = y;
		net->el[net->len].x = x;
		RC_ASSERT(model, !OUT_OF_U16(switches[i]));
		if (fpga_switch_is_used(model, y, x, switches[i]))
			HERE();
		fpga_switch_enable(model, y, x, switches[i]);
		net->el[net->len].idx = switches[i];
		net->len++;
	}
	RC_RETURN(model);
}

static void _fnet_remove_sw(struct fpga_model *model, struct fpga_net *net_p, int i)
{
	if (!fpga_switch_is_used(model, net_p->el[i].y, net_p->el[i].x, net_p->el[i].idx))
		HERE();
	fpga_switch_disable(model, net_p->el[i].y, net_p->el[i].x, net_p->el[i].idx);
	if (net_p->len > i+1)
		memmove(&net_p->el[i], &net_p->el[i+1],
			(net_p->len-i-1)*sizeof(net_p->el[0]));
	net_p->len--;
}

int fnet_remove_sw(struct fpga_model* model, net_idx_t net_i,
	int y, int x, const swidx_t* switches, int num_sw)
{
	struct fpga_net* net;
	int i, j;

	RC_CHECK(model);
	RC_ASSERT(model, net_i <= model->highest_used_net);
	net = &model->nets[net_i-1];
	if (!net->len) {
		HERE();
		return 0;
	}
	for (i = 0; i < num_sw; i++) {
		for (j = 0; j < net->len; j++) {
			if (net->el[j].y == y
			    && net->el[j].x == x
			    && net->el[j].idx == switches[i])
				break;
		}
		if (j >= net->len) {
			// for now we expect the 'to-be-removed'
			// switches to be in the net
			HERE();
			continue;
		}
		_fnet_remove_sw(model, net, j);
	}
	RC_RETURN(model);
}

int fnet_remove_all_sw(struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net_p;
	int i;

	RC_CHECK(model);
	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);
	for (i = 0; i < net_p->len; i++) {
		if (net_p->el[i].idx & NET_IDX_IS_PINW)
			continue;
		_fnet_remove_sw(model, net_p, i);
	}
	RC_RETURN(model);
}

static void fprintf_inout_pin(FILE* f, struct fpga_model* model,
	net_idx_t net_i, struct net_el* el)
{
	struct fpga_tile* tile;
	pinw_idx_t pinw_i;
	dev_idx_t dev_idx;
	int in_pin;
	const char* pin_str;
	char buf[1024];

	if (!(el->idx & NET_IDX_IS_PINW))
		{ HERE(); return; }

	tile = YX_TILE(model, el->y, el->x);
	dev_idx = el->dev_idx;
	if (dev_idx < 0 || dev_idx >= tile->num_devs)
		{ HERE(); return; }
	pinw_i = el->idx & NET_IDX_MASK;
	if (pinw_i < 0 || pinw_i >= tile->devs[dev_idx].num_pinw_total)
		{ HERE(); return; }
	in_pin = pinw_i < tile->devs[dev_idx].num_pinw_in;

   	pin_str = fdev_pinw_idx2str(tile->devs[dev_idx].type, pinw_i);
	if (!pin_str) { HERE(); return; }

	snprintf(buf, sizeof(buf), "net %i %s y%i x%i %s %i pin %s\n",
		net_i, in_pin ? "in" : "out", el->y, el->x,
		fdev_type2str(tile->devs[dev_idx].type),
		fdev_typeidx(model, el->y, el->x, dev_idx),
		pin_str);
	fprintf(f, "%s", buf);
}

void fnet_printf(FILE* f, struct fpga_model* model, net_idx_t net_i)
{
	struct fpga_net* net;
	int i;

	net = fnet_get(model, net_i);
	if (!net) { HERE(); return; }
	for (i = 0; i < net->len; i++) {
		if (net->el[i].idx & NET_IDX_IS_PINW) {
			fprintf_inout_pin(f, model, net_i, &net->el[i]);
			continue;
		}
		// switch
		fprintf(f, "net %i sw y%i x%i %s\n",
			net_i, net->el[i].y, net->el[i].x,
			fpga_switch_print(model, net->el[i].y,
				net->el[i].x, net->el[i].idx));
	}
}

//
// routing
//

// locates the n'th (idx) inpin or outpin (is_out) in the net
static int fnet_pinw(struct fpga_model *model, net_idx_t net_i,
	int is_out, int idx)
{
	struct fpga_net *net_p;
	struct fpga_device *dev_p;
	int i, next_idx;

	if (model->rc) { HERE(); return -1; };
	net_p = fnet_get(model, net_i);
	if (!net_p) { HERE(); return -1; };

	next_idx = 0;
	for (i = 0; i < net_p->len; i++) {
		if (!(net_p->el[i].idx & NET_IDX_IS_PINW))
			continue;
		dev_p = FPGA_DEV(model, net_p->el[i].y,
			net_p->el[i].x, net_p->el[i].dev_idx);
		if ((is_out && (net_p->el[i].idx & NET_IDX_MASK) < dev_p->num_pinw_in)
		    || (!is_out && (net_p->el[i].idx & NET_IDX_MASK) >= dev_p->num_pinw_in))
			continue;
		if (next_idx == idx)
			return i;
		next_idx++;
	}
	return -1;
}

static int fpga_switch_2sets(struct fpga_model* model, int from_y, int from_x,
	str16_t from_pt, int to_y, int to_x, str16_t to_pt,
	struct sw_set* from_set, struct sw_set* to_set)
{
	struct sw_conns conns;
	struct sw_set to_switches;
	int i;

#ifdef DBG_SWITCH_2SETS
	printf("fpga_switch_2sets() from y%i-x%i-%s to y%i-x%i-%s\n",
		from_y, from_x, strarray_lookup(&model->str, from_pt),
		to_y, to_x, strarray_lookup(&model->str, to_pt));
#endif
	// only supports 1 switch on the end side right now
	fpga_swset_fromto(model, to_y, to_x, to_pt, SW_TO, &to_switches);
	RC_ASSERT(model, to_switches.len);

	construct_sw_conns(&conns, model, from_y, from_x, from_pt,
		SW_FROM, /*max_depth*/ 2, NO_NET);
	while (fpga_switch_conns(&conns) != NO_CONN) {
#ifdef DBG_SWITCH_2SETS
		printf(" sw %s conn y%i-x%i-%s\n", fmt_swset(model, from_y,
			from_x, &conns.chain.set, SW_FROM),
			conns.dest_y, conns.dest_x,
			strarray_lookup(&model->str, conns.dest_str_i));
#endif
		if (conns.dest_y != to_y
		    || conns.dest_x != to_x) continue;
		for (i = 0; i < to_switches.len; i++) {
			if (conns.dest_str_i != fpga_switch_str_i(model, to_y,
			    to_x, to_switches.sw[i], SW_FROM))
				continue;
			*from_set = conns.chain.set;
			to_set->len = 1;
			to_set->sw[0] = to_switches.sw[i];
			RC_RETURN(model);
		}
	}
	destruct_sw_conns(&conns);
	from_set->len = 0;
	to_set->len = 0;
	RC_RETURN(model);
}

static int fnet_dir_route(struct fpga_model* model, int from_y, int from_x,
	str16_t from_pt, int to_y, int to_x, str16_t to_pt, net_idx_t net_i)
{
	struct sw_set from_set, to_set;
	struct switch_to_rel switch_to_rel;
	int dist;

	RC_CHECK(model);
	do {
		fpga_switch_2sets(model, from_y, from_x, from_pt, to_y, to_x, to_pt,
			&from_set, &to_set);
		RC_CHECK(model);
		if (from_set.len && to_set.len) {
			fnet_add_sw(model, net_i, from_y, from_x, from_set.sw, from_set.len);
			fnet_add_sw(model, net_i, to_y, to_x, to_set.sw, to_set.len);
			RC_RETURN(model);
		}
		dist = abs(to_y - from_y) + abs(to_x - from_x);

		// Go through all single-depth conns and try
		// switch_2sets from there.
		if (dist <= 4) {
			struct sw_conns sw_conns;

			construct_sw_conns(&sw_conns, model, from_y, from_x,
				from_pt, SW_FROM, /*max_depth*/ 1, net_i);
			RC_CHECK(model);
			while (fpga_switch_conns(&sw_conns) != NO_CONN) {
				if ((fpga_switch_first(model, sw_conns.dest_y, sw_conns.dest_x,
					sw_conns.dest_str_i, SW_FROM) == NO_SWITCH)
				    || (abs(sw_conns.dest_y - from_y) + abs(sw_conns.dest_x - from_x) >= dist))
					continue;
				fpga_switch_2sets(model, sw_conns.dest_y,
					sw_conns.dest_x, sw_conns.dest_str_i,
					to_y, to_x, to_pt, &from_set, &to_set);
				RC_CHECK(model);
				if (from_set.len && to_set.len) {
					fnet_add_sw(model, net_i, from_y, from_x, sw_conns.chain.set.sw, sw_conns.chain.set.len);
					fnet_add_sw(model, net_i, sw_conns.dest_y, sw_conns.dest_x, from_set.sw, from_set.len);
					fnet_add_sw(model, net_i, to_y, to_x, to_set.sw, to_set.len);
					destruct_sw_conns(&sw_conns);
					RC_RETURN(model);
				}
			}
			destruct_sw_conns(&sw_conns);
		}
		switch_to_rel.model = model;
		switch_to_rel.start_y = from_y;
		switch_to_rel.start_x = from_x;
		switch_to_rel.start_switch = from_pt;
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.flags = SWTO_REL_WEAK_TARGET;
		switch_to_rel.rel_y = to_y - from_y;
		switch_to_rel.rel_x = to_x - from_x;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		fpga_switch_to_rel(&switch_to_rel);
		RC_ASSERT(model, switch_to_rel.set.len);
		fnet_add_sw(model, net_i, switch_to_rel.start_y, switch_to_rel.start_x,
			switch_to_rel.set.sw, switch_to_rel.set.len);
		from_y = switch_to_rel.dest_y;
		from_x = switch_to_rel.dest_x;
		from_pt = switch_to_rel.dest_connpt;
	} while (1);
	RC_RETURN(model);
}

static int fpga_switch_2sets_add(struct fpga_model* model, int from_y, int from_x,
	str16_t from_pt, int to_y, int to_x, str16_t to_pt, net_idx_t net_i)
{
	struct sw_set from_set, to_set;

	fpga_switch_2sets(model, from_y, from_x, from_pt, to_y, to_x, to_pt,
		&from_set, &to_set);
	RC_ASSERT(model, from_set.len && to_set.len);
	fnet_add_sw(model, net_i, from_y, from_x, from_set.sw, from_set.len);
	fnet_add_sw(model, net_i, to_y, to_x, to_set.sw, to_set.len);
	RC_RETURN(model);
}

static int fnet_route_iob_to_clock(struct fpga_model *model, net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	struct fpga_device *out_dev, *in_dev;
	struct switch_to_yx_l2 switch_to_yx_l2;
	struct switch_to_rel switch_to_rel;
	int hclk_y;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y, net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y, net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	RC_ASSERT(model, out_dev && in_dev);

	// to regs (l2 allows one intermediate hop)
	switch_to_yx_l2.l1.yx_req = YX_X_CENTER_CMTPLL | YX_Y_CENTER;
	switch_to_yx_l2.l1.flags = SWTO_YX_CLOSEST;
	switch_to_yx_l2.l1.model = model;
	switch_to_yx_l2.l1.y = net_p->el[out_i].y;
	switch_to_yx_l2.l1.x = net_p->el[out_i].x;
	switch_to_yx_l2.l1.start_switch = out_dev->pinw[net_p->el[out_i].idx
		& NET_IDX_MASK];
	switch_to_yx_l2.l1.from_to = SW_FROM;
	switch_to_yx_l2.l1.exclusive_net = NO_NET;
	fpga_switch_to_yx_l2(&switch_to_yx_l2);
	RC_ASSERT(model, switch_to_yx_l2.l1.set.len);
	fnet_add_sw(model, net_i, switch_to_yx_l2.l1.y, switch_to_yx_l2.l1.x,
		switch_to_yx_l2.l1.set.sw, switch_to_yx_l2.l1.set.len);
	if (switch_to_yx_l2.l2_set.len)
		fnet_add_sw(model, net_i, switch_to_yx_l2.l2_y, switch_to_yx_l2.l2_x,
			switch_to_yx_l2.l2_set.sw, switch_to_yx_l2.l2_set.len);

	// to center (possibly over intermediate)
	switch_to_rel.dest_y = switch_to_yx_l2.l1.dest_y;
	switch_to_rel.dest_x = switch_to_yx_l2.l1.dest_x;
	switch_to_rel.dest_connpt = switch_to_yx_l2.l1.dest_connpt;
	do {
		switch_to_rel.model = model;
		switch_to_rel.start_y = switch_to_rel.dest_y;
		switch_to_rel.start_x = switch_to_rel.dest_x;
		switch_to_rel.start_switch = switch_to_rel.dest_connpt;
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.flags = SWTO_REL_WEAK_TARGET;
		switch_to_rel.rel_y = model->center_y - switch_to_rel.start_y;
		switch_to_rel.rel_x = model->center_x - switch_to_rel.start_x;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		fpga_switch_to_rel(&switch_to_rel);
		RC_ASSERT(model, switch_to_rel.set.len);
		fnet_add_sw(model, net_i, switch_to_rel.start_y, switch_to_rel.start_x,
			switch_to_rel.set.sw, switch_to_rel.set.len);
	} while (switch_to_rel.dest_y != model->center_y
		 || switch_to_rel.dest_x != model->center_x);

	// to hclk
	RC_ASSERT(model, switch_to_rel.dest_y == model->center_y
			 && switch_to_rel.dest_x == model->center_x);
	hclk_y = y_to_hclk(net_p->el[in_i].y, model);
	RC_ASSERT(model, hclk_y != -1);
	do {
		switch_to_rel.model = model;
		switch_to_rel.start_y = switch_to_rel.dest_y;
		switch_to_rel.start_x = switch_to_rel.dest_x;
		switch_to_rel.start_switch = switch_to_rel.dest_connpt;
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.flags = SWTO_REL_WEAK_TARGET;
		switch_to_rel.rel_y = hclk_y - switch_to_rel.start_y;
		switch_to_rel.rel_x = 0;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		fpga_switch_to_rel(&switch_to_rel);
		RC_ASSERT(model, switch_to_rel.set.len);
		fnet_add_sw(model, net_i, switch_to_rel.start_y, switch_to_rel.start_x,
			switch_to_rel.set.sw, switch_to_rel.set.len);
	} while (switch_to_rel.dest_y != hclk_y);

	// to routing col
	do {
		switch_to_rel.model = model;
		switch_to_rel.start_y = switch_to_rel.dest_y;
		switch_to_rel.start_x = switch_to_rel.dest_x;
		switch_to_rel.start_switch = switch_to_rel.dest_connpt;
		switch_to_rel.from_to = SW_FROM;
		switch_to_rel.flags = SWTO_REL_WEAK_TARGET;
		switch_to_rel.rel_y = 0;
		switch_to_rel.rel_x = net_p->el[in_i].x-1 - switch_to_rel.start_x;
		switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
		fpga_switch_to_rel(&switch_to_rel);
		RC_ASSERT(model, switch_to_rel.set.len);
		fnet_add_sw(model, net_i, switch_to_rel.start_y, switch_to_rel.start_x,
			switch_to_rel.set.sw, switch_to_rel.set.len);
	} while (switch_to_rel.dest_x != net_p->el[in_i].x-1);

	// to logic device routing
	switch_to_rel.model = model;
	switch_to_rel.start_y = switch_to_rel.dest_y;
	switch_to_rel.start_x = switch_to_rel.dest_x;
	switch_to_rel.start_switch = switch_to_rel.dest_connpt;
	switch_to_rel.from_to = SW_FROM;
	switch_to_rel.flags = SWTO_REL_DEFAULT;
	switch_to_rel.rel_y = net_p->el[in_i].y - switch_to_rel.start_y;
	switch_to_rel.rel_x = net_p->el[in_i].x-1 - switch_to_rel.start_x;
	switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
	fpga_switch_to_rel(&switch_to_rel);
	RC_ASSERT(model, switch_to_rel.set.len);
	fnet_add_sw(model, net_i, switch_to_rel.start_y, switch_to_rel.start_x,
		switch_to_rel.set.sw, switch_to_rel.set.len);

	// connect with pinw
	fpga_switch_2sets_add(model,
		net_p->el[in_i].y, net_p->el[in_i].x-1, switch_to_rel.dest_connpt,
		net_p->el[in_i].y, net_p->el[in_i].x,
		in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK], net_i);
	RC_RETURN(model);
}

static int fnet_route_iob_to_logic(struct fpga_model *model, net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	struct fpga_device *out_dev, *in_dev;
	struct switch_to_yx switch_to;
	struct switch_to_rel switch_to_rel;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y, net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y, net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	RC_ASSERT(model, out_dev && in_dev);

	// switch to ilogic
	switch_to.yx_req = YX_DEV_ILOGIC;
	switch_to.flags = SWTO_YX_DEF;
	switch_to.model = model;
	switch_to.y = net_p->el[out_i].y;
	switch_to.x = net_p->el[out_i].x;
	switch_to.start_switch = out_dev->pinw[net_p->el[out_i].idx
		& NET_IDX_MASK];
	switch_to.from_to = SW_FROM;
	switch_to.exclusive_net = NO_NET;
	fpga_switch_to_yx(&switch_to);
	fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
		switch_to.set.sw, switch_to.set.len);

	// switch to ilogic routing
	switch_to.yx_req = YX_ROUTING_TILE;
	switch_to.flags = SWTO_YX_DEF;
	switch_to.model = model;
	switch_to.y = switch_to.dest_y;
	switch_to.x = switch_to.dest_x;
	switch_to.start_switch = switch_to.dest_connpt;
	switch_to.from_to = SW_FROM;
	switch_to.exclusive_net = net_i;
	fpga_switch_to_yx(&switch_to);
	fnet_add_sw(model, net_i, switch_to.y, switch_to.x,
		switch_to.set.sw, switch_to.set.len);

	// switch backwards from logic tile to logic routing tile
	switch_to_rel.model = model;
	switch_to_rel.start_y = net_p->el[in_i].y;
	switch_to_rel.start_x = net_p->el[in_i].x;
	switch_to_rel.start_switch =
		in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK];
	switch_to_rel.from_to = SW_TO;
	switch_to_rel.flags = SWTO_REL_DEFAULT;
	switch_to_rel.rel_y = 0;
	switch_to_rel.rel_x = -1;
	switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
	fpga_switch_to_rel(&switch_to_rel);
	RC_ASSERT(model, switch_to_rel.set.len);
	fnet_add_sw(model, net_i, switch_to_rel.start_y,
		switch_to_rel.start_x, switch_to_rel.set.sw,
		switch_to_rel.set.len);

	// route from ilogic routing to logic routing
	fpga_switch_2sets_add(model, switch_to.dest_y, switch_to.dest_x,
		switch_to.dest_connpt,
		switch_to_rel.dest_y, switch_to_rel.dest_x,
		switch_to_rel.dest_connpt,
		net_i);
	RC_RETURN(model);
}

static int fnet_route_logic_to_iob(struct fpga_model *model, net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	struct fpga_device *out_dev, *in_dev;
	struct switch_to_yx switch_to_yx;
	struct switch_to_rel switch_to_rel;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y, net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y, net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	RC_ASSERT(model, out_dev && in_dev);

	// switch from ologic to iob
	switch_to_yx.yx_req = YX_DEV_OLOGIC;
	switch_to_yx.flags = SWTO_YX_DEF;
	switch_to_yx.model = model;
	switch_to_yx.y = net_p->el[in_i].y;
	switch_to_yx.x = net_p->el[in_i].x;
	switch_to_yx.start_switch = in_dev->pinw[net_p->el[in_i].idx
		& NET_IDX_MASK];
	switch_to_yx.from_to = SW_TO;
	switch_to_yx.exclusive_net = net_i;
	fpga_switch_to_yx(&switch_to_yx);
	fnet_add_sw(model, net_i, switch_to_yx.y, switch_to_yx.x,
		switch_to_yx.set.sw, switch_to_yx.set.len);

	// switches inside ologic to ologic routing
	switch_to_yx.yx_req = YX_ROUTING_TILE;
	switch_to_yx.flags = SWTO_YX_DEF;
	switch_to_yx.model = model;
	switch_to_yx.y = switch_to_yx.dest_y;
	switch_to_yx.x = switch_to_yx.dest_x;
	switch_to_yx.start_switch = switch_to_yx.dest_connpt;
	switch_to_yx.from_to = SW_TO;
	switch_to_yx.exclusive_net = net_i;
	fpga_switch_to_yx(&switch_to_yx);
	fnet_add_sw(model, net_i, switch_to_yx.y, switch_to_yx.x,
		switch_to_yx.set.sw, switch_to_yx.set.len);

	// switch inside logic tile
	switch_to_rel.model = model;
	switch_to_rel.start_y = net_p->el[out_i].y;
	switch_to_rel.start_x = net_p->el[out_i].x;
	switch_to_rel.start_switch =
		out_dev->pinw[net_p->el[out_i].idx & NET_IDX_MASK];
	switch_to_rel.from_to = SW_FROM;
	switch_to_rel.flags = SWTO_REL_DEFAULT;
	switch_to_rel.rel_y = 0;
	switch_to_rel.rel_x = -1;
	switch_to_rel.target_connpt = STRIDX_NO_ENTRY;
	fpga_switch_to_rel(&switch_to_rel);
	fnet_add_sw(model, net_i, switch_to_rel.start_y,
		switch_to_rel.start_x, switch_to_rel.set.sw,
		switch_to_rel.set.len);

	// route from logic routing to ilogic routing
	fnet_dir_route(model, switch_to_rel.dest_y,
		switch_to_rel.dest_x, switch_to_rel.dest_connpt,
		switch_to_yx.dest_y, switch_to_yx.dest_x, switch_to_yx.dest_connpt,
		net_i);
	RC_RETURN(model);
}

static int fnet_route_logic_carry(struct fpga_model *model, net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	struct fpga_device *out_dev, *in_dev;
	int xm_col, from_str_i, to_str_i;
	swidx_t sw;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);
	out_dev = FPGA_DEV(model, net_p->el[out_i].y, net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y, net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	RC_ASSERT(model, out_dev && in_dev);

	RC_ASSERT(model, (net_p->el[in_i].x == net_p->el[out_i].x)
		&& (net_p->el[in_i].y == regular_row_up(net_p->el[out_i].y, model)));

	xm_col = has_device_type(model, net_p->el[out_i].y, net_p->el[out_i].x, DEV_LOGIC, LOGIC_M);
	if (xm_col) {
		from_str_i = strarray_find(&model->str, "M_COUT");
		to_str_i = strarray_find(&model->str, "M_COUT_N");
	} else { // xl
		from_str_i = strarray_find(&model->str, "XL_COUT");
		to_str_i = strarray_find(&model->str, "XL_COUT_N");
	}
	RC_ASSERT(model, from_str_i != STRIDX_NO_ENTRY && !OUT_OF_U16(from_str_i));
	RC_ASSERT(model, to_str_i != STRIDX_NO_ENTRY && !OUT_OF_U16(to_str_i));

	sw = fpga_switch_lookup(model, net_p->el[out_i].y, net_p->el[out_i].x,
		from_str_i, to_str_i);
	fnet_add_sw(model, net_i, net_p->el[out_i].y, net_p->el[out_i].x,
		&sw, /*len*/ 1);
	RC_RETURN(model);
}

static int fnet_route_logic_to_self(struct fpga_model *model,
	net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	int logic_y, logic_x, logic_dev_idx;
	struct fpga_device *logic_dev;
	struct sw_set sw_set;
	int out_dest_y, out_dest_x, in_dest_y, in_dest_x;
	str16_t out_dest_connpt, in_dest_connpt;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);

	logic_y = net_p->el[out_i].y;
	logic_x = net_p->el[out_i].x;
	logic_dev_idx = net_p->el[out_i].dev_idx;
	RC_ASSERT(model, net_p->el[in_i].y == logic_y
		&& net_p->el[in_i].x == logic_x
		&& net_p->el[in_i].dev_idx == logic_dev_idx);
	logic_dev = FPGA_DEV(model, logic_y, logic_x, logic_dev_idx);
	RC_ASSERT(model, logic_dev);

	fpga_first_conn(model, logic_y, logic_x,
		logic_dev->pinw[net_p->el[out_i].idx & NET_IDX_MASK], SW_FROM,
		/*max_depth*/ 2, net_i,
		&sw_set, &out_dest_y, &out_dest_x, &out_dest_connpt);
	RC_CHECK(model);
	fnet_add_sw(model, net_i, logic_y, logic_x, sw_set.sw, sw_set.len);

	fpga_first_conn(model, logic_y, logic_x,
		logic_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK], SW_TO,
		/*max_depth*/ 2, net_i,
		&sw_set, &in_dest_y, &in_dest_x, &in_dest_connpt);
	RC_ASSERT(model, out_dest_y == in_dest_y
		&& out_dest_x == in_dest_x);
	fnet_add_sw(model, net_i, logic_y, logic_x, sw_set.sw, sw_set.len);

	fpga_multi_switch_lookup(model, out_dest_y, out_dest_x,
		out_dest_connpt, in_dest_connpt, /*max_depth*/ 2, net_i,
		&sw_set);
	RC_ASSERT(model, sw_set.len);
	fnet_add_sw(model, net_i, out_dest_y, out_dest_x, sw_set.sw, sw_set.len);

	RC_RETURN(model);
}

static int fnet_route_to_inpin(struct fpga_model *model, net_idx_t net_i, int out_i, int in_i)
{
	struct fpga_net *net_p;
	struct fpga_device *out_dev, *in_dev;

	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);

	out_dev = FPGA_DEV(model, net_p->el[out_i].y, net_p->el[out_i].x, net_p->el[out_i].dev_idx);
	in_dev = FPGA_DEV(model, net_p->el[in_i].y, net_p->el[in_i].x, net_p->el[in_i].dev_idx);
	RC_ASSERT(model, out_dev && in_dev);

	if (out_dev->type == DEV_IOB) {
		if (in_dev->type != DEV_LOGIC)
			RC_FAIL(model, ENOTSUP);
		if ((net_p->el[in_i].idx & NET_IDX_MASK) == LI_CLK)
			fnet_route_iob_to_clock(model, net_i, out_i, in_i);
		else
			fnet_route_iob_to_logic(model, net_i, out_i, in_i);
	} else if (out_dev->type == DEV_LOGIC) {
		if (in_dev->type == DEV_IOB)
			fnet_route_logic_to_iob(model, net_i, out_i, in_i);
		else if (in_dev->type == DEV_LOGIC) {
			if (net_p->el[in_i].y == net_p->el[out_i].y
			    && net_p->el[in_i].x == net_p->el[out_i].x) {
				fnet_route_logic_to_self(model, net_i, out_i, in_i);
			} else {
				RC_ASSERT(model, (net_p->el[out_i].idx & NET_IDX_MASK) == LO_COUT
					&& (net_p->el[in_i].idx & NET_IDX_MASK) == LI_CIN);
				fnet_route_logic_carry(model, net_i, out_i, in_i);
			}
		} else RC_FAIL(model, ENOTSUP);
	} else
		RC_FAIL(model, ENOTSUP);
	RC_RETURN(model);
}

int fnet_route(struct fpga_model* model, net_idx_t net_i)
{
	int out_i, in_i, in_enum;

	RC_CHECK(model);
	out_i = fnet_pinw(model, net_i, /*is_out*/ 1, /*idx*/ 0);
	if (out_i == -1) { HERE(); return 0; }
	in_i = fnet_pinw(model, net_i, /*is_out*/ 0, /*idx*/ 0);
	if (in_i == -1) { HERE(); return 0; }
	if (fnet_pinw(model, net_i, /*is_out*/ 1, /*idx*/ 1) != -1)
		RC_FAIL(model, EINVAL);

	in_enum = 0;
	do {
		fnet_route_to_inpin(model, net_i, out_i, in_i);
		RC_CHECK(model);
		in_i = fnet_pinw(model, net_i, /*is_out*/ 0, /*idx*/ ++in_enum);
	} while (in_i != -1);
	RC_RETURN(model);
}

int fnet_vcc_gnd(struct fpga_model *model, net_idx_t net_i, int is_vcc)
{
	struct fpga_net *net_p;
	int out_i, in_i, in_enum;
	struct fpga_device *in_dev;
	str16_t from_i;

	RC_CHECK(model);
	out_i = fnet_pinw(model, net_i, /*is_out*/ 1, /*idx*/ 0);
	if (out_i != -1) { HERE(); return 0; }
	in_i = fnet_pinw(model, net_i, /*is_out*/ 0, /*idx*/ 0);
	if (in_i == -1) { HERE(); return 0; }
	net_p = fnet_get(model, net_i);
	RC_ASSERT(model, net_p);

	from_i = strarray_find(&model->str, is_vcc ? "VCC_WIRE" : "GND_WIRE");
	RC_ASSERT(model, !OUT_OF_U16(from_i));
	in_enum = 0;
	do {
		in_dev = FPGA_DEV(model, net_p->el[in_i].y,
			net_p->el[in_i].x, net_p->el[in_i].dev_idx & NET_IDX_MASK);
		RC_ASSERT(model, in_dev);
		if (in_dev->type == DEV_LOGIC) {
			fpga_switch_2sets_add(model,
				net_p->el[in_i].y, net_p->el[in_i].x-1, from_i,
				net_p->el[in_i].y, net_p->el[in_i].x,
				in_dev->pinw[net_p->el[in_i].idx & NET_IDX_MASK],
				net_i);
			RC_CHECK(model);
		} else HERE();
		in_i = fnet_pinw(model, net_i, /*is_out*/ 0, /*idx*/ ++in_enum);
	} while (in_i != -1);
	RC_RETURN(model);
}
