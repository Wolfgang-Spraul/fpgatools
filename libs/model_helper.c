//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

#define NUM_PF_BUFS	32

const char* pf(const char* fmt, ...)
{
	// safe to call it NUM_PF_BUFStimes in 1 expression,
	// such as function params or a net structure
	static char pf_buf[NUM_PF_BUFS][128];
	static int last_buf = 0;
	va_list list;
	last_buf = (last_buf+1)%NUM_PF_BUFS;
	pf_buf[last_buf][0] = 0;
	va_start(list, fmt);
	vsnprintf(pf_buf[last_buf], sizeof(pf_buf[0]), fmt, list);
	va_end(list);
	return pf_buf[last_buf];
}

const char* wpref(struct fpga_model* model, int y, int x, const char* wire_name)
{
	static char buf[8][128];
	static int last_buf = 0;
	const char *prefix;
	int i;

	prefix = "";
	if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
		prefix = is_atx(X_CENTER_REGS_COL, model, x+3)
			? "REGC_INT_" : "REGH_";
	} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
		prefix = "HCLK_";
	else if (is_aty(Y_INNER_TOP, model, y))
		prefix = "IOI_TTERM_";
	else if (is_aty(Y_INNER_BOTTOM, model, y))
		prefix = "IOI_BTERM_";
	else {
		if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL
			   |X_RIGHT_IO_DEVS_COL|X_LEFT_IO_DEVS_COL
			   |X_FABRIC_BRAM_VIA_COL|X_FABRIC_MACC_VIA_COL,
				model, x)) {

			if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_M))
				prefix = "CLEXM_";
			else if (has_device_type(model, y, x, DEV_LOGIC, LOGIC_L))
				prefix = "CLEXL_";
			else if (has_device(model, y, x, DEV_ILOGIC))
				prefix = "IOI_";
			else if (is_atx(X_CENTER_LOGIC_COL, model, x)
				 && is_aty(Y_CHIP_HORIZ_REGS, model, y+1))
				prefix = "INT_INTERFACE_REGC_";
			else
				prefix = "INT_INTERFACE_";
		}
		else if (is_atx(X_CENTER_CMTPLL_COL, model, x))
			prefix = "CMT_PLL_";
		else if (is_atx(X_RIGHT_MCB|X_LEFT_MCB, model, x)) {
			if (y == model->die->mcb_ypos)
				prefix = "MCB_";
			else {
				for (i = 0; i < model->die->num_mui; i++) {
					if (y == model->die->mui_pos[i]+1) {
						prefix = "MCB_MUI_";
						break;
					}
				}
				if (i >= model->die->num_mui)
					prefix = "MCB_INT_";
			}
		} else if (is_atx(X_INNER_RIGHT, model, x))
			prefix = "RTERM_";
		else if (is_atx(X_INNER_LEFT, model, x))
			prefix = "LTERM_";
		else if (is_atx(X_CENTER_REGS_COL, model, x))
			prefix = "CLKV_";
		else if (is_atx(X_FABRIC_BRAM_COL, model, x))
			prefix = "BRAMSITE_";
		else if (is_atx(X_FABRIC_MACC_COL, model, x))
			prefix = "MACCSITE_";
	}

	last_buf = (last_buf+1)%8;
	snprintf(buf[last_buf], sizeof(*buf), "%s%s", prefix, wire_name);
	return buf[last_buf];
}

int has_connpt(struct fpga_model* model, int y, int x,
	const char* name)
{
	struct fpga_tile* tile;
	uint16_t name_i;
	int i;

	i = strarray_find(&model->str, name);
	if (i == STRIDX_NO_ENTRY)
		return 0;
	name_i = i;

	tile = YX_TILE(model, y, x);
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == name_i)
			return 1;
	}
	return 0;
}

#define CONN_NAMES_INCREMENT	128

// add_switch() assumes that the new element is appended
// at the end of the array.
static void connpt_names_array_append(struct fpga_tile* tile, int name_i)
{
	if (!(tile->num_conn_point_names % CONN_NAMES_INCREMENT)) {
		uint16_t* new_ptr = realloc(tile->conn_point_names,
			(tile->num_conn_point_names+CONN_NAMES_INCREMENT)*2*sizeof(uint16_t));
		if (!new_ptr) EXIT(ENOMEM);
		tile->conn_point_names = new_ptr;
	}
	tile->conn_point_names[tile->num_conn_point_names*2] = tile->num_conn_point_dests;
	tile->conn_point_names[tile->num_conn_point_names*2+1] = name_i;
	tile->num_conn_point_names++;
}

static int add_connpt_name_i(struct fpga_model *model, int y, int x,
	str16_t name_i, int warn_dup, int *conn_point_o)
{
	struct fpga_tile *tile;
	int i;

	RC_CHECK(model);
	tile = YX_TILE(model, y, x);

	// All destinations for a connection point must be under
	// one unique entry for that connection point, so we
	// first have to search for existing destinations.
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == name_i)
			break;
	}
	if (conn_point_o) *conn_point_o = i;
	if (i < tile->num_conn_point_names) {
		if (warn_dup)
			fprintf(stderr, "Duplicate connection point name "
				"y%i x%i %s\n", y, x,
				strarray_lookup(&model->str, name_i));
	} else
		// This is the first connection under name, add name.
		connpt_names_array_append(tile, name_i);
	RC_RETURN(model);
}

int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o)
{
	int rc, i;

	RC_CHECK(model);

	rc = strarray_add(&model->str, connpt_name, &i);
	if (rc) RC_FAIL(model, rc);

	RC_ASSERT(model, !OUT_OF_U16(i));
	if (name_i) *name_i = i;

	return add_connpt_name_i(model, y, x, i, warn_if_duplicate, conn_point_o);
}

int has_device(struct fpga_model* model, int y, int x, int dev)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i, type_count;

	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == dev)
			type_count++;
	}
	return type_count;
}

int has_device_type(struct fpga_model* model, int y, int x, int dev, int subtype)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i, type_subtype_count;

	type_subtype_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == dev
		    && tile->devs[i].subtype == subtype)
			type_subtype_count++;
	}
	return type_subtype_count;
}

int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2,
	int dup_warn)
{
	char name_buf[MAX_WIRENAME_LEN];

	RC_CHECK(model);

	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix1);
	add_connpt_name(model, y, x, name_buf, dup_warn,
		/*name_i*/ 0, /*connpt_o*/ 0);

	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix2);
	add_connpt_name(model, y, x, name_buf, dup_warn,
		/*name_i*/ 0, /*connpt_o*/ 0);

	RC_RETURN(model);
}

#define CONNS_INCREMENT		128
#undef DBG_ADD_CONN_UNI

static int add_conn_uni_i(struct fpga_model *model,
	int from_y, int from_x, str16_t from_name, int *from_connpt_o,
	int to_y, int to_x, str16_t to_name)
{
	struct fpga_tile *tile;
	uint16_t *new_ptr;
	int conn_start, num_conn_point_dests_for_this_wire, j;

	RC_CHECK(model);
#ifdef DBG_ADD_CONN_UNI
	fprintf(stderr, "add_conn_uni_i() from y%i x%i %s connpt %i"
		" to y%i x%i %s\n", from_y, from_x,
		strarray_lookup(&model->str, from_name), *from_connpt_o,
		to_y, to_x, strarray_lookup(&model->str, to_name));
#endif
	// this optimization saved about 30% of model creation time
	if (*from_connpt_o == -1) {
		add_connpt_name_i(model, from_y, from_x, from_name,
			0 /* warn_if_duplicate */, from_connpt_o);
		RC_CHECK(model);
	}

	tile = YX_TILE(model, from_y, from_x);
	conn_start = tile->conn_point_names[(*from_connpt_o)*2];
	if ((*from_connpt_o)+1 >= tile->num_conn_point_names)
		num_conn_point_dests_for_this_wire = tile->num_conn_point_dests - conn_start;
	else
		num_conn_point_dests_for_this_wire = tile->conn_point_names[((*from_connpt_o)+1)*2] - conn_start;

	// Is the connection made a second time?
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
		if (tile->conn_point_dests[j*3] == to_x
		    && tile->conn_point_dests[j*3+1] == to_y
		    && tile->conn_point_dests[j*3+2] == to_name) {
			fprintf(stderr, "Duplicate conn (num_conn_point_dests %i): y%i x%i %s - y%i x%i %s.\n",
				num_conn_point_dests_for_this_wire,
				from_y, from_x, strarray_lookup(&model->str, from_name),
				to_y, to_x, strarray_lookup(&model->str, to_name));
			for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
				fprintf(stderr, "c%i: y%i x%i %s -> y%i x%i %s\n", j,
					from_y, from_x, strarray_lookup(&model->str, from_name),
					tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
					strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
			}
			RC_RETURN(model);
		}
	}

	if (!(tile->num_conn_point_dests % CONNS_INCREMENT)) {
		new_ptr = realloc(tile->conn_point_dests,
			(tile->num_conn_point_dests+CONNS_INCREMENT)*3*sizeof(uint16_t));
		if (!new_ptr) RC_FAIL(model, ENOMEM);
		tile->conn_point_dests = new_ptr;
	}
	if (tile->num_conn_point_dests > j)
		memmove(&tile->conn_point_dests[(j+1)*3],
			&tile->conn_point_dests[j*3],
			(tile->num_conn_point_dests-j)*3*sizeof(uint16_t));
	tile->conn_point_dests[j*3] = to_x;
	tile->conn_point_dests[j*3+1] = to_y;
	tile->conn_point_dests[j*3+2] = to_name;
	tile->num_conn_point_dests++;
	for (j = (*from_connpt_o)+1; j < tile->num_conn_point_names; j++)
		tile->conn_point_names[j*2]++;
#ifdef DBG_ADD_CONN_UNI
	fprintf(stderr, " conn_point_dests for y%i x%i %s now:\n",
		from_y, from_x, strarray_lookup(&model->str, from_name));
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire+1; j++) {
		fprintf(stderr, "  c%i: y%i x%i %s -> y%i x%i %s\n",
			j, from_y, from_x, strarray_lookup(&model->str, from_name),
			tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
			strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
	}
#endif
	RC_RETURN(model);
}

static int add_conn_uni(struct fpga_model *model,
	int y1, int x1, const char *name1,
	int y2, int x2, const char *name2)
{
	str16_t name1_i, name2_i;
	int j, from_connpt_o, rc;

	RC_CHECK(model);

	rc = strarray_add(&model->str, name1, &j);
	if (rc) RC_FAIL(model, rc);
	RC_ASSERT(model, !OUT_OF_U16(j));
	name1_i = j;
	rc = strarray_add(&model->str, name2, &j);
	if (rc) RC_FAIL(model, rc);
	RC_ASSERT(model, !OUT_OF_U16(j));
	name2_i = j;

	from_connpt_o = -1;
	return add_conn_uni_i(model, y1, x1, name1_i, &from_connpt_o, y2, x2, name2_i);
}

int add_conn_bi(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2)
{
	add_conn_uni(model, y1, x1, name1, y2, x2, name2);
	add_conn_uni(model, y2, x2, name2, y1, x1, name1);
	RC_RETURN(model);
}

int add_conn_bi_pref(struct fpga_model* model,
	int y1, int x1, const char* name1,
	int y2, int x2, const char* name2)
{
	add_conn_bi(model, y1, x1, wpref(model, y1, x1, name1),
			   y2, x2, wpref(model, y2, x2, name2));
	RC_RETURN(model);
}

int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func,
	int y1, int x1, const char* name1, int start1, int last1,
	int y2, int x2, const char* name2, int start2)
{
	char buf1[MAX_WIRENAME_LEN], buf2[MAX_WIRENAME_LEN];
	int i;

	RC_CHECK(model);
	for (i = start1; i <= last1; i++) {
		snprintf(buf1, sizeof(buf1), name1, i);
		if (start2 & COUNT_DOWN)
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)-(i-start1));
		else
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)+(i-start1));
		(*add_conn_func)(model, y1, x1, buf1, y2, x2, buf2);
	}
	RC_RETURN(model);
}

int add_conn_net(struct fpga_model* model, int add_pref, const struct w_net *net)
{
	int i, j, rc;

	RC_CHECK(model);
	if (net->num_pts < 2) RC_FAIL(model, EINVAL);
	if (!net->last_inc) {
		str16_t net_name_i[MAX_NET_POINTS];
		int str_i, net_connpt_o[MAX_NET_POINTS];

		for (i = 0; i < net->num_pts; i++) {
			rc = strarray_add(&model->str, add_pref
				? wpref(model,
					net->pt[i].y, net->pt[i].x,
					net->pt[i].name)
				: net->pt[i].name, &str_i);
			if (rc) RC_FAIL(model, rc);
			RC_ASSERT(model, !OUT_OF_U16(str_i));
			net_name_i[i] = str_i;

			net_connpt_o[i] = -1;
		}
		for (i = 0; i < net->num_pts; i++) {
			for (j = i+1; j < net->num_pts; j++) {
				if (net->pt[j].y == net->pt[i].y
				    && net->pt[j].x == net->pt[i].x)
					continue;
				add_conn_uni_i(model,
					net->pt[i].y, net->pt[i].x, net_name_i[i],
					&net_connpt_o[i],
					net->pt[j].y, net->pt[j].x, net_name_i[j]);
				add_conn_uni_i(model,
					net->pt[j].y, net->pt[j].x, net_name_i[j],
					&net_connpt_o[j],
					net->pt[i].y, net->pt[i].x, net_name_i[i]);
			}
		}
	} else { // last_inc != 0
		for (i = 0; i < net->num_pts; i++) {
			for (j = i+1; j < net->num_pts; j++) {
				// We are buildings nets like a NN2 B-M-E net where
				// we add the _S0 wire at the end, at the same x/y
				// coordinate as the M point. Here we skip such
				// connections back to the start tile.
				if (net->pt[j].y == net->pt[i].y
				    && net->pt[j].x == net->pt[i].x)
					continue;
				add_conn_range(model,
					add_pref ? add_conn_bi_pref : add_conn_bi,
					net->pt[i].y, net->pt[i].x,
					net->pt[i].name,
					net->pt[i].start_count,
					net->pt[i].start_count + net->last_inc,
					net->pt[j].y, net->pt[j].x,
					net->pt[j].name,
					net->pt[j].start_count);
			}
		}
	}
	RC_RETURN(model);
}

