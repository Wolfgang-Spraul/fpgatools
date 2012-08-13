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

	if (strarray_find(&model->str, name, &i))
		ABORT(1);
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

static int _add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name, int warn_if_duplicate, uint16_t* name_i,
	int* conn_point_o);

int add_connpt_name(struct fpga_model* model, int y, int x,
	const char* connpt_name)
{
	return _add_connpt_name(model, y, x, connpt_name,
		1 /* warn_if_duplicate */,
		0 /* name_i */, 0 /* conn_point_o */);
}

#define CONN_NAMES_INCREMENT	128

static int _add_connpt_name(struct fpga_model* model, int y, int x,
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
	if (!(tile->num_conn_point_names % CONN_NAMES_INCREMENT)) {
		uint16_t* new_ptr = realloc(tile->conn_point_names,
			(tile->num_conn_point_names+CONN_NAMES_INCREMENT)*2*sizeof(uint16_t));
		if (!new_ptr) {
			fprintf(stderr, "Out of memory %s:%i\n", __FILE__, __LINE__);
			return 0;
		}
		tile->conn_point_names = new_ptr;
	}
	tile->conn_point_names[tile->num_conn_point_names*2] = tile->num_conn_point_dests;
	tile->conn_point_names[tile->num_conn_point_names*2+1] = _name_i;
	tile->num_conn_point_names++;
	return 0;
}

int has_device(struct fpga_model* model, int y, int x, int dev)
{
	struct fpga_tile* tile = YX_TILE(model, y, x);
	int i;

	for (i = 0; i < tile->num_devices; i++) {
		if (tile->devices[i].type == dev)
			return 1;
	}
	return 0;
}

int add_connpt_2(struct fpga_model* model, int y, int x,
	const char* connpt_name, const char* suffix1, const char* suffix2)
{
	char name_buf[64];
	int rc;

	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix1);
	rc = add_connpt_name(model, y, x, name_buf);
	if (rc) goto xout;
	snprintf(name_buf, sizeof(name_buf), "%s%s", connpt_name, suffix2);
	rc = add_connpt_name(model, y, x, name_buf);
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

	rc = _add_connpt_name(model, y1, x1, name1, 0 /* warn_if_duplicate */,
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

#define SWITCH_ALLOC_INCREMENT 64

#define DBG_ALLOW_ADDPOINTS

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
	rc = strarray_find(&model->str, from, &from_idx);
	if (rc) goto xout;
	rc = strarray_find(&model->str, to, &to_idx);
	if (rc) goto xout;
#endif
	if (from_idx == STRIDX_NO_ENTRY || to_idx == STRIDX_NO_ENTRY) {
		fprintf(stderr, "No string for switch from %s (%i) or %s (%i).\n",
			from, from_idx, to, to_idx);
		return -1;
	}

	from_connpt_o = -1;
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == from_idx) {
			from_connpt_o = i;
			break;
		}
	}
#ifdef DBG_ALLOW_ADDPOINTS
	if (from_connpt_o == -1) {
		rc = add_connpt_name(model, y, x, from);
		if (rc) goto xout;
		for (i = 0; i < tile->num_conn_point_names; i++) {
			if (tile->conn_point_names[i*2+1] == from_idx) {
				from_connpt_o = i;
				break;
			}
		}
	}
#endif
	to_connpt_o = -1;
	for (i = 0; i < tile->num_conn_point_names; i++) {
		if (tile->conn_point_names[i*2+1] == to_idx) {
			to_connpt_o = i;
			break;
		}
	}
#ifdef DBG_ALLOW_ADDPOINTS
	if (to_connpt_o == -1) {
		rc = add_connpt_name(model, y, x, to);
		if (rc) goto xout;
		for (i = 0; i < tile->num_conn_point_names; i++) {
			if (tile->conn_point_names[i*2+1] == to_idx) {
				to_connpt_o = i;
				break;
			}
		}
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

	for (i = 0; i < tile->num_switches; i++) {
		if ((tile->switches[i] & 0x3FFFFFFF) == (new_switch & 0x3FFFFFFF)) {
			fprintf(stderr, "Internal error in %s:%i duplicate switch from %s to %s\n",
				__FILE__, __LINE__, from, to);
			return -1;
		}
	}
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
	return 0;
}

int is_atx(int check, struct fpga_model* model, int x)
{
	if (x < 0) return 0;
	if (check & X_OUTER_LEFT && !x) return 1;
	if (check & X_INNER_LEFT && x == 1) return 1;
	if (check & X_INNER_RIGHT && x == model->x_width-2) return 1;
	if (check & X_OUTER_RIGHT && x == model->x_width-1) return 1;
	if (check & X_ROUTING_COL
	    && (model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	        || x == LEFT_IO_ROUTING || x == model->x_width-5
		|| x == model->center_x-3)) return 1;
	if (model->tiles[x].flags & TF_FABRIC_ROUTING_COL) {
		if (check & X_ROUTING_TO_BRAM_COL
		    && model->tiles[x+1].flags & TF_FABRIC_BRAM_VIA_COL
		    && model->tiles[x+2].flags & TF_FABRIC_BRAM_COL) return 1;
		if (check & X_ROUTING_TO_MACC_COL
		    && model->tiles[x+1].flags & TF_FABRIC_MACC_VIA_COL
		    && model->tiles[x+2].flags & TF_FABRIC_MACC_COL) return 1;
	}
	if (check & X_ROUTING_NO_IO && model->tiles[x].flags & TF_ROUTING_NO_IO) return 1;
	if (check & X_ROUTING_HAS_IO && !(model->tiles[x].flags & TF_ROUTING_NO_IO)) return 1;
	if (check & X_LOGIC_COL
	    && (model->tiles[x].flags & TF_FABRIC_LOGIC_COL
	        || x == model->center_x-2)) return 1;
	if (check & X_FABRIC_ROUTING_COL && model->tiles[x].flags & TF_FABRIC_ROUTING_COL) return 1;
	// todo: the routing/no_io flags could be cleaned up
	if (check & X_FABRIC_LOGIC_ROUTING_COL
	    && model->tiles[x].flags & TF_FABRIC_ROUTING_COL
	    && model->tiles[x+1].flags & TF_FABRIC_LOGIC_COL) return 1;
	if (check & X_FABRIC_LOGIC_COL && model->tiles[x].flags & TF_FABRIC_LOGIC_COL) return 1;
	if (check & X_FABRIC_LOGIC_IO_COL && model->tiles[x].flags & TF_FABRIC_LOGIC_COL && !(model->tiles[x-1].flags & TF_ROUTING_NO_IO)) return 1;
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

int row_num(int y, struct fpga_model* model)
{
	int result;
	is_in_row(model, y, &result, 0 /* row_pos */);
	return result;
}

int row_pos(int y, struct fpga_model* model)
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
