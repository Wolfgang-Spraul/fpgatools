//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "model.h"

#define NUM_PF_BUFS	16

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
	char* prefix;

	if (is_aty(Y_CHIP_HORIZ_REGS, model, y)) {
		prefix = is_atx(X_CENTER_REGS_COL, model, x+3)
			? "REGC_INT_" : "REGH_";
	} else if (is_aty(Y_ROW_HORIZ_AXSYMM, model, y))
		prefix = "HCLK_";
	else if (is_aty(Y_INNER_TOP, model, y))
		prefix = "IOI_TTERM_";
	else if (is_aty(Y_INNER_BOTTOM, model, y))
		prefix = "IOI_BTERM_";
	else
		prefix = "";

	last_buf = (last_buf+1)%8;
	buf[last_buf][0] = 0;
	strcpy(buf[last_buf], prefix);
	strcat(buf[last_buf], wire_name);
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

int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o)
{
	struct fpga_tile* tile;
	uint16_t _name_i;
	int rc, i;

	tile = &model->tiles[y * model->x_width + x];
	rc = strarray_add(&model->str, connpt_name, &i);
	if (rc) return rc;
	if (i > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	_name_i = i;
	if (name_i) *name_i = i;

	// Search for an existing connection point under name.
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == _name_i)
			break;
	}
	if (conn_point_o) *conn_point_o = i;
	if (i < tile->num_conn_point_names) {
		if (warn_if_duplicate)
			fprintf(stderr,
				"Duplicate connection point name y%02i x%02u %s\n",
				y, x, connpt_name);
		return 0;
	}
	// This is the first connection under name, add name.
	connpt_names_array_append(tile, _name_i);
	return 0;
}

int has_device(struct fpga_model* model, int y, int x, int dev)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i;

	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == dev)
			return 1;
	}
	return 0;
}

int has_device_type(struct fpga_model* model, int y, int x, int dev, int subtype)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i;

	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type == dev) {
			switch (dev) {
				case DEV_LOGIC:
				case DEV_IOB:
					if (tile->devs[i].subtype == subtype)
						return 1;
					break;
				default: EXIT(1);
			}
		}
	}
	return 0;
}

int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2,
	int dup_warn)
{
	char name_buf[64];
	int rc;

	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix1);
	rc = add_connpt_name(model, y, x, name_buf, dup_warn,
		/*name_i*/ 0, /*connpt_o*/ 0);
	if (rc) goto xout;
	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix2);
	rc = add_connpt_name(model, y, x, name_buf, dup_warn,
		/*name_i*/ 0, /*connpt_o*/ 0);
	if (rc) goto xout;
	return 0;
xout:
	return rc;
}

#define CONNS_INCREMENT		128
#undef DBG_ADD_CONN_UNI

int add_conn_uni(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	struct fpga_tile* tile;
	uint16_t name1_i, name2_i;
	uint16_t* new_ptr;
	int conn_start, num_conn_point_dests_for_this_wire, rc, j, conn_point_o;

	rc = add_connpt_name(model, y1, x1, name1, 0 /* warn_if_duplicate */,
		&name1_i, &conn_point_o);
	if (rc) goto xout;

	rc = strarray_add(&model->str, name2, &j);
	if (rc) return rc;
	if (j > 0xFFFF) {
		fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	name2_i = j;
	tile = &model->tiles[y1 * model->x_width + x1];
	conn_start = tile->conn_point_names[conn_point_o*2];
	if (conn_point_o+1 >= tile->num_conn_point_names)
		num_conn_point_dests_for_this_wire = tile->num_conn_point_dests - conn_start;
	else
		num_conn_point_dests_for_this_wire = tile->conn_point_names[(conn_point_o+1)*2] - conn_start;

	// Is the connection made a second time?
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
		if (tile->conn_point_dests[j*3] == x2
		    && tile->conn_point_dests[j*3+1] == y2
		    && tile->conn_point_dests[j*3+2] == name2_i) {
			fprintf(stderr, "Duplicate conn (num_conn_point_dests %i): y%02i x%02i %s - y%02i x%02i %s.\n",
				num_conn_point_dests_for_this_wire, y1, x1, name1, y2, x2, name2);
			for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire; j++) {
				fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j,
					y1, x1, name1,
					tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
					strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
			}
			return 0;
		}
	}

	if (!(tile->num_conn_point_dests % CONNS_INCREMENT)) {
		new_ptr = realloc(tile->conn_point_dests, (tile->num_conn_point_dests+CONNS_INCREMENT)*3*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_dests = new_ptr;
	}
	if (tile->num_conn_point_dests > j)
		memmove(&tile->conn_point_dests[(j+1)*3], &tile->conn_point_dests[j*3], (tile->num_conn_point_dests-j)*3*sizeof(uint16_t));
	tile->conn_point_dests[j*3] = x2;
	tile->conn_point_dests[j*3+1] = y2;
	tile->conn_point_dests[j*3+2] = name2_i;
	tile->num_conn_point_dests++;
	while (conn_point_o+1 < tile->num_conn_point_names) {
		tile->conn_point_names[(conn_point_o+1)*2]++;
		conn_point_o++;
	}
#if DBG_ADD_CONN_UNI
	printf("conn_point_dests for y%02i x%02i %s now:\n", y1, x1, name1);
	for (j = conn_start; j < conn_start + num_conn_point_dests_for_this_wire+1; j++) {
		fprintf(stderr, "c%i: y%02i x%02i %s -> y%02i x%02i %s\n", j, y1, x1, name1,
			tile->conn_point_dests[j*3+1], tile->conn_point_dests[j*3],
			strarray_lookup(&model->str, tile->conn_point_dests[j*3+2]));
	}
#endif
	return 0;
xout:
	return rc;
}