#define SWITCH_ALLOC_INCREMENT 256

#define DBG_ALLOW_ADDPOINTS
// Enable CHECK_DUPLICATES when working on the switch architecture,
// but otherwise keep it disabled since it slows down building the
// model a lot.
#undef CHECK_DUPLICATES

int add_switch(struct fpga_model* model, int y, int x, const char* from,
	const char* to, int is_bidirectional)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int rc, i, from_idx, to_idx, from_connpt_o, to_connpt_o;
	uint32_t new_switch;

	RC_CHECK(model);
// later this can be strarray_find() and not strarray_add(), but
// then we need all wires and ports to be present first...
#ifdef DBG_ALLOW_ADDPOINTS
	rc = strarray_add(&model->str, from, &from_idx);
	if (rc) goto xout;
	rc = strarray_add(&model->str, to, &to_idx);
	if (rc) goto xout;
#else
	from_idx = strarray_find(&model->str, from);
	to_idx = strarray_find(&model->str, to);
#endif
	if (from_idx == STRIDX_NO_ENTRY || to_idx == STRIDX_NO_ENTRY) {
		fprintf(stderr, "No string for switch from %s (%i) or %s (%i).\n",
			from, from_idx, to, to_idx);
		return -1;
	}

	// It seems searching backwards is a little faster than
	// searching forwards. Merging the two loops into one
	// made the total slower, presumably due to cache issues.
	from_connpt_o = -1;
	for (i = tile->num_conn_point_names-1; i >= 0; i--) {
		if (tile->conn_point_names[i*2+1] == from_idx) {
			from_connpt_o = i;
			break;
		}
	}
	to_connpt_o = -1;
	for (i = tile->num_conn_point_names-1; i >= 0; i--) {
		if (tile->conn_point_names[i*2+1] == to_idx) {
			to_connpt_o = i;
			break;
		}
	}
#ifdef DBG_ALLOW_ADDPOINTS
	if (from_connpt_o == -1) {
		from_connpt_o = tile->num_conn_point_names;
		connpt_names_array_append(tile, from_idx);
	}
	if (to_connpt_o == -1) {
		to_connpt_o = tile->num_conn_point_names;
		connpt_names_array_append(tile, to_idx);
	}
#endif
	if (from_connpt_o == -1 || to_connpt_o == -1) {
		fprintf(stderr, "No conn point for switch from %s (%i/%i) or %s (%i/%i).\n",
			from, from_idx, from_connpt_o, to, to_idx, to_connpt_o);
		return -1;
	}
	if (from_connpt_o > SWITCH_MAX_CONNPT_O
	    || to_connpt_o > SWITCH_MAX_CONNPT_O) {
		fprintf(stderr, "Internal error in %s:%i (from_o %i to_o %i)\n",
			__FILE__, __LINE__, from_connpt_o, to_connpt_o);
		return -1;
	}
	new_switch = (from_connpt_o << 15) | to_connpt_o;
	if (is_bidirectional)
		new_switch |= SWITCH_BIDIRECTIONAL;

#ifdef CHECK_DUPLICATES
	for (i = 0; i < tile->num_switches; i++) {
		if ((tile->switches[i] & 0x3FFFFFFF) == (new_switch & 0x3FFFFFFF)) {
			fprintf(stderr, "Internal error in %s:%i duplicate switch from %s to %s\n",
				__FILE__, __LINE__, from, to);
			return -1;
		}
	}
#endif
	if (!(tile->num_switches % SWITCH_ALLOC_INCREMENT)) {
		uint32_t* new_ptr = realloc(tile->switches,
			(tile->num_switches+SWITCH_ALLOC_INCREMENT)*sizeof(*tile->switches));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
		tile->switches = new_ptr;
	}
	tile->switches[tile->num_switches++] = new_switch;
	return 0;
xout:
	return rc;
}

int add_switch_set(struct fpga_model* model, int y, int x, const char* prefix,
	const char** pairs, int suffix_inc)
{
	int i, j, from_len, to_len, rc;
	char from[64], to[64];

	RC_CHECK(model);
	if (!prefix) prefix = "";
	for (i = 0; pairs[i*2][0]; i++) {
		snprintf(from, sizeof(from), "%s%s", prefix, pairs[i*2]);
		snprintf(to, sizeof(to), "%s%s", prefix, pairs[i*2+1]);
		if (!suffix_inc) {
			rc = add_switch(model, y, x, from, to,
				0 /* bidir */);
			if (rc) goto xout;
		} else {
			from_len = strlen(from);
			to_len = strlen(to);
			for (j = 0; j <= suffix_inc; j++) {
				snprintf(&from[from_len], sizeof(from)-from_len, "%i", j);
				snprintf(&to[to_len], sizeof(to)-to_len, "%i", j);
				rc = add_switch(model, y, x, from, to,
					0 /* bidir */);
				if (rc) goto xout;
			}
		}
	}
	return 0;
xout:
	return rc;
}

int replicate_switches_and_names(struct fpga_model* model,
	int y_from, int x_from, int y_to, int x_to)
{
	struct fpga_tile* from_tile, *to_tile;
	int rc;

	RC_CHECK(model);
	from_tile = YX_TILE(model, y_from, x_from);
	to_tile = YX_TILE(model, y_to, x_to);
	if (to_tile->num_conn_point_names
	    || to_tile->num_conn_point_dests
	    || to_tile->num_switches
	    || from_tile->num_conn_point_dests
	    || !from_tile->num_conn_point_names
	    || !from_tile->num_switches) FAIL(EINVAL);

	to_tile->conn_point_names = malloc(((from_tile->num_conn_point_names/CONN_NAMES_INCREMENT)+1)*CONN_NAMES_INCREMENT*2*sizeof(uint16_t));
	if (!to_tile->conn_point_names) EXIT(ENOMEM);
	memcpy(to_tile->conn_point_names, from_tile->conn_point_names, from_tile->num_conn_point_names*2*sizeof(uint16_t));
	to_tile->num_conn_point_names = from_tile->num_conn_point_names;

	to_tile->switches = malloc(((from_tile->num_switches/SWITCH_ALLOC_INCREMENT)+1)*SWITCH_ALLOC_INCREMENT*sizeof(*from_tile->switches));
	if (!to_tile->switches) EXIT(ENOMEM);
	memcpy(to_tile->switches, from_tile->switches, from_tile->num_switches*sizeof(*from_tile->switches));
	to_tile->num_switches = from_tile->num_switches;
	return 0;
fail:
	return rc;
}

void seed_strx(struct fpga_model *model, const struct seed_data *data)
{
	int x, i;
	for (x = 0; x < model->x_width; x++) {
		model->tmp_str[x] = 0;
		for (i = 0; data[i].flags; i++) {
			if (is_atx(data[i].flags, model, x))
				model->tmp_str[x] = data[i].str;
		}
	}
}

void seed_stry(struct fpga_model *model, const struct seed_data *data)
{
	int y, i;
	for (y = 0; y < model->y_height; y++) {
		model->tmp_str[y] = 0;
		for (i = 0; data[i].flags; i++) {
			if (is_aty(data[i].flags, model, y))
				model->tmp_str[y] = data[i].str;
		}
	}
}

char next_non_whitespace(const char* s)
{
	int i;
	for (i = 0; s[i] == ' '; i++);
	return s[i];
}

char last_major(const char* str, int cur_o)
{
	for (; cur_o; cur_o--) {
		if (str[cur_o-1] >= 'A' && str[cur_o-1] <= 'Z')
			return str[cur_o-1];
	}
	return 0;
}

int is_aty(int check, struct fpga_model* model, int y)
{
	if (y < 0) return 0;
	if (check & Y_OUTER_TOP && y == TOP_OUTER_ROW) return 1;
	if (check & Y_INNER_TOP && y == TOP_INNER_ROW) return 1;
	if (check & Y_INNER_BOTTOM && y == model->y_height-BOT_INNER_ROW) return 1;
	if (check & Y_OUTER_BOTTOM && y == model->y_height-BOT_OUTER_ROW) return 1;
	if (check & Y_CHIP_HORIZ_REGS && y == model->center_y) return 1;
	if (check & (Y_ROW_HORIZ_AXSYMM|Y_BOTTOM_OF_ROW|Y_REGULAR_ROW)) {
		int row_pos;
		is_in_row(model, y, 0 /* row_num */, &row_pos);
		if (check & Y_ROW_HORIZ_AXSYMM && row_pos == 8) return 1;
		if (check & Y_BOTTOM_OF_ROW && row_pos == 16) return 1;
		if (check & Y_REGULAR_ROW && ((row_pos >= 0 && row_pos < 8) || (row_pos > 8 && row_pos <= 16))) return 1;
	}
	if (check & Y_LEFT_WIRED && model->tiles[y*model->x_width].flags & TF_WIRED) return 1;
	if (check & Y_RIGHT_WIRED && model->tiles[y*model->x_width + model->x_width-RIGHT_OUTER_O].flags & TF_WIRED) return 1;
	if (check & Y_TOPBOT_IO_RANGE
	    && ((y > TOP_INNER_ROW && y <= TOP_INNER_ROW + TOP_IO_TILES)
	        || (y >= model->y_height - BOT_INNER_ROW - BOT_IO_TILES && y < model->y_height - BOT_INNER_ROW))) return 1;
	if (check & Y_TOP_OUTER_IO && y == TOP_OUTER_IO) return 1;
	if (check & Y_TOP_INNER_IO && y == TOP_INNER_IO) return 1;
	if (check & Y_BOT_INNER_IO && y == model->y_height-BOT_INNER_IO) return 1;
	if (check & Y_BOT_OUTER_IO && y == model->y_height-BOT_OUTER_IO) return 1;
	return 0;
}

int is_atx(int check, struct fpga_model* model, int x)
{
	if (x < 0) return 0;
	if (check & X_OUTER_LEFT && !x) return 1;
	if (check & X_INNER_LEFT && x == 1) return 1;
	if (check & X_INNER_RIGHT && x == model->x_width-2) return 1;
	if (check & X_OUTER_RIGHT && x == model->x_width-1) return 1;
	if (check & X_ROUTING_NO_IO
	    && model->tiles[x].flags & TF_ROUTING_NO_IO) return 1;
	if (check & X_FABRIC_LOGIC_XM_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_LOGIC_XM_COL) return 1;
	if (check & X_FABRIC_LOGIC_XL_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_LOGIC_XL_COL) return 1;
	if (check & X_FABRIC_LOGIC_XM_COL
	    && model->tiles[x].flags & TF_FABRIC_LOGIC_XM_COL) return 1;
	if (check & X_FABRIC_LOGIC_XL_COL
	    && model->tiles[x].flags & TF_FABRIC_LOGIC_XL_COL) return 1;
	if (check & X_FABRIC_BRAM_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_BRAM_VIA_COL
	    && model->tiles[x+2].flags & TF_FABRIC_BRAM_COL) return 1;
	if (check & X_FABRIC_MACC_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_MACC_VIA_COL
	    && model->tiles[x+2].flags & TF_FABRIC_MACC_COL) return 1;
	if (check & X_FABRIC_BRAM_VIA_COL && model->tiles[x].flags & TF_FABRIC_BRAM_VIA_COL) return 1;
	if (check & X_FABRIC_MACC_VIA_COL && model->tiles[x].flags & TF_FABRIC_MACC_VIA_COL) return 1;
	if (check & X_FABRIC_BRAM_COL && model->tiles[x].flags & TF_FABRIC_BRAM_COL) return 1;
	if (check & X_FABRIC_MACC_COL && model->tiles[x].flags & TF_FABRIC_MACC_COL) return 1;
	if (check & X_CENTER_ROUTING_COL && x == model->center_x-3) return 1;
	if (check & X_CENTER_LOGIC_COL && x == model->center_x-2) return 1;
	if (check & X_CENTER_CMTPLL_COL && x == model->center_x-1) return 1;
	if (check & X_CENTER_REGS_COL && x == model->center_x) return 1;
	if (check & X_LEFT_IO_ROUTING_COL && x == LEFT_IO_ROUTING) return 1;
	if (check & X_LEFT_IO_DEVS_COL && x == LEFT_IO_DEVS) return 1;
	if (check & X_RIGHT_IO_ROUTING_COL
	    && x == model->x_width-RIGHT_IO_ROUTING_O) return 1;
	if (check & X_RIGHT_IO_DEVS_COL
	    && x == model->x_width-RIGHT_IO_DEVS_O) return 1;
	if (check & X_LEFT_SIDE && x < model->center_x) return 1;
	if (check & X_LEFT_MCB && x == LEFT_MCB_COL) return 1;
	if (check & X_RIGHT_MCB && x == model->x_width-RIGHT_MCB_O) return 1;
	return 0;
}

int is_atyx(int check, struct fpga_model* model, int y, int x)
{
	struct fpga_tile* tile;

	if (y < 0 || x < 0) return 0;
	// todo: YX_ROUTING_TILE could be implemented using X_ROUTING_COL and Y_REGULAR_ROW ?
	if (check & YX_ROUTING_TILE
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->x_width-RIGHT_IO_ROUTING_O
		|| x == model->center_x-CENTER_ROUTING_O)) {
		int row_pos;
		is_in_row(model, y, 0 /* row_num */, &row_pos);
		if (row_pos >= 0 && row_pos != 8) return 1;
	}
	tile = YX_TILE(model, y, x);
	if (check & YX_IO_ROUTING
            && (tile->type == IO_ROUTING || tile->type == ROUTING_IO_L)) return 1;
	if (check & YX_ROUTING_TO_FABLOGIC
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && has_device(model, y, x+1, DEV_LOGIC)) return 1;
	if (check & YX_DEV_ILOGIC && has_device(model, y, x, DEV_ILOGIC)) return 1;
	if (check & YX_DEV_OLOGIC && has_device(model, y, x, DEV_OLOGIC)) return 1;
	if (check & YX_DEV_LOGIC && has_device(model, y, x, DEV_LOGIC)) return 1;
	if (check & YX_DEV_IOB && has_device(model, y, x, DEV_IOB)) return 1;
	if (check & YX_CENTER_MIDBUF && tile->flags & TF_CENTER_MIDBUF) return 1;
	if (check & YX_OUTER_TERM
	    && (is_atx(X_OUTER_LEFT|X_OUTER_RIGHT, model, x)
		|| is_aty(Y_OUTER_TOP|Y_OUTER_BOTTOM, model, y))) return 1;
	if (check & YX_INNER_TERM
	    && (is_atx(X_INNER_LEFT|X_INNER_RIGHT, model, x)
		|| is_aty(Y_INNER_TOP|Y_INNER_BOTTOM, model, y))) return 1;
	if (check & YX_OUTSIDE_OF_ROUTING
	    && (x < LEFT_IO_ROUTING
		|| x > model->x_width-RIGHT_IO_ROUTING_O
		|| y <= TOP_INNER_ROW
		|| y >= model->y_height-BOT_INNER_ROW )) return 1;
	if (check & YX_ROUTING_BOUNDARY
	    && is_atyx(YX_ROUTING_TILE, model, y, x)
	    && (x == LEFT_IO_ROUTING || x == model->x_width-RIGHT_IO_ROUTING_O
	        || y == TOP_FIRST_REGULAR || y == model->y_height-BOT_LAST_REGULAR_O)) return 1;
	if (check & YX_X_CENTER_CMTPLL
	    && is_atx(X_CENTER_CMTPLL_COL, model, x)) return 1;
	if (check & YX_Y_CENTER
	    && is_aty(Y_CHIP_HORIZ_REGS, model, y)) return 1;
	if (check & YX_CENTER
	    && is_atx(X_CENTER_REGS_COL, model, x)
	    && is_aty(Y_CHIP_HORIZ_REGS, model, y)) return 1;
	return 0;
}

void is_in_row(const struct fpga_model* model, int y,
	int* row_num, int* row_pos)
{
	int dist_to_center;

	if (row_num) *row_num = -1;
	if (row_pos) *row_pos = -1;
	if (y < TOP_IO_TILES) return;
	// normalize y to beginning of rows
	y -= TOP_IO_TILES;

	// calculate distance to center and check
	// that y is not pointing to the center
	dist_to_center = (model->die->num_rows/2)*(8+1/*middle of row*/+8);
	if (y == dist_to_center) return;
	if (y > dist_to_center) y--;

	// check that y is not pointing past the last row
	if (y >= model->die->num_rows*(8+1+8)) return;

	if (row_num) *row_num = model->die->num_rows-(y/(8+1+8))-1;
	if (row_pos) *row_pos = y%(8+1+8);
}

int which_row(int y, struct fpga_model* model)
{
	int result;
	is_in_row(model, y, &result, 0 /* row_pos */);
	return result;
}

int pos_in_row(int y, struct fpga_model* model)
{
	int result;
	is_in_row(model, y, 0 /* row_num */, &result);
	return result;
}

int regular_row_pos(int y, struct fpga_model* model)
{
	int row_pos = pos_in_row(y, model);
	if (row_pos == -1 || row_pos == HCLK_POS) return -1;
	if (row_pos > HCLK_POS) row_pos--;
	return row_pos;
}

int row_to_hclk(int row, struct fpga_model *model)
{
	int hclk_pos = model->y_height - BOT_LAST_REGULAR_O - row*ROW_SIZE - HCLK_POS;
	if (hclk_pos < model->center_y)
		hclk_pos--; // center regs
	if (hclk_pos < TOP_FIRST_REGULAR)
		{ HERE(); return -1; }
	return hclk_pos;
}

int y_to_hclk(int y, struct fpga_model *model)
{
	int row_num, row_pos;

	is_in_row(model, y, &row_num, &row_pos);
	if (row_num == -1
	    || row_pos == -1 || row_pos == HCLK_POS)
		{ HERE(); return -1; }
	return row_to_hclk(row_num, model);
}

const char* logicin_s(int wire, int routing_io)
{
	if (routing_io && ((wire & LWF_WIRE_MASK) == X_A5 || (wire & LWF_WIRE_MASK) == X_B4))
		return pf("INT_IOI_LOGICIN_B%i", wire & LWF_WIRE_MASK);
	return pf("LOGICIN_B%i", wire & LWF_WIRE_MASK);
}

const char* logicin_str(enum logicin_wire w)
{
	switch (w) {
		case M_A1:
		case X_A1: return "A1";
		case M_A2:
		case X_A2: return "A2";
		case M_A3:
		case X_A3: return "A3";
		case M_A4:
		case X_A4: return "A4";
		case M_A5:
		case X_A5: return "A5";
		case M_A6:
		case X_A6: return "A6";
		case M_AX:
		case X_AX: return "AX";
		case M_B1:
		case X_B1: return "B1";
		case M_B2:
		case X_B2: return "B2";
		case M_B3:
		case X_B3: return "B3";
		case M_B4:
		case X_B4: return "B4";
		case M_B5:
		case X_B5: return "B5";
		case M_B6:
		case X_B6: return "B6";
		case M_BX:
		case X_BX: return "BX";
		case M_C1:
		case X_C1: return "C1";
		case M_C2:
		case X_C2: return "C2";
		case M_C3:
		case X_C3: return "C3";
		case M_C4:
		case X_C4: return "C4";
		case M_C5:
		case X_C5: return "C5";
		case M_C6:
		case X_C6: return "C6";
		case M_CE:
		case X_CE: return "CE";
		case M_CX:
		case X_CX: return "CX";
		case M_D1:
		case X_D1: return "D1";
		case M_D2:
		case X_D2: return "D2";
		case M_D3:
		case X_D3: return "D3";
		case M_D4:
		case X_D4: return "D4";
		case M_D5:
		case X_D5: return "D5";
		case M_D6:
		case X_D6: return "D6";
		case M_DX:
		case X_DX: return "DX";

		case M_AI: return "AI";
		case M_BI: return "BI";
		case M_CI: return "CI";
		case M_DI: return "DI";
		case M_WE: return "WE";
	}
	EXIT(1);
	return 0;
}

const char* logicout_str(enum logicout_wire w)
{
	switch (w) {
		case X_A:   
		case M_A:    return "A";
		case X_AMUX:
		case M_AMUX: return "AMUX";
		case X_AQ:
		case M_AQ:   return "AQ";
		case X_B:
		case M_B:    return "B";
		case X_BMUX:
		case M_BMUX: return "BMUX";
		case X_BQ:
		case M_BQ:   return "BQ";
		case X_C:
		case M_C:    return "C";
		case X_CMUX:
		case M_CMUX: return "CMUX";
		case X_CQ:
		case M_CQ:   return "CQ";
		case X_D:
		case M_D:    return "D";
		case X_DMUX:
		case M_DMUX: return "DMUX";
		case X_DQ:
		case M_DQ:   return "DQ";
	}
	EXIT(1);
	return 0;
}