int add_conn_uni_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_uni(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

int add_conn_bi(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	int rc = add_conn_uni(model, y1, x1, name1, y2, x2, name2);
	if (rc) return rc;
	return add_conn_uni(model, y2, x2, name2, y1, x1, name1);
}

int add_conn_bi_pref(struct fpga_model* model, int y1, int x1, const char* name1, int y2, int x2, const char* name2)
{
	return add_conn_bi(model,
		y1, x1, wpref(model, y1, x1, name1),
		y2, x2, wpref(model, y2, x2, name2));
}

int add_conn_range(struct fpga_model* model, add_conn_f add_conn_func, int y1, int x1, const char* name1, int start1, int last1, int y2, int x2, const char* name2, int start2)
{
	char buf1[128], buf2[128];
	int rc, i;

	if (last1 <= start1)
		return (*add_conn_func)(model, y1, x1, name1, y2, x2, name2);
	for (i = start1; i <= last1; i++) {
		snprintf(buf1, sizeof(buf1), name1, i);
		if (start2 & COUNT_DOWN)
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)-(i-start1));
		else
			snprintf(buf2, sizeof(buf2), name2, (start2 & COUNT_MASK)+(i-start1));
		rc = (*add_conn_func)(model, y1, x1, buf1, y2, x2, buf2);
		if (rc) return rc;
	}
	return 0;
}

int add_conn_net(struct fpga_model* model, add_conn_f add_conn_func, struct w_net* net)
{
	int i, j, rc;

	for (i = 0; net->pts[i].name[0] && i < sizeof(net->pts)/sizeof(net->pts[0]); i++) {
		for (j = i+1; net->pts[j].name[0] && j < sizeof(net->pts)/sizeof(net->pts[0]); j++) {
			rc = add_conn_range(model, add_conn_func,
				net->pts[i].y, net->pts[i].x,
				net->pts[i].name,
				net->pts[i].start_count,
				net->pts[i].start_count + net->last_inc,
				net->pts[j].y, net->pts[j].x,
				net->pts[j].name,
				net->pts[j].start_count);
			if (rc) goto xout;
		}
	}
	return 0;
xout:
	return rc;
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

void seed_strx(struct fpga_model* model, struct seed_data* data)
{
	int x, i;
	for (x = 0; x < model->x_width; x++) {
		model->tmp_str[x] = 0;
		for (i = 0; data[i].x_flags; i++) {
			if (is_atx(data[i].x_flags, model, x))
				model->tmp_str[x] = data[i].str;
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
	if (check & Y_INNER_TOP && y == TOP_INNER_ROW) return 1;
	if (check & Y_INNER_BOTTOM && y == model->y_height-BOT_INNER_ROW) return 1;
	if (check & Y_CHIP_HORIZ_REGS && y == model->center_y) return 1;
	if (check & (Y_ROW_HORIZ_AXSYMM|Y_BOTTOM_OF_ROW)) {
		int row_pos;
		is_in_row(model, y, 0 /* row_num */, &row_pos);
		if (check & Y_ROW_HORIZ_AXSYMM && row_pos == 8) return 1;
		if (check & Y_BOTTOM_OF_ROW && row_pos == 16) return 1;
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
	if (check & YX_ROUTING_TILE
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->x_width-5
		|| x == model->center_x-3)) {
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
	return 0;
}

void is_in_row(const struct fpga_model* model, int y,
	int* row_num, int* row_pos)
{
	int dist_to_center;

	if (row_num) *row_num = -1;
	if (row_pos) *row_pos = -1;
	if (y < 2) return;
	// normalize y to beginning of rows
	y -= 2;

	// calculate distance to center and check
	// that y is not pointing to the center
	dist_to_center = (model->cfg_rows/2)*(8+1/*middle of row*/+8);
	if (y == dist_to_center) return;
	if (y > dist_to_center) y--;

	// check that y is not pointing past the last row
	if (y >= model->cfg_rows*(8+1+8)) return;

	if (row_num) *row_num = model->cfg_rows-(y/(8+1+8))-1;
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

const char* fpga_wire2str(enum extra_wires wire)
{
 	enum { NUM_BUFS = 8, BUF_SIZE = 64 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;

	switch (wire) {
		case GFAN0: return "GFAN0";
		case GFAN1: return "GFAN1";
		case CLK0: return "CLK0";
		case CLK1: return "CLK1";
		case SR0: return "SR0";
		case SR1: return "SR1";
		case GND_WIRE: return "GND_WIRE";
		case VCC_WIRE: return "VCC_WIRE";
		case FAN_B: return "FAN_B";
		case LOGICIN20: return "LOGICIN20";
		case LOGICIN21: return "LOGICIN21";
		case LOGICIN44: return "LOGICIN44";
		case LOGICIN52: return "LOGICIN52";
		case LOGICIN_N21: return "LOGICIN_N21";
		case LOGICIN_N28: return "LOGICIN_N28";
		case LOGICIN_N52: return "LOGICIN_N52";
		case LOGICIN_N60: return "LOGICIN_N60";
		case LOGICIN_S20: return "LOGICIN_S20";
		case LOGICIN_S36: return "LOGICIN_S36";
		case LOGICIN_S44: return "LOGICIN_S44";
		case LOGICIN_S62: return "LOGICIN_S62";
		default: ;
	}

	last_buf = (last_buf+1)%NUM_BUFS;
	buf[last_buf][0] = 0;
	if (wire >= GCLK0 && wire <= GCLK15)
		snprintf(buf[last_buf], sizeof(buf[0]), "GCLK%i", wire-GCLK0);
	else if (wire >= DW && wire <= DW_LAST) {
		char beg_end;
		int flags;

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
	} else HERE();
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