const char *fpga_connpt_str(struct fpga_model *model, enum extra_wires wire,
	int y, int x, int dest_y, int dest_x)
{
 	enum { NUM_BUFS = 8, BUF_SIZE = MAX_WIRENAME_LEN };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;
	const char *wstr;
	int i, wnum, wchar;

	if (dest_y == -1) dest_y = y;
	if (dest_x == -1) dest_x = x;
	last_buf = (last_buf+1)%NUM_BUFS;
	buf[last_buf][0] = 0;

	if (wire == CLK0 || wire == CLK1
	    || wire == SR0 || wire == SR1) {
		if (is_atx(X_LEFT_IO_DEVS_COL|X_FABRIC_LOGIC_COL|X_FABRIC_MACC_VIA_COL
			   |X_FABRIC_BRAM_VIA_COL|X_CENTER_LOGIC_COL|X_RIGHT_IO_DEVS_COL, model, x))
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"INT_INTERFACE_%s", fpga_wire2str(wire));
		else if (is_atx(X_LEFT_MCB|X_RIGHT_MCB, model, x)) {
			for (i = 0; i < model->die->num_mui; i++) {
				if (y == model->die->mui_pos[i]+1) {
					if (y-dest_y < 0 || y-dest_y > 1) HERE();
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"MUI_%s_INT%i", fpga_wire2str(wire), y-dest_y);
					break;
				}
			}
			if (i >= model->die->num_mui)
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			if (has_device(model, y, x, DEV_MACC)) {
				if (y-dest_y < 0 || y-dest_y >= 4) HERE();
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"MACC_%s_INT%i", fpga_wire2str(wire), y-dest_y);
			} else
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else
			strcpy(buf[last_buf], fpga_wire2str(wire));
	} else if (wire >= LOGICOUT_B0 && wire <= LOGICOUT_B0 + LOGICOUT_HIGHEST) {
		wnum = wire - LOGICOUT_B0;
		if (is_atx(X_ROUTING_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"LOGICOUT%i", wnum);
		} else if (is_atx(X_LEFT_IO_DEVS_COL|X_RIGHT_IO_DEVS_COL, model, x)) {
			if (is_atx(X_LEFT_MCB|X_RIGHT_MCB, model, dest_x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"INT_INTERFACE_LOGICOUT_%i", wnum);
			else
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"INT_INTERFACE_LOGICOUT%i", wnum);
		} else if (is_atx(X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"INT_INTERFACE_LOGICOUT%i", wnum);
		} else if (is_atx(X_FABRIC_MACC_VIA_COL|X_FABRIC_BRAM_VIA_COL, model, x)) {
			if (is_atx(X_FABRIC_MACC_COL, model, dest_x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"INT_INTERFACE_LOGICOUT_%i", wnum);
			else
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"INT_INTERFACE_LOGICOUT%i", wnum);
		} else if (is_atx(X_LEFT_MCB|X_RIGHT_MCB, model, x)) {
			for (i = 0; i < model->die->num_mui; i++) {
				if (y == model->die->mui_pos[i]+1) {
					if (y-dest_y < 0 || y-dest_y > 1) HERE();
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"MUI_LOGICOUT%i_INT%i", wnum, y-dest_y);
					break;
				}
			}
			if (i >= model->die->num_mui)
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			if (has_device(model, y, x, DEV_MACC)) {
				if (y-dest_y < 0 || y-dest_y >= 4) HERE();
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"MACC_LOGICOUT%i_INT%i", wnum, y-dest_y);
			} else
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else
			strcpy(buf[last_buf], fpga_wire2str(wire));
	} else if (wire >= LOGICIN_B0 && wire <= LOGICIN_B0 + LOGICIN_HIGHEST) {
		wnum = wire - LOGICIN_B0;
		if (is_atx(X_ROUTING_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"LOGICIN_B%i", wnum);
		} else if (is_atx(X_LEFT_IO_DEVS_COL|X_FABRIC_LOGIC_COL|X_CENTER_LOGIC_COL
				  |X_RIGHT_IO_DEVS_COL|X_FABRIC_MACC_VIA_COL
				  |X_FABRIC_BRAM_VIA_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"INT_INTERFACE_LOGICBIN%i", wnum);
		} else if (is_atx(X_LEFT_MCB|X_RIGHT_MCB, model, x)) {
			for (i = 0; i < model->die->num_mui; i++) {
				if (y == model->die->mui_pos[i]+1) {
					if (y-dest_y < 0 || y-dest_y > 1) HERE();
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"MUI_LOGICINB%i_INT%i", wnum, y-dest_y);
					break;
				}
			}
			if (i >= model->die->num_mui)
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else if (is_atx(X_FABRIC_MACC_COL, model, x)) {
			if (has_device(model, y, x, DEV_MACC)) {
				if (y-dest_y < 0 || y-dest_y >= 4) HERE();
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"MACC_LOGICINB%i_INT%i", wnum, y-dest_y);
			} else
				strcpy(buf[last_buf], fpga_wire2str(wire));
		} else
			strcpy(buf[last_buf], fpga_wire2str(wire));
	} else if ((wire >= CFB0 && wire <= CFB15)
	    || (wire >= DFB0 && wire <= DFB7)) {
		if (wire >= CFB0 && wire <= CFB15) {
			wstr = "CFB";
			wnum = wire-CFB0;
		} else if (wire >= DFB0 && wire <= DFB7) {
			wstr = "DFB";
			wnum = wire-DFB0;
		} else HERE();

		if (is_aty(Y_OUTER_TOP, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_aty(Y_INNER_TOP, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_REGT_%s%s_%c%i",
						wstr,
						wnum >= 8 ? "1" : "",
						wnum%2 ? 'S' : 'M',
						(wnum%4)/2+1);
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_S");
				} else HERE();
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_TTERM_%s", fpga_wire2str(wire));
			else if (is_atx(X_CENTER_REGS_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGV_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_1)
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"IOI_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum%8 >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_REGT_%s%s_%c%i",
						wstr,
						wnum >= 8 ? "1" : "",
						wnum % 2 ? 'S' : 'M',
						(wnum%4)/2+1);
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_S");
				} else HERE();
			} else HERE();
		} else if (is_aty(Y_TOP_OUTER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"TIOI_%s_%s%s_%c%s",
				wnum%4 < 2 ? "OUTER" : "INNER",
				wstr,
				wnum >= 8 ? "1" : "",
				wnum % 2 ? 'S' : 'M',
				(wnum%4)/2 ? "_EXT" : "");
		} else if (is_aty(Y_TOP_INNER_IO, model, y)) {
			if (wnum%4 >= 2) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"TIOI_INNER_%s%s_%c", wstr,
					wnum >= 8 ? "1" : "",
					wnum%2 ? 'S' : 'M');
			} else HERE();
		} else if (is_aty(Y_BOT_INNER_IO, model, y)) {
			if (wnum%4 >= 2) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"BIOI_INNER_%s%s_%c", wstr,
					wnum >= 8 ? "1" : "",
					wnum%2 ? 'S' : 'M');
			} else HERE();
		} else if (is_aty(Y_BOT_OUTER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"BIOI_%s_%s%s_%c%s",
				wnum%4 < 2 ? "OUTER" : "INNER",
				wstr,
				wnum >= 8 ? "1" : "",
				wnum%2 ? 'S' : 'M',
				(wnum%4)/2 ? "_EXT" : "");
		} else if (is_aty(Y_INNER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
				if (wnum%8 >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_N");
				} else HERE();
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_BTERM_%s", fpga_wire2str(wire));
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"REGV_BTERM_%s", fpga_wire2str(wire+4));
				} else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_1) {
				if (wnum < 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_BTERM_%s", fpga_wire2str(wire+4));
				else if (wnum >= 8 && wnum <= 11)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_BTERM_BUFPLL_%s", fpga_wire2str(wire+4));
				else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire+4));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_N");
				} else HERE();
			} else HERE();
		} else if (is_aty(Y_OUTER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_OUTER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGL_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_INNER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_LTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_TOP_%s", fpga_wire2str(wire));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum%8 < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_BOT_%s", fpga_wire2str(wire));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum%8 >= 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s_EXT", fpga_wire2str(wire-4));
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum%8 >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_BOT_%s", fpga_wire2str(wire-4));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum%8 >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_TOP_%s", fpga_wire2str(wire-4));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else HERE();
		} else if (is_atx(X_LEFT_IO_ROUTING_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
			  "INT_%s%s_%c", wstr, wnum >= 8 ? "1" : "",
			  wnum%2 ? 'S' : 'M');
		} else if (is_atx(X_LEFT_IO_DEVS_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
			  "LIOI_%s%s_%c_ILOGIC", wstr, wnum >= 8 ? "1" : "",
			  wnum%2 ? 'S' : 'M');
		} else if (is_atx(X_RIGHT_IO_DEVS_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
			  "RIOI_%s%s_%c_ILOGIC", wstr, wnum >= 8 ? "1" : "",
			  wnum%2 ? 'S' : 'M');
		} else if (is_atx(X_INNER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_RTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum%8 >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_TOP_%s", fpga_wire2str(wire-4));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum%8 >= 6) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_BOT_%s", fpga_wire2str(wire-4));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum%8 < 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s%s", fpga_wire2str(wire),
					  wnum < 8 ? "_EXT" : "");
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_BOT_%s", fpga_wire2str(wire));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_TOP_%s", fpga_wire2str(wire));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else HERE();
		} else if (is_atx(X_OUTER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGR_%s", fpga_wire2str(wire));
			else HERE();
		} else HERE();
	} else if (wire >= CLKPIN0 && wire <= CLKPIN7) {
		wstr = "CLKPIN";
		wnum = wire-CLKPIN0;

		if (is_aty(Y_OUTER_TOP, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_aty(Y_INNER_TOP, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"IOI_REGT_%s", fpga_wire2str(wire));
			else if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_TTERM_%s", fpga_wire2str(wire));
			else if (is_atx(X_CENTER_REGS_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGV_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_1)
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"IOI_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum >= 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_REGT_%s", fpga_wire2str(wire-4));
				else
					HERE();
			} else HERE();
		} else if (is_aty(Y_INNER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
				if (wnum >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire));
				} else HERE();
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_BTERM_%s", fpga_wire2str(wire));
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"REGV_BTERM_%s", fpga_wire2str(wire+4));
				} else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_1) {
				if (wnum < 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_BTERM_%s", fpga_wire2str(wire+4));
				else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire+4));
				} else HERE();
			} else HERE();
		} else if (is_aty(Y_OUTER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_OUTER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGL_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_INNER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_LTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s", fpga_wire2str(wire));
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s", fpga_wire2str(wire));
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum >= 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s_EXT", fpga_wire2str(wire-4));
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s", fpga_wire2str(wire-4));
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s", fpga_wire2str(wire-4));
				} else HERE();
			} else HERE();
		} else if (is_atx(X_INNER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_RTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum >= 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s", fpga_wire2str(wire-4));
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum >= 6) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s", fpga_wire2str(wire-4));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum < 4)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s%s", fpga_wire2str(wire),
					  wnum < 8 ? "_EXT" : "");
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s", fpga_wire2str(wire));
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s", fpga_wire2str(wire));
				} else HERE();
			} else HERE();
		} else if (is_atx(X_OUTER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGR_%s", fpga_wire2str(wire));
			else HERE();
		} else HERE();
	} else if ((wire >= DQSN0 && wire <= DQSN3)
		    || (wire >= DQSP0 && wire <= DQSP3)) {

		if (wire >= DQSN0 && wire <= DQSN3) {
			wchar = 'N';
			wnum = wire - DQSN0;
		} else if (wire >= DQSP0 && wire <= DQSP3) {
			wchar = 'P';
			wnum = wire - DQSP0;
		} else HERE();

		if (is_aty(Y_OUTER_TOP, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_aty(Y_INNER_TOP, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_REGT_%s", fpga_wire2str(wire));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_S");
				} else HERE();
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGT_TTERM_%s", fpga_wire2str(wire));
			else if (is_atx(X_CENTER_REGS_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGV_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_1)
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"IOI_TTERM_%s", fpga_wire2str(wire));
			else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum%8 >= 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_REGT_%s", fpga_wire2str(wire-2));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_S");
				} else HERE();
			} else HERE();
		} else if (is_aty(Y_TOP_OUTER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"TIOI_%s_OUT%c%s", 
				wnum%2 ? "INNER" : "UPPER", wchar,
				wnum%2 ? "_EXT" : "");
		} else if (is_aty(Y_TOP_INNER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"TIOI_INNER_OUT%c", wchar);
		} else if (is_aty(Y_BOT_INNER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"BIOI_INNER_OUT%c", wchar);
		} else if (is_aty(Y_BOT_OUTER_IO, model, y)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]),
				"BIOI_%s_OUT%c%s", 
				wnum%2 ? "INNER" : "OUTER", wchar,
				wnum%2 ? "_EXT" : "");
		} else if (is_aty(Y_INNER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_LOGIC_COL, model, x)) {
				if (wnum >= 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_N");
				} else HERE();
			} else if (is_atx(X_CENTER_CMTPLL_COL, model, x)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_BTERM_%s", fpga_wire2str(wire));
			} else if (is_atx(X_CENTER_REGS_COL, model, x)) {
				if (wnum < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"REGV_BTERM_%s", fpga_wire2str(wire+2));
				} else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_1) {
				if (wnum < 2)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"IOI_BTERM_%s", fpga_wire2str(wire+2));
				else HERE();
			} else if (x == model->center_x + CENTER_X_PLUS_2) {
				if (wnum < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
						"BTERM_CLB_%s", fpga_wire2str(wire+2));
					if (is_atyx(YX_DEV_ILOGIC, model, dest_y, dest_x))
						strcat(buf[last_buf], "_N");
				} else HERE();
			} else HERE();
		} else if (is_aty(Y_OUTER_BOTTOM, model, y)) {
			if (is_atx(X_CENTER_CMTPLL_COL, model, x))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGB_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_OUTER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGL_%s", fpga_wire2str(wire));
			else HERE();
		} else if (is_atx(X_INNER_LEFT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_LTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum%8 < 4) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_TOP_%s", fpga_wire2str(wire));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum%8 < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_BOT_%s", fpga_wire2str(wire));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum >= 2)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_%s_EXT", fpga_wire2str(wire-2));
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum >= 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_BOT_%s", fpga_wire2str(wire-2));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum >= 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_LTERM_TOP_%s", fpga_wire2str(wire-2));
					if (is_atx(X_LEFT_IO_ROUTING_COL|X_LEFT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_E");
				} else HERE();
			} else HERE();
		} else if (is_atx(X_LEFT_IO_ROUTING_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]), "INT_OUT%c", wchar);
		} else if (is_atx(X_LEFT_IO_DEVS_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]), "LIOI_OUT%c", wchar);
		} else if (is_atx(X_RIGHT_IO_DEVS_COL, model, x)) {
			snprintf(buf[last_buf], sizeof(buf[last_buf]), "RIOI_OUT%c", wchar);
		} else if (is_atx(X_INNER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGH_RTERM_%s", fpga_wire2str(wire));
			} else if (y == model->center_y + CENTER_Y_PLUS_1) {
				if (wnum >= 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_TOP_%s", fpga_wire2str(wire-2));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y + CENTER_Y_PLUS_2) {
				if (wnum >= 3) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_BOT_%s", fpga_wire2str(wire-2));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_1
				   || y == model->center_y - CENTER_Y_MINUS_2) {
				if (wnum < 2)
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_%s%s", fpga_wire2str(wire),
					  wnum < 8 ? "_EXT" : "");
				else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_3) {
				if (wnum < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_BOT_%s", fpga_wire2str(wire));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else if (y == model->center_y - CENTER_Y_MINUS_4) {
				if (wnum < 2) {
					snprintf(buf[last_buf], sizeof(buf[last_buf]),
					  "IOI_RTERM_TOP_%s", fpga_wire2str(wire));
					if (is_atx(X_RIGHT_IO_DEVS_COL, model, dest_x))
						strcat(buf[last_buf], "_W");
				} else HERE();
			} else HERE();
		} else if (is_atx(X_OUTER_RIGHT, model, x)) {
			if (is_aty(Y_CHIP_HORIZ_REGS, model, y))
				snprintf(buf[last_buf], sizeof(buf[last_buf]),
					"REGR_%s", fpga_wire2str(wire));
			else HERE();
		} else HERE();
	} else HERE();
	return buf[last_buf];
}

const char* fpga_wire2str(enum extra_wires wire)
{
 	enum { NUM_BUFS = 8, BUF_SIZE = MAX_WIRENAME_LEN };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;
	int flags;

	switch (wire) {
		case GFAN0:		return "GFAN0";
		case GFAN1:		return "GFAN1";
		case CLK0:		return "CLK0";
		case CLK1:		return "CLK1";
		case SR0:		return "SR0";
		case SR1:		return "SR1";
		case GND_WIRE:		return "GND_WIRE";
		case VCC_WIRE:		return "VCC_WIRE";
		case FAN_B:		return "FAN_B";
		case LOGICIN20:		return "LOGICIN20";
		case LOGICIN21:		return "LOGICIN21";
		case LOGICIN44:		return "LOGICIN44";
		case LOGICIN52:		return "LOGICIN52";
		case LOGICIN_N21:	return "LOGICIN_N21";
		case LOGICIN_N28:	return "LOGICIN_N28";
		case LOGICIN_N52:	return "LOGICIN_N52";
		case LOGICIN_N60:	return "LOGICIN_N60";
		case LOGICIN_S20:	return "LOGICIN_S20";
		case LOGICIN_S36:	return "LOGICIN_S36";
		case LOGICIN_S44:	return "LOGICIN_S44";
		case LOGICIN_S62:	return "LOGICIN_S62";
		case IOCE:		return "IOCE";
		case IOCLK:		return "IOCLK";
		case PLLCE:		return "PLLCE";
		case PLLCLK:		return "PLLCLK";
		case CKPIN:		return "CKPIN";
		case CLK_FEEDBACK:	return "CLK_FEEDBACK";
		case CLK_INDIRECT:	return "CLK_INDIRECT";
		case CFB0:		return "CFB0";
		case CFB1:		return "CFB1";
		case CFB2:		return "CFB2";
		case CFB3:		return "CFB3";
		case CFB4:		return "CFB4";
		case CFB5:		return "CFB5";
		case CFB6:		return "CFB6";
		case CFB7:		return "CFB7";
		case CFB8:		return "CFB1_0";
		case CFB9:		return "CFB1_1";
		case CFB10:		return "CFB1_2";
		case CFB11:		return "CFB1_3";
		case CFB12:		return "CFB1_4";
		case CFB13:		return "CFB1_5";
		case CFB14:		return "CFB1_6";
		case CFB15:		return "CFB1_7";
		case DFB0:		return "DFB0";
		case DFB1:		return "DFB1";
		case DFB2:		return "DFB2";
		case DFB3:		return "DFB3";
		case DFB4:		return "DFB4";
		case DFB5:		return "DFB5";
		case DFB6:		return "DFB6";
		case DFB7:		return "DFB7";
		case CLKPIN0:		return "CLKPIN0";
		case CLKPIN1:		return "CLKPIN1";
		case CLKPIN2:		return "CLKPIN2";
		case CLKPIN3:		return "CLKPIN3";
		case CLKPIN4:		return "CLKPIN4";
		case CLKPIN5:		return "CLKPIN5";
		case CLKPIN6:		return "CLKPIN6";
		case CLKPIN7:		return "CLKPIN7";
		case DQSN0:		return "DQSN0";
		case DQSN1:		return "DQSN1";
		case DQSN2:		return "DQSN2";
		case DQSN3:		return "DQSN3";
		case DQSP0:		return "DQSP0";
		case DQSP1:		return "DQSP1";
		case DQSP2:		return "DQSP2";
		case DQSP3:		return "DQSP3";
		default: ;
	}

	last_buf = (last_buf+1)%NUM_BUFS;
	buf[last_buf][0] = 0;
	if (wire >= LOGICOUT_B0 && wire <= LOGICOUT_B0+LOGICOUT_HIGHEST)
		snprintf(buf[last_buf], sizeof(buf[0]), "LOGICOUT%i", wire-LOGICOUT_B0);
	else if (wire >= LOGICIN_B0 && wire <= LOGICIN_B0+LOGICIN_HIGHEST)
		snprintf(buf[last_buf], sizeof(buf[0]), "LOGICIN%i", wire-LOGICIN_B0);
	else if (wire >= GCLK0 && wire <= GCLK15)
		snprintf(buf[last_buf], sizeof(buf[0]), "GCLK%i", wire-GCLK0);
	else if (wire >= DW && wire <= DW_LAST) {
		char beg_end;

		wire -= DW;
		flags = wire & DIR_FLAGS;
		wire &= ~DIR_FLAGS;
		beg_end = (flags & DIR_BEG) ? 'B' : 'E';
		if (flags & DIR_S0 && wire%4 == 0)
			snprintf(buf[last_buf], sizeof(buf[0]),
				"%s%c_S0", wire_base(wire/4), beg_end);
		else if (flags & DIR_N3 && wire%4 == 3)
			snprintf(buf[last_buf], sizeof(buf[0]),
				"%s%c_N3", wire_base(wire/4), beg_end);
		else
			snprintf(buf[last_buf], sizeof(buf[0]),
				"%s%c%i", wire_base(wire/4), beg_end, wire%4);

	} else if (wire >= LW && wire <= LW_LAST) {
		wire -= LW;
		if ((wire&(~LD1)) >= LI_FIRST && (wire&(~LD1)) <= LI_LAST)
			snprintf(buf[last_buf], sizeof(buf[0]), "LOGICIN_B%i", fdev_logic_inbit(wire));
		else if ((wire&(~LD1)) >= LO_FIRST && (wire&(~LD1)) <= LO_LAST)
			snprintf(buf[last_buf], sizeof(buf[0]), "LOGICOUT%i", fdev_logic_outbit(wire));
		else HERE();
	} else if (wire >= BW && wire <= BW_LAST) {
		const char* bram_pref;
		int bram_w;

		wire -= BW;
		flags = wire & BW_FLAGS;
		bram_w = wire & ~BW_FLAGS;
		if (flags & B8_0)
			bram_pref = "RAMB8BWER_0_";
		else if (flags & B8_1)
			bram_pref = "RAMB8BWER_1_";
		else
			bram_pref = "RAMB16BWER_";

		if (bram_w >= BI_ADDRA0 && bram_w <= BI_ADDRA13) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sADDRA%i", bram_pref, bram_w-BI_ADDRA0);
		} else if (bram_w >= BI_ADDRB0 && bram_w <= BI_ADDRB13) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sADDRB%i", bram_pref, bram_w-BI_ADDRB0);
		} else if (bram_w >= BI_DIA0 && bram_w <= BI_DIA31) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDIA%i", bram_pref, bram_w-BI_DIA0);
		} else if (bram_w >= BI_DIB0 && bram_w <= BI_DIB31) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDIB%i", bram_pref, bram_w-BI_DIB0);
		} else if (bram_w >= BI_DIPA0 && bram_w <= BI_DIPA3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDIPA%i", bram_pref, bram_w-BI_DIPA0);
		} else if (bram_w >= BI_DIPB0 && bram_w <= BI_DIPB3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDIPB%i", bram_pref, bram_w-BI_DIPB0);

		} else if (bram_w >= BO_DOA0 && bram_w <= BO_DOA31) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDOA%i", bram_pref, bram_w-BO_DOA0);
		} else if (bram_w >= BO_DOB0 && bram_w <= BO_DOB31) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDOB%i", bram_pref, bram_w-BO_DOB0);
		} else if (bram_w >= BO_DOPA0 && bram_w <= BO_DOPA3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDOPA%i", bram_pref, bram_w-BO_DOPA0);
		} else if (bram_w >= BO_DOPB0 && bram_w <= BO_DOPB3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sDOPB%i", bram_pref, bram_w-BO_DOPB0);

		} else if (bram_w >= BI_WEA0 && bram_w <= BI_WEA3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sWEA%i", bram_pref, bram_w-BI_WEA0);
		} else if (bram_w >= BI_WEB0 && bram_w <= BI_WEB3) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sWEB%i", bram_pref, bram_w-BI_WEB0);
		} else if (bram_w == BI_REGCEA) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sREGCEA", bram_pref);
		} else if (bram_w == BI_REGCEB) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sREGCEB", bram_pref);
		} else if (bram_w == BI_ENA) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sENA", bram_pref);
		} else if (bram_w == BI_ENB) {
			snprintf(buf[last_buf], sizeof(buf[0]), "%sENB", bram_pref);
		} else HERE();
	} else if (wire >= MW && wire <= MW_LAST) {
		int macc_w = wire - MW;
		if (macc_w >= MI_A0 && macc_w <= MI_A17)
			snprintf(buf[last_buf], sizeof(buf[0]), "A%i_DSP48A1_SITE", macc_w-MI_A0);
		else if (macc_w >= MI_B0 && macc_w <= MI_B17)
			snprintf(buf[last_buf], sizeof(buf[0]), "B%i_DSP48A1_SITE", macc_w-MI_B0);
		else if (macc_w >= MI_C0 && macc_w <= MI_C47)
			snprintf(buf[last_buf], sizeof(buf[0]), "C%i_DSP48A1_SITE", macc_w-MI_C0);
		else if (macc_w >= MI_D0 && macc_w <= MI_D17)
			snprintf(buf[last_buf], sizeof(buf[0]), "D%i_DSP48A1_SITE", macc_w-MI_D0);
		else if (macc_w >= MI_OPMODE0 && macc_w <= MI_OPMODE7)
			snprintf(buf[last_buf], sizeof(buf[0]), "OPMODE%i_DSP48A1_SITE", macc_w-MI_OPMODE0);
		else if (macc_w >= MO_P0 && macc_w <= MO_P47)
			snprintf(buf[last_buf], sizeof(buf[0]), "P%i_DSP48A1_SITE", macc_w-MO_P0);
		else if (macc_w >= MO_M0 && macc_w <= MO_M35)
			snprintf(buf[last_buf], sizeof(buf[0]), "M%i_DSP48A1_SITE", macc_w-MO_M0);
		else if (macc_w == MI_CEA)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEA_DSP48A1_SITE");
		else if (macc_w == MI_CEB)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEB_DSP48A1_SITE");
		else if (macc_w == MI_CEC)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEC_DSP48A1_SITE");
		else if (macc_w == MI_CED)
			snprintf(buf[last_buf], sizeof(buf[0]), "CED_DSP48A1_SITE");
		else if (macc_w == MI_CEM)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEM_DSP48A1_SITE");
		else if (macc_w == MI_CEP)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEP_DSP48A1_SITE");
		else if (macc_w == MI_CE_OPMODE)
			snprintf(buf[last_buf], sizeof(buf[0]), "CEOPMODE_DSP48A1_SITE");
		else if (macc_w == MI_CE_CARRYIN)
			snprintf(buf[last_buf], sizeof(buf[0]), "CECARRYIN_DSP48A1_SITE");
		else if (macc_w == MO_CARRYOUT)
			snprintf(buf[last_buf], sizeof(buf[0]), "CARRYOUTF_DSP48A1_SITE");
		else HERE();
	} else fprintf(stderr, "#E %s:%i unsupported wire %i\n", __FILE__, __LINE__, wire);

	return buf[last_buf];
}

str16_t fpga_wire2str_i(struct fpga_model* model, enum extra_wires wire)
{
	return strarray_find(&model->str, fpga_wire2str(wire));
}

enum extra_wires fpga_str2wire(const char* str)
{
	const char* _str;
	enum wire_type wtype;
	int len, num, flags;

	_str = str;
	if (!strncmp(_str, "INT_IOI_", 8))
		_str = &_str[8];

	if (!strncmp(_str, "GCLK", 4)) {
		len = strlen(_str);
		if (!strcmp(&_str[len-4], "_BRK"))
			len -= 4;
		if (len > 4) {
			num = to_i(&_str[4], len-4);
			if (num >= 0 && num <= 15)
				return GCLK0 + num;
		}
	}
	if (!strncmp(_str, "LOGICIN_B", 9)) {
		len = strlen(&_str[9]);
		if (len) {
			switch (to_i(&_str[9], len)) {
				case X_A1: return LW + LI_A1;
				case X_A2: return LW + LI_A2;
				case X_A3: return LW + LI_A3;
				case X_A4: return LW + LI_A4;
				case X_A5: return LW + LI_A5;
				case X_A6: return LW + LI_A6;
				case X_AX: return LW + LI_AX;
				case X_B1: return LW + LI_B1;
				case X_B2: return LW + LI_B2;
				case X_B3: return LW + LI_B3;
				case X_B4: return LW + LI_B4;
				case X_B5: return LW + LI_B5;
				case X_B6: return LW + LI_B6;
				case X_BX: return LW + LI_BX;
				case X_C1: return LW + LI_C1;
				case X_C2: return LW + LI_C2;
				case X_C3: return LW + LI_C3;
				case X_C4: return LW + LI_C4;
				case X_C5: return LW + LI_C5;
				case X_C6: return LW + LI_C6;
				case X_CE: return LW + LI_CE;
				case X_CX: return LW + LI_CX;
				case X_D1: return LW + LI_D1;
				case X_D2: return LW + LI_D2;
				case X_D3: return LW + LI_D3;
				case X_D4: return LW + LI_D4;
				case X_D5: return LW + LI_D5;
				case X_D6: return LW + LI_D6;
				case X_DX: return LW + LI_DX;
				case M_A1: return LW + (LI_A1|LD1);
				case M_A2: return LW + (LI_A2|LD1);
				case M_A3: return LW + (LI_A3|LD1);
				case M_A4: return LW + (LI_A4|LD1);
				case M_A5: return LW + (LI_A5|LD1);
				case M_A6: return LW + (LI_A6|LD1);
				case M_AX: return LW + (LI_AX|LD1);
				case M_AI: return LW + (LI_AI|LD1);
				case M_B1: return LW + (LI_B1|LD1);
				case M_B2: return LW + (LI_B2|LD1);
				case M_B3: return LW + (LI_B3|LD1);
				case M_B4: return LW + (LI_B4|LD1);
				case M_B5: return LW + (LI_B5|LD1);
				case M_B6: return LW + (LI_B6|LD1);
				case M_BX: return LW + (LI_BX|LD1);
				case M_BI: return LW + (LI_BI|LD1);
				case M_C1: return LW + (LI_C1|LD1);
				case M_C2: return LW + (LI_C2|LD1);
				case M_C3: return LW + (LI_C3|LD1);
				case M_C4: return LW + (LI_C4|LD1);
				case M_C5: return LW + (LI_C5|LD1);
				case M_C6: return LW + (LI_C6|LD1);
				case M_CE: return LW + (LI_CE|LD1);
				case M_CX: return LW + (LI_CX|LD1);
				case M_CI: return LW + (LI_CI|LD1);
				case M_D1: return LW + (LI_D1|LD1);
				case M_D2: return LW + (LI_D2|LD1);
				case M_D3: return LW + (LI_D3|LD1);
				case M_D4: return LW + (LI_D4|LD1);
				case M_D5: return LW + (LI_D5|LD1);
				case M_D6: return LW + (LI_D6|LD1);
				case M_DX: return LW + (LI_DX|LD1);
				case M_DI: return LW + (LI_DI|LD1);
				case M_WE: return LW + (LI_WE|LD1);
			}
		}
	}
	if (!strncmp(_str, "LOGICOUT", 8)) {
		len = strlen(&_str[8]);
		if (len) {
			switch (to_i(&_str[8], len)) {
				case M_A:	return LW + (LO_A|LD1);
				case M_AMUX:	return LW + (LO_AMUX|LD1);
				case M_AQ:	return LW + (LO_AQ|LD1);
				case M_B:	return LW + (LO_B|LD1);
				case M_BMUX:	return LW + (LO_BMUX|LD1);
				case M_BQ:	return LW + (LO_BQ|LD1);
				case M_C:	return LW + (LO_C|LD1);
				case M_CMUX:	return LW + (LO_CMUX|LD1);
				case M_CQ:	return LW + (LO_CQ|LD1);
				case M_D:	return LW + (LO_D|LD1);
				case M_DMUX:	return LW + (LO_DMUX|LD1);
				case M_DQ:	return LW + (LO_DQ|LD1);
				case X_A:	return LW + LO_A;
				case X_AMUX:	return LW + LO_AMUX;
				case X_AQ:	return LW + LO_AQ;
				case X_B:	return LW + LO_B;
				case X_BMUX:	return LW + LO_BMUX;
				case X_BQ:	return LW + LO_BQ;
				case X_C:	return LW + LO_C;
				case X_CMUX:	return LW + LO_CMUX;
				case X_CQ:	return LW + LO_CQ;
				case X_D:	return LW + LO_D;
				case X_DMUX:	return LW + LO_DMUX;
				case X_DQ:	return LW + LO_DQ;
			}
		}
	}
	if ((wtype = base2wire(_str))) {
		flags = 0;
		if (_str[3] == 'B')
			flags |= DIR_BEG;
		if (_str[3] != 'B' && _str[3] != 'E')
			HERE();
		else {
			if (!strcmp(&_str[4], "_S0")) {
				num = 0;
				flags |= DIR_S0;
			} else if (!strcmp(&_str[4], "_N3")) {
				num = 3;
				flags |= DIR_N3;
			} else switch (_str[4]) {
				case '0': num = 0; break;
				case '1': num = 1; break;
				case '2': num = 2; break;
				case '3': num = 3; break;
				default:
					HERE();
					num = -1;
					break;
			}
			if (num != -1)
				return DW + ((wtype*4 + num)|flags);
		}
	}
	if (!strcmp(_str, "GFAN0")) return GFAN0;
	if (!strcmp(_str, "GFAN1")) return GFAN1;
	if (!strcmp(_str, "CLK0")) return CLK0;
	if (!strcmp(_str, "CLK1")) return CLK1;
	if (!strcmp(_str, "SR0")) return SR0;
	if (!strcmp(_str, "SR1")) return SR1;
	if (!strcmp(_str, "GND_WIRE")) return GND_WIRE;
	if (!strcmp(_str, "VCC_WIRE")) return VCC_WIRE;
	if (!strcmp(_str, "FAN_B")) return FAN_B;
	if (!strncmp(_str, "LOGICIN", 7)) {
		if (!strcmp(&_str[7], "20")) return LOGICIN20;
		if (!strcmp(&_str[7], "21")) return LOGICIN21;
		if (!strcmp(&_str[7], "44")) return LOGICIN44;
		if (!strcmp(&_str[7], "52")) return LOGICIN52;
		if (!strcmp(&_str[7], "_N21")) return LOGICIN_N21;
		if (!strcmp(&_str[7], "_N28")) return LOGICIN_N28;
		if (!strcmp(&_str[7], "_N52")) return LOGICIN_N52;
		if (!strcmp(&_str[7], "_N60")) return LOGICIN_N60;
		if (!strcmp(&_str[7], "_S20")) return LOGICIN_S20;
		if (!strcmp(&_str[7], "_S36")) return LOGICIN_S36;
		if (!strcmp(&_str[7], "_S44")) return LOGICIN_S44;
		if (!strcmp(&_str[7], "_S62")) return LOGICIN_S62;
	}
	HERE();
	return NO_WIRE;
}

int fdev_logic_inbit(pinw_idx_t idx)
{
	if (idx & LD1) {
		idx &= ~LD1;
		if (idx >= LI_A1 && idx <= LI_C6)
			return M_A1 + (idx/6)*8+idx%6;
		if (idx >= LI_D1 && idx <= LI_D6)
			return M_D1 + (idx-LI_D1);
		switch (idx) {
			case LI_AX: return M_AX;
			case LI_BX: return M_BX;
			case LI_CE: return M_CE;
			case LI_CX: return M_CX;
			case LI_DX: return M_DX;
			case LI_AI: return M_AI;
			case LI_BI: return M_BI;
			case LI_CI: return M_CI;
			case LI_DI: return M_DI;
			case LI_WE: return M_WE;
		}
		HERE();
		return -1;
	}
	if (idx >= LI_A1 && idx <= LI_C6)
		return X_A1 + (idx/6)*7+idx%6;
	else if (idx >= LI_D1 && idx <= LI_D6)
		return X_D1 + (idx-LI_D1);
	else switch (idx) {
		case LI_AX: return X_AX;
		case LI_BX: return X_BX;
		case LI_CE: return X_CE;
		case LI_CX: return X_CX;
		case LI_DX: return X_DX;
	}
	HERE();
	return -1;
}

int fdev_logic_outbit(pinw_idx_t idx)
{
	if (idx & LD1) {
		idx &= ~LD1;
		switch (idx) {
			case LO_A:	return M_A;
			case LO_AMUX:	return M_AMUX;
			case LO_AQ:	return M_AQ;
			case LO_B:	return M_B;
			case LO_BMUX:	return M_BMUX;
			case LO_BQ:	return M_BQ;
			case LO_C:	return M_C;
			case LO_CMUX:	return M_CMUX;
			case LO_CQ:	return M_CQ;
			case LO_D:	return M_D;
			case LO_DMUX:	return M_DMUX;
			case LO_DQ:	return M_DQ;
		}
		HERE();
		return -1;
	}
	switch (idx) {
		case LO_A:	return X_A;
		case LO_AMUX:	return X_AMUX;
		case LO_AQ:	return X_AQ;
		case LO_B:	return X_B;
		case LO_BMUX:	return X_BMUX;
		case LO_BQ:	return X_BQ;
		case LO_C:	return X_C;
		case LO_CMUX:	return X_CMUX;
		case LO_CQ:	return X_CQ;
		case LO_D:	return X_D;
		case LO_DMUX:	return X_DMUX;
		case LO_DQ:	return X_DQ;
	}
	HERE();
	return -1;
}

void fdev_bram_inbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_62)
{
	const int ramb16_map[4*63] = {
		// tile #3 (topmost)
		 /* 0*/ 0, BW+BI_DIB25, 0, BW+BI_DIB24, 
		 /* 4*/ BW+BI_DIB22, 0, 0, 0,
		 /* 8*/ BW+BI_DIB31, BW+BI_DIB29, BW+BI_WEA0, 0, 
		 /*12*/ 0, 0, 0, 0, 
		 /*16*/ 0, 0, 0, BW+BI_DIB26, 
		 /*20*/ 0, 0, 0, BW+BI_DIB19, 
		 /*24*/ BW+BI_ADDRB13, BW+BI_REGCEB, BW+BI_DIPB2, BW+BI_DIB27, 
		 /*28*/ 0, 0, BW+BI_WEA1, 0,
		 /*32*/ 0, 0, 0, 0,
		 /*36*/ 0, 0, BW+BI_DIB18, BW+BI_DIB30,
		 /*40*/ 0, BW+BI_DIPB3, 0, 0,
		 /*44*/ BW+BI_WEA3, 0, 0, BW+BI_DIB17, 
		 /*48*/ 0, 0, 0, 0,
		 /*52*/ 0, 0, BW+BI_DIB20, BW+BI_DIB23,
		 /*56*/ 0, BW+BI_DIB21, BW+BI_WEA2, BW+BI_DIB28, 
		 /*60*/ 0, 0, BW+BI_DIB16,

		// tile #2
		 /* 0*/ 0, 0, 0, BW+BI_DIA27,
		 /* 4*/ BW+BI_ADDRB10, BW+BI_ADDRB2, 0, BW+BI_DIA17,
		 /* 8*/ 0, BW+BI_DIA31, BW+BI_ADDRB12, 0,
		 /*12*/ BW+BI_ADDRB3, 0, 0, BW+BI_DIA16,
		 /*16*/ BW+BI_DIA19, BW+BI_ADDRB7, 0, BW+BI_DIA28,
		 /*20*/ BW+BI_DIA20, 0, 0, BW+BI_DIA22,
		 /*24*/ BW+BI_ADDRB0, BW+BI_ADDRB9, BW+BI_DIA24, BW+BI_DIA30,
		 /*28*/ BW+BI_DIA29, 0, BW+BI_DIA25, 0,
		 /*32*/ BW+BI_REGCEA, 0, 0, 0,
		 /*36*/ BW+BI_DIA18, 0, BW+BI_DIA21, 0,
		 /*40*/ 0, BW+BI_DIA26, BW+BI_ADDRB1, 0,
		 /*44*/ BW+BI_DIPA2, BW+BI_ADDRB6, 0, BW+BI_ADDRB5,
		 /*48*/ BW+BI_DIA23, 0, 0, 0,
		 /*52*/ 0, 0, BW+BI_ENB, BW+BI_DIPA3,
		 /*56*/ 0, BW+BI_ADDRB8, BW+BI_ADDRB11, 0,
		 /*60*/ 0, 0, BW+BI_ADDRB4,

		// tile #1
		 /* 0*/ 0, BW+BI_ADDRA5, BW+BI_DIB13, BW+BI_DIB8,
		 /* 4*/ BW+BI_DIB5, 0, 0, 0,
		 /* 8*/ BW+BI_ADDRA11, BW+BI_ADDRA8, BW+BI_ADDRA3, 0,
		 /*12*/ BW+BI_DIB0, 0, BW+BI_DIB14, 0,
		 /*16*/ 0, 0, 0, BW+BI_DIB9,
		 /*20*/ 0, 0, 0, BW+BI_DIB3,
		 /*24*/ 0, 0, BW+BI_DIB7, BW+BI_ADDRA6,
		 /*28*/ BW+BI_DIB10, BW+BI_DIB11, BW+BI_DIPB1, BW+BI_ADDRA12,
		 /*32*/ BW+BI_ADDRA7, 0, BW+BI_ADDRA0, 0,
		 /*36*/ 0, BW+BI_DIB15, BW+BI_DIB2, BW+BI_ADDRA10,
		 /*40*/ 0, BW+BI_DIPB0, 0, 0,
		 /*44*/ BW+BI_ADDRA2, 0, 0, BW+BI_DIB1,
		 /*48*/ BW+BI_ENA, 0, 0, 0,
		 /*52*/ BW+BI_ADDRA9, 0, BW+BI_ADDRA1, BW+BI_DIB6,
		 /*56*/ 0, BW+BI_DIB4, BW+BI_ADDRA4, BW+BI_DIB12,
		 /*60*/ 0, 0, 0,

		// tile #0 (bottommost)
		 /* 0*/ 0, 0, 0, BW+BI_DIA11,
		 /* 4*/ BW+BI_WEB0, 0, 0, BW+BI_DIA1,
		 /* 8*/ 0, BW+BI_DIA15, BW+BI_DIA10, 0,
		 /*12*/ 0, 0, 0, BW+BI_DIA0,
		 /*16*/ BW+BI_DIA3, 0, 0, BW+BI_DIA12,
		 /*20*/ BW+BI_DIA4, 0, 0, BW+BI_DIA6,
		 /*24*/ 0, BW+BI_WEB1, BW+BI_DIA8, BW+BI_DIA13,
		 /*28*/ 0, 0, BW+BI_DIA9, BW+BI_ADDRA13,
		 /*32*/ BW+BI_DIA14, 0, BW+BI_WEB3, 0,
		 /*36*/ BW+BI_DIA2, 0, BW+BI_DIA5, 0,
		 /*40*/ 0, 0, 0, 0,
		 /*44*/ 0, 0, 0, 0,
		 /*48*/ BW+BI_DIA7, 0, 0, 0,
		 /*52*/ 0, 0, BW+BI_WEB2, BW+BI_DIPA1,
		 /*56*/ 0, BW+BI_DIPA0, 0, 0,
		 /*60*/ 0, 0, 0 };

	const int ramb8_map[4*63] = {
		// tile #3 (topmost)
		 /* 0*/ 0, BW+(B8_1|BI_DIB9), 0, BW+(B8_1|BI_DIB8),
		 /* 4*/ BW+(B8_1|BI_DIB6), BW+(B8_1|BI_ADDRB4), 0, BW+(B8_1|BI_ADDRB2),
		 /* 8*/ BW+(B8_1|BI_DIB15), BW+(B8_1|BI_DIB13), BW+(B8_0|BI_WEA0), 0,
		 /*12*/ BW+(B8_1|BI_ADDRB6), 0, 0, BW+(B8_1|BI_ADDRB1),
		 /*16*/ BW+(B8_1|BI_ADDRB8), BW+(B8_1|BI_ADDRB10), 0, BW+(B8_1|BI_DIB10),
		 /*20*/ BW+(B8_1|BI_ADDRB7), 0, 0, BW+(B8_1|BI_DIB3),
		 /*24*/ BW+(B8_1|BI_ADDRB0), BW+(B8_0|BI_REGCEB), BW+(B8_1|BI_DIPB0), BW+(B8_1|BI_DIB11),
		 /*28*/ 0, 0, BW+(B8_0|BI_WEA1), 0,
		 /*32*/ 0, 0, BW+(B8_1|BI_ADDRB11), 0,
		 /*36*/ BW+(B8_1|BI_ADDRB5), 0, BW+(B8_1|BI_DIB2), BW+(B8_1|BI_DIB14),
		 /*40*/ 0, BW+(B8_1|BI_DIPB1), BW+(B8_1|BI_ADDRB3), 0,
		 /*44*/ BW+(B8_1|BI_WEA1), BW+(B8_1|BI_ADDRB9), 0, BW+(B8_1|BI_DIB1),
		 /*48*/ BW+(B8_1|BI_ADDRB12), 0, 0, 0,
		 /*52*/ 0, 0, BW+(B8_1|BI_DIB4), BW+(B8_1|BI_DIB7),
		 /*56*/ 0, BW+(B8_1|BI_DIB5), BW+(B8_1|BI_WEA0), BW+(B8_1|BI_DIB12),
		 /*60*/ 0, 0, BW+(B8_1|BI_DIB0),

		// tile #2
		 /* 0*/ 0, 0, 0, BW+(B8_1|BI_DIA11),
		 /* 4*/ BW+(B8_0|BI_ADDRB10), BW+(B8_0|BI_ADDRB2), 0, BW+(B8_1|BI_DIA1),
		 /* 8*/ 0, BW+(B8_1|BI_DIA15), BW+(B8_0|BI_ADDRB12), 0,
		 /*12*/ BW+(B8_0|BI_ADDRB3), 0, 0, BW+(B8_1|BI_DIA0),
		 /*16*/ BW+(B8_1|BI_DIA3), BW+(B8_0|BI_ADDRB7), 0, BW+(B8_1|BI_DIA12),
		 /*20*/ BW+(B8_1|BI_DIA4), 0, 0, BW+(B8_1|BI_DIA6),
		 /*24*/ BW+(B8_0|BI_ADDRB0), BW+(B8_0|BI_ADDRB9), BW+(B8_1|BI_DIA8), BW+(B8_1|BI_DIA14),
		 /*28*/ BW+(B8_1|BI_DIA13), BW+(B8_1|BI_REGCEA), BW+(B8_1|BI_DIA9), 0,
		 /*32*/ BW+(B8_0|BI_REGCEA), 0, BW+(B8_1|BI_ENB), 0,
		 /*36*/ BW+(B8_1|BI_DIA2), 0, BW+(B8_1|BI_DIA5), 0,
		 /*40*/ 0, BW+(B8_1|BI_DIA10), BW+(B8_0|BI_ADDRB1), 0,
		 /*44*/ BW+(B8_1|BI_DIPA0), BW+(B8_0|BI_ADDRB6), 0, BW+(B8_0|BI_ADDRB5),
		 /*48*/ BW+(B8_1|BI_DIA7), 0, 0, 0,
		 /*52*/ 0, 0, BW+(B8_0|BI_ENB), BW+(B8_1|BI_DIPA1),
		 /*56*/ 0, BW+(B8_0|BI_ADDRB8), BW+(B8_0|BI_ADDRB11), BW+(B8_1|BI_REGCEB),
		 /*60*/ 0, 0, BW+(B8_0|BI_ADDRB4),

		// tile #1
		 /* 0*/ 0, BW+(B8_0|BI_ADDRA5), BW+(B8_0|BI_DIB13), BW+(B8_0|BI_DIB8),
		 /* 4*/ BW+(B8_0|BI_DIB5), 0, 0, 0,
		 /* 8*/ BW+(B8_0|BI_ADDRA11), BW+(B8_0|BI_ADDRA8), BW+(B8_0|BI_ADDRA3), 0,
		 /*12*/ BW+(B8_0|BI_DIB0), 0, BW+(B8_0|BI_DIB14), 0,
		 /*16*/ 0, 0, 0, BW+(B8_0|BI_DIB9),
		 /*20*/ 0, 0, 0, BW+(B8_0|BI_DIB3),
		 /*24*/ 0, BW+(B8_1|BI_ENA), BW+(B8_0|BI_DIB7), BW+(B8_0|BI_ADDRA6),
		 /*28*/ BW+(B8_0|BI_DIB10), BW+(B8_0|BI_DIB11), BW+(B8_0|BI_DIPB1), BW+(B8_0|BI_ADDRA12),
		 /*32*/ BW+(B8_0|BI_ADDRA7), 0, BW+(B8_0|BI_ADDRA0), 0,
		 /*36*/ 0, BW+(B8_0|BI_DIB15), BW+(B8_0|BI_DIB2), BW+(B8_0|BI_ADDRA10),
		 /*40*/ 0, BW+(B8_0|BI_DIPB0), 0, 0,
		 /*44*/ BW+(B8_0|BI_ADDRA2), 0, 0, BW+(B8_0|BI_DIB1),
		 /*48*/ BW+(B8_0|BI_ENA), 0, 0, 0,
		 /*52*/ BW+(B8_0|BI_ADDRA9), 0, BW+(B8_0|BI_ADDRA1), BW+(B8_0|BI_DIB6),
		 /*56*/ 0, BW+(B8_0|BI_DIB4), BW+(B8_0|BI_ADDRA4), BW+(B8_0|BI_DIB12),
		 /*60*/ 0, 0, 0,

		// tile #0 (bottommost)
		 /* 0*/ 0, BW+(B8_1|BI_ADDRA3), BW+(B8_1|BI_ADDRA8), BW+(B8_0|BI_DIA11),
		 /* 4*/ BW+(B8_0|BI_WEB0), 0, 0, BW+(B8_0|BI_DIA1),
		 /* 8*/ BW+(B8_1|BI_ADDRA12), BW+(B8_0|BI_DIA15), BW+(B8_0|BI_DIA10), 0,
		 /*12*/ 0, 0, BW+(B8_1|BI_ADDRA9), BW+(B8_0|BI_DIA0),
		 /*16*/ BW+(B8_0|BI_DIA3), 0, 0, BW+(B8_0|BI_DIA12),
		 /*20*/ BW+(B8_0|BI_DIA4), 0, 0, BW+(B8_0|BI_DIA6),
		 /*24*/ 0, BW+(B8_0|BI_WEB1), BW+(B8_0|BI_DIA8), BW+(B8_0|BI_DIA13),
		 /*28*/ BW+(B8_1|BI_ADDRA4), BW+(B8_1|BI_ADDRA5), BW+(B8_0|BI_DIA9), BW+(B8_1|BI_ADDRA0),
		 /*32*/ BW+(B8_0|BI_DIA14), 0, BW+(B8_1|BI_WEB1), 0,
		 /*36*/ BW+(B8_0|BI_DIA2), BW+(B8_1|BI_ADDRA11), BW+(B8_0|BI_DIA5), BW+(B8_1|BI_ADDRA10),
		 /*40*/ 0, BW+(B8_1|BI_ADDRA1), 0, 0,
		 /*44*/ 0, 0, 0, 0,
		 /*48*/ BW+(B8_0|BI_DIA7), 0, 0, 0,
		 /*52*/ BW+(B8_1|BI_ADDRA7), 0, BW+(B8_1|BI_WEB0), BW+(B8_0|BI_DIPA1),
		 /*56*/ 0, BW+(B8_0|BI_DIPA0), BW+(B8_1|BI_ADDRA2), BW+(B8_1|BI_ADDRA6),
		 /*60*/ 0, 0, 0 };

	const int *search_map;
	int i;

	if (wire < BW || wire > BW_LAST) {
		HERE();
		*tile0_to_3 = -1;
		return;
	}
	search_map = ((wire-BW) & BW_FLAGS) ? ramb8_map : ramb16_map;
	for (i = 0; i < 4*63; i++) {
		if (search_map[i] == wire) {
			*tile0_to_3 = 3-(i/63);
			*wire0_to_62 = i%63;
			return;
		}
	}
	fprintf(stderr, "#E %s:%i unknown wire %i\n",
		__FILE__, __LINE__, wire);
	*tile0_to_3 = -1;
}

void fdev_bram_outbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_23)
{
	const int ramb16_map[4*24] = {
		// tile #3 (topmost)
		 /* 0*/ 0, BW+BO_DOPB3, 0, BW+BO_DOB27,
		 /* 4*/ BW+BO_DOA29, BW+BO_DOA25, BW+BO_DOB29, BW+BO_DOA24,
		 /* 8*/ BW+BO_DOPA3, BW+BO_DOB31, BW+BO_DOB25, BW+BO_DOA31,
		 /*12*/ BW+BO_DOB24, 0, 0, BW+BO_DOA27,
		 /*16*/ BW+BO_DOA30, BW+BO_DOA26, BW+BO_DOA28, 0,
		 /*20*/ BW+BO_DOB28, BW+BO_DOB30, BW+BO_DOB26, 0,

		// tile #2
		 /* 0*/ BW+BO_DOA18, BW+BO_DOB20, BW+BO_DOA16, BW+BO_DOPB2,
		 /* 4*/ BW+BO_DOA23, BW+BO_DOB18, BW+BO_DOB22, 0,
		 /* 8*/ BW+BO_DOA20, 0, BW+BO_DOA19, 0,
		 /*12*/ BW+BO_DOB17, BW+BO_DOA22, BW+BO_DOA17, 0,
		 /*16*/ BW+BO_DOB23, BW+BO_DOB19, BW+BO_DOA21, BW+BO_DOB16,
		 /*20*/ BW+BO_DOB21, 0, BW+BO_DOPA2, 0,

		// tile #1
		 /* 0*/ BW+BO_DOB8, BW+BO_DOPA1, 0, BW+BO_DOA11,
		 /* 4*/ BW+BO_DOB13, BW+BO_DOA9, BW+BO_DOA13, 0,
		 /* 8*/ BW+BO_DOB11, BW+BO_DOB15, BW+BO_DOB9, BW+BO_DOA15,
		 /*12*/ BW+BO_DOA8, BW+BO_DOB12, 0, 0,
		 /*16*/ BW+BO_DOA14, BW+BO_DOA10, BW+BO_DOPB1, 0,
		 /*20*/ BW+BO_DOA12, BW+BO_DOB14, BW+BO_DOB10, 0,

		// tile #0 (bottommost)
		 /* 0*/ BW+BO_DOA2, BW+BO_DOB4, BW+BO_DOA0, BW+BO_DOPB0,
		 /* 4*/ BW+BO_DOB6, BW+BO_DOB2, BW+BO_DOA6, BW+BO_DOA1,
		 /* 8*/ BW+BO_DOA4, 0, BW+BO_DOA3, 0,
		 /*12*/ BW+BO_DOB1, BW+BO_DOB5, BW+BO_DOB0, BW+BO_DOPA0,
		 /*16*/ BW+BO_DOA7, BW+BO_DOB3, BW+BO_DOA5, 0,
		 /*20*/ 0, BW+BO_DOB7, 0, 0
		};
	const int ramb8_map[4*24] = {
		// tile #3 (topmost)
		 /* 0*/ 0, BW+(BO_DOPB1|B8_1), 0, BW+(BO_DOB11|B8_1),
		 /* 4*/ BW+(BO_DOA13|B8_1), BW+(BO_DOA9|B8_1), BW+(BO_DOB13|B8_1), BW+(BO_DOA8|B8_1),
		 /* 8*/ BW+(BO_DOPA1|B8_1), BW+(BO_DOB15|B8_1), BW+(BO_DOB9|B8_1), BW+(BO_DOA15|B8_1),
		 /*12*/ BW+(BO_DOB8|B8_1), 0, 0, BW+(BO_DOA11|B8_1),
		 /*16*/ BW+(BO_DOA14|B8_1), BW+(BO_DOA10|B8_1), BW+(BO_DOA12|B8_1), 0,
		 /*20*/ BW+(BO_DOB12|B8_1), BW+(BO_DOB14|B8_1), BW+(BO_DOB10|B8_1), 0,

		// tile #2
		 /* 0*/ BW+(BO_DOA2|B8_1), BW+(BO_DOB4|B8_1), BW+(BO_DOA0|B8_1), BW+(BO_DOPB0|B8_1),
		 /* 4*/ BW+(BO_DOA7|B8_1), BW+(BO_DOB2|B8_1), BW+(BO_DOB6|B8_1), 0,
		 /* 8*/ BW+(BO_DOA4|B8_1), 0, BW+(BO_DOA3|B8_1), 0,
		 /*12*/ BW+(BO_DOB1|B8_1), BW+(BO_DOA6|B8_1), BW+(BO_DOA1|B8_1), 0,
		 /*16*/ BW+(BO_DOB7|B8_1), BW+(BO_DOB3|B8_1), BW+(BO_DOA5|B8_1), BW+(BO_DOB0|B8_1),
		 /*20*/ BW+(BO_DOB5|B8_1), 0, BW+(BO_DOPA0|B8_1), 0,

		// tile #1
		 /* 0*/ BW+(BO_DOB8|B8_0), BW+(BO_DOPA1|B8_0), 0, BW+(BO_DOA11|B8_0),
		 /* 4*/ BW+(BO_DOB13|B8_0), BW+(BO_DOA9|B8_0), BW+(BO_DOA13|B8_0), 0,
		 /* 8*/ BW+(BO_DOB11|B8_0), BW+(BO_DOB15|B8_0), BW+(BO_DOB9|B8_0), BW+(BO_DOA15|B8_0),
		 /*12*/ BW+(BO_DOA8|B8_0), BW+(BO_DOB12|B8_0), 0, 0,
		 /*16*/ BW+(BO_DOA14|B8_0), BW+(BO_DOA10|B8_0), BW+(BO_DOPB1|B8_0), 0,
		 /*20*/ BW+(BO_DOA12|B8_0), BW+(BO_DOB14|B8_0), BW+(BO_DOB10|B8_0), 0,

		// tile #0 (bottommost)
		 /* 0*/ BW+(BO_DOA2|B8_0), BW+(BO_DOB4|B8_0), BW+(BO_DOA0|B8_0), BW+(BO_DOPB0|B8_0),
		 /* 4*/ BW+(BO_DOB6|B8_0), BW+(BO_DOB2|B8_0), BW+(BO_DOA6|B8_0), BW+(BO_DOA1|B8_0),
		 /* 8*/ BW+(BO_DOA4|B8_0), 0, BW+(BO_DOA3|B8_0), 0,
		 /*12*/ BW+(BO_DOB1|B8_0), BW+(BO_DOB5|B8_0), BW+(BO_DOB0|B8_0), BW+(BO_DOPA0|B8_0),
		 /*16*/ BW+(BO_DOA7|B8_0), BW+(BO_DOB3|B8_0), BW+(BO_DOA5|B8_0), 0,
		 /*20*/ 0, BW+(BO_DOB7|B8_0), 0, 0
		};

	const int *search_map;
	int i;

	if (wire < BW || wire > BW_LAST) {
		HERE();
		*tile0_to_3 = -1;
		return;
	}
	search_map = ((wire-BW) & BW_FLAGS) ? ramb8_map : ramb16_map;
	for (i = 0; i < 4*24; i++) {
		if (search_map[i] == wire) {
			*tile0_to_3 = 3-(i/24);
			*wire0_to_23 = i%24;
			return;
		}
	}
	fprintf(stderr, "#E %s:%i unknown wire %i\n",
		__FILE__, __LINE__, wire);
	*tile0_to_3 = -1;
}

int fdev_is_bram8_inwire(int bi_wire)
{
	if (bi_wire == BI_ADDRA13 || bi_wire == BI_ADDRB13)
		return 0;
	if (bi_wire == BI_WEA2 || bi_wire == BI_WEA3
	    || bi_wire == BI_WEB2 || bi_wire == BI_WEB3)
		return 0;
	if (bi_wire == BI_DIPA2 || bi_wire == BI_DIPA3
	    || bi_wire == BI_DIPB2 || bi_wire == BI_DIPB3)
		return 0;
	if ((bi_wire >= BI_DIA16 && bi_wire <= BI_DIA31)
	    || (bi_wire >= BI_DIB16 && bi_wire <= BI_DIB31))
		return 0;
	return 1;
}

int fdev_is_bram8_outwire(int bo_wire)
{
	if (bo_wire == BO_DOPA2 || bo_wire == BO_DOPA3
	    || bo_wire == BO_DOPB2 || bo_wire == BO_DOPB3)
		return 0;
	if ((bo_wire >= BO_DOA16 && bo_wire <= BO_DOA31)
	    || (bo_wire >= BO_DOB16 && bo_wire <= BO_DOB31))
		return 0;
	return 1;
}

void fdev_macc_inbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_62)
{
	const int map[4*63] = {
		// tile #3 (topmost)
		 /* 0*/ 0, MW+MI_A12, 0, MW+MI_C43,
		 /* 4*/ MW+MI_C40, 0, 0, MW+MI_A6,
		 /* 8*/ MW+MI_A17, 0, MW+MI_C42, 0,
		 /*12*/ 0, 0, MW+MI_A16, MW+MI_A5,
		 /*16*/ MW+MI_A7, MW+MI_C38, 0, MW+MI_A13,
		 /*20*/ 0, 0, 0, MW+MI_A8,
		 /*24*/ 0, 0, MW+MI_A10, MW+MI_C44,
		 /*28*/ 0, MW+MI_A14, MW+MI_A11, 0,
		 /*32*/ MW+MI_C45, 0, 0, 0,
		 /*36*/ MW+MI_C36, MW+MI_C47, 0, MW+MI_C46,
		 /*40*/ 0, 0, 0, 0,
		 /*44*/ MW+MI_A9, 0, 0, MW+MI_C37,
		 /*48*/ 0, 0, 0, 0,
		 /*52*/ MW+MI_A15, 0, 0, MW+MI_C41,
		 /*56*/ 0, MW+MI_C39, 0, 0,
		 /*60*/ 0, 0, 0,

		// tile #2
		 /* 0*/ 0, 0, 0, MW+MI_C31,
		 /* 4*/ MW+MI_C28, 0, 0, MW+MI_D16,
		 /* 8*/ MW+MI_A4, 0, MW+MI_C30, 0,
		 /*12*/ MW+MI_D17, 0, MW+MI_C34, MW+MI_OPMODE3,
		 /*16*/ MW+MI_C24, MW+MI_CED, 0, MW+MI_A0,
		 /*20*/ MW+MI_C25, 0, 0, MW+MI_C26,
		 /*24*/ MW+MI_OPMODE7, 0, MW+MI_C29, MW+MI_C32,
		 /*28*/ 0, MW+MI_A1, MW+MI_CEA, MW+MI_C35,
		 /*32*/ 0, 0, MW+MI_CE_CARRYIN, 0,
		 /*36*/ MW+MI_B16, 0, 0, MW+MI_A3,
		 /*40*/ 0, MW+MI_CEB, MW+MI_OPMODE2, 0,
		 /*44*/ MW+MI_CEP, 0, 0, MW+MI_B17,
		 /*48*/ MW+MI_C27, 0, 0, 0,
		 /*52*/ MW+MI_A2, 0, 0, MW+MI_CE_OPMODE,
		 /*56*/ 0, MW+MI_CEC, MW+MI_CEM, MW+MI_C33,
		 /*60*/ 0, 0, MW+MI_OPMODE5,

		// tile #1
		 /* 0*/ 0, MW+MI_B12, MW+MI_D14, 0,
		 /* 4*/ MW+MI_D11, MW+MI_OPMODE4, 0, 0,
		 /* 8*/ MW+MI_OPMODE1, MW+MI_C23, MW+MI_D12, 0,
		 /*12*/ 0, 0, MW+MI_D15, MW+MI_C12,
		 /*16*/ MW+MI_OPMODE6, MW+MI_C15, 0, MW+MI_C21,
		 /*20*/ 0, 0, 0, MW+MI_D10,
		 /*24*/ 0, 0, MW+MI_C18, MW+MI_B13,
		 /*28*/ MW+MI_D13, 0, 0, MW+MI_OPMODE0,
		 /*32*/ MW+MI_C22, 0, MW+MI_B9, 0,
		 /*36*/ MW+MI_C13, 0, 0, MW+MI_B15,
		 /*40*/ 0, MW+MI_C19, 0, 0,
		 /*44*/ MW+MI_C17, MW+MI_D9, 0, MW+MI_C14,
		 /*48*/ MW+MI_C16, 0, 0, 0,
		 /*52*/ MW+MI_B14, 0, 0, MW+MI_B11,
		 /*56*/ 0, MW+MI_B10, MW+MI_C20, 0,
		 /*60*/ 0, 0, 0,

		// tile #0 (bottommost)
		 /* 0*/ 0, MW+MI_D5, 0, MW+MI_C8,
		 /* 4*/ MW+MI_C5, 0, 0, MW+MI_D0,
		 /* 8*/ MW+MI_D8, 0, 0, 0,
		 /*12*/ 0, 0, MW+MI_C11, MW+MI_C0,
		 /*16*/ 0, MW+MI_C3, 0, 0,
		 /*20*/ MW+MI_C2, 0, 0, MW+MI_C4,
		 /*24*/ 0, MW+MI_D2, MW+MI_D4, 0,
		 /*28*/ MW+MI_C9, MW+MI_B6, MW+MI_B4, MW+MI_B8,
		 /*32*/ MW+MI_C10, 0, MW+MI_B2, 0,
		 /*36*/ 0, MW+MI_B7, MW+MI_B1, MW+MI_D7,
		 /*40*/ 0, MW+MI_C7, MW+MI_B0, 0,
		 /*44*/ MW+MI_B3, 0, 0, MW+MI_D1,
		 /*48*/ 0, 0, 0, 0,
		 /*52*/ 0, 0, 0, MW+MI_C6,
		 /*56*/ 0, MW+MI_D3, MW+MI_B5, MW+MI_D6,
		 /*60*/ 0, 0, MW+MI_C1 };
	int i;

	if (wire < MW || wire > MW_LAST) {
		HERE();
		*tile0_to_3 = -1;
		return;
	}
	for (i = 0; i < 4*63; i++) {
		if (map[i] == wire) {
			*tile0_to_3 = 3-(i/63);
			*wire0_to_62 = i%63;
			return;
		}
	}
	fprintf(stderr, "#E %s:%i unknown wire %i\n",
		__FILE__, __LINE__, wire);
	*tile0_to_3 = -1;
}

void fdev_macc_outbit(enum extra_wires wire, int* tile0_to_3, int* wire0_to_23)
{
	const int map[4*24] = {
		// tile #3 (topmost)
		 /* 0*/ MW+MO_P37, MW+MO_M33, MW+MO_P35, MW+MO_P41,
		 /* 4*/ MW+MO_M34, MW+MO_P38, MW+MO_P44, MW+MO_M29,
		 /* 8*/ MW+MO_M32, MW+MO_CARRYOUT, MW+MO_M30, MW+MO_P47,
		 /*12*/ MW+MO_P36, 0, 0, MW+MO_P40,
		 /*16*/ MW+MO_M35, MW+MO_M31, MW+MO_P42, MW+MO_M28,
		 /*20*/ MW+MO_P43, MW+MO_P45, MW+MO_P39, MW+MO_P46,
		// tile #2
		 /* 0*/ MW+MO_M21, MW+MO_P29, MW+MO_M18, MW+MO_M23,
		 /* 4*/ MW+MO_P32, 0, MW+MO_P31, MW+MO_P24,
		 /* 8*/ MW+MO_P28, MW+MO_P34, MW+MO_P25, MW+MO_M27,
		 /*12*/ MW+MO_M20, MW+MO_M25, MW+MO_M19, MW+MO_M22,
		 /*16*/ 0, MW+MO_P26, MW+MO_P30, 0,
		 /*20*/ MW+MO_M24, MW+MO_P33, MW+MO_P27, MW+MO_M26,
		// tile #1
		 /* 0*/ MW+MO_M11, MW+MO_P19, MW+MO_P12, MW+MO_M14,
		 /* 4*/ MW+MO_P21, MW+MO_M12, 0, MW+MO_P13,
		 /* 8*/ MW+MO_P18, 0, MW+MO_P15, MW+MO_P23,
		 /*12*/ MW+MO_P14, MW+MO_P20, MW+MO_M9, MW+MO_M13,
		 /*16*/ MW+MO_P22, MW+MO_P16, MW+MO_M15, MW+MO_M10,
		 /*20*/ MW+MO_M16, 0, MW+MO_P17, MW+MO_M17,
		// tile #0 (bottommost)
		 /* 0*/ MW+MO_P2, MW+MO_P6, 0, MW+MO_M3,
		 /* 4*/ MW+MO_P9, MW+MO_P3, MW+MO_M6, MW+MO_P1,
		 /* 8*/ MW+MO_M4, MW+MO_P11, MW+MO_M1, 0,
		 /*12*/ MW+MO_M0, MW+MO_M5, MW+MO_P0, MW+MO_P5,
		 /*16*/ MW+MO_P10, MW+MO_M2, MW+MO_P7, 0,
		 /*20*/ MW+MO_P8, MW+MO_M7, MW+MO_P4, MW+MO_M8 };
	int i;

	if (wire < MW || wire > MW_LAST) {
		HERE();
		*tile0_to_3 = -1;
		return;
	}
	for (i = 0; i < 4*24; i++) {
		if (map[i] == wire) {
			*tile0_to_3 = 3-(i/24);
			*wire0_to_23 = i%24;
			return;
		}
	}
	fprintf(stderr, "#E %s:%i unknown wire %i\n",
		__FILE__, __LINE__, wire);
	*tile0_to_3 = -1;
}
