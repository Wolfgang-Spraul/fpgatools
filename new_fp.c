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

#include "model.h"
#include "helper.h"

#define PRINT_FLAG(f)	if (tf & f) { printf (" %s", #f); tf &= ~f; }

int printf_tiles(struct fpga_model* model);
int printf_static_conns(struct fpga_model* model);

int main(int argc, char** argv)
{
	struct fpga_model model;
	int rc;

	if ((rc = fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING)))
		goto fail;

	printf("fpga_floorplan_format 1\n");

	//
	// What needs to be in the file:
	// - all devices, configuration for each device
	//   probably multiple lines that are adding config strings
	// - wires maybe separately, and/or as named connection points
	//   in tiles?
	// - connection pairs that can be enabled/disabled
	// - global flags and configuration registers
	// - the static data should be optional (unused conn pairs,
	//   unused devices, wires)
	//
	// - each line should be in the global namespace, line order
	//   should not matter
	// - file should be easily parsable with bison
	// - lines should typically not exceed 80 characters
	//

	rc = printf_tiles(&model);
	if (rc) goto fail;

	rc = printf_static_conns(&model);
	if (rc) goto fail;

	// todo: static_net <lists all wires connected together statically in a long line>
	return EXIT_SUCCESS;

fail:
	return rc;
}

int printf_tiles(struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y;

	for (x = 0; x < model->tile_x_range; x++) {
		printf("\n");
		for (y = 0; y < model->tile_y_range; y++) {
			tile = &model->tiles[y*model->tile_x_range + x];

			printf("tile y%02i x%02i", y, x);

			if (tile->type == NA && !(tile->flags)) {
				printf(" -\n");
				continue;
			}
			if (tile->type != NA)
				printf(" name %s", fpga_tiletype_str(tile->type));
			if (tile->flags) {
				int tf = tile->flags;
				printf(" flags");

				PRINT_FLAG(TF_LOGIC_XL_DEV);
				PRINT_FLAG(TF_LOGIC_XM_DEV);
				PRINT_FLAG(TF_IOLOGIC_DELAY_DEV);

				if (tf)
					printf(" 0x%x", tf);
			}
			printf("\n");
		}
	}
	return 0;
}

struct conn_printf_data
{
	int src_x, src_y; // src_x is -1 for empty entry
	char src_conn_point[41];
	int num_combined_wires; // 0 for no suffix, otherwise 4 for 0:3 etc.
	int dest_x, dest_y;
	char dest_conn_point[41];
};

#define MAX_CONN_PRINTF_ENTRIES	40000
static struct conn_printf_data s_conn_printf_buf[MAX_CONN_PRINTF_ENTRIES];
static int s_conn_printf_entries;

int sort_by_tile(const void* a, const void* b)
{
	int i, rc;

	struct conn_printf_data* _a = (struct conn_printf_data*) a;
	struct conn_printf_data* _b = (struct conn_printf_data*) b;

	if (_a->src_x < 0 && _b->src_x < 0) return 0;
	if (_a->src_x < 0) return -1;
	if (_b->src_x < 0) return 1;

	if (_a->src_x != _b->src_x)
		return _a->src_x - _b->src_x;
	if (_a->src_y != _b->src_y)
		return _a->src_y - _b->src_y;
	if (_a->dest_x != _b->dest_x)
		return _a->dest_x - _b->dest_x;
	if (_a->dest_y != _b->dest_y)
		return _a->dest_y - _b->dest_y;
	if (_a->num_combined_wires != _b->num_combined_wires)
		return _b->num_combined_wires - _a->num_combined_wires;

	rc = compare_with_number(_a->src_conn_point, _b->src_conn_point);
	if (rc) return rc;

	// following is a special version of strcmp(_a->dest_conn_point, _b->dest_conn_point)
	// to push '_' before digits.
	for (i = 0; _a->dest_conn_point[i]; i++) {
		if (_a->dest_conn_point[i] != _b->dest_conn_point[i]) {
			// The point here is to get a NN2B0 -> NN2E_S0 before
			// a NN2B0 -> NN2E0 so that later wire combinations
			// are easier.
			if (_a->dest_conn_point[i] >= '0' && _a->dest_conn_point[i] <= '9'
			    && _b->dest_conn_point[i] == '_')
				return 1;
			if (_b->dest_conn_point[i] >= '0' && _b->dest_conn_point[i] <= '9'
			    && _a->dest_conn_point[i] == '_')
				return -1;
			return _a->dest_conn_point[i] - _b->dest_conn_point[i];
		}
	}
	return _a->dest_conn_point[i] - _b->dest_conn_point[i];
}

int find_last_wire_digit(const char* wire_str, int* start_o, int* end_o, int* num)
{
	// There are some known suffixes that may confuse our
	// 'last digit' logic so we skip those.
	static const char suffix[8][16] =
		{ "_S0", "_N3", "_INT0", "_INT1", "_INT2", "_INT3", "" };
	int i, suffix_len, right_bound, _end_o, base;

	right_bound = strlen(wire_str);
	for (i = 0; suffix[i][0]; i++) {
		suffix_len = strlen(suffix[i]);
		if (right_bound > suffix_len
		    && !strcmp(&wire_str[right_bound - suffix_len],
			suffix[i])) {
			right_bound -= suffix_len;
			break;
		}
	}
	_end_o = -1;
	for (i = right_bound; i; i--) {
		if (wire_str[i-1] >= '0' && wire_str[i-1] <= '9' && _end_o == -1) {
			_end_o = i;
			continue;
		}
		if ((wire_str[i-1] < '0' || wire_str[i-1] > '9') && _end_o != -1) {
			*start_o = i;
			*end_o = _end_o;
			*num = 0;
			base = 1;
			for (i = *end_o - 1; i >= *start_o; i--) {
				*num += (wire_str[i] - '0')*base;
				base *= 10;
			}
			return 1;
		}
	}
	return 0;
}

void sort_and_reduce_printf_buf()
{
	int src_digit_start_o, src_digit_end_o, src_digit;
	int dest_digit_start_o, dest_digit_end_o, dest_digit;
	int second_src_digit_start_o, second_src_digit_end_o, second_src_digit;
	int second_dest_digit_start_o, second_dest_digit_end_o, second_dest_digit;
	int i, j, sequence_size;
	int sequence[100]; // support up to 100 elements in sequence
	char old_suffix[41];

	if (s_conn_printf_entries < 2)
		return;

	// First sort by y/x src, then y/x dest, then src conn point, then
	// dest conn points - all in preparation of the numbered wires reduction.
	qsort(s_conn_printf_buf, s_conn_printf_entries,
		sizeof(s_conn_printf_buf[0]), sort_by_tile);

	// Reduce numbered wire sets.
	for (i = 0; i < s_conn_printf_entries-1; i++) {
		if (s_conn_printf_buf[i].src_x < 0)
			continue;
		if (!find_last_wire_digit(s_conn_printf_buf[i].src_conn_point, &src_digit_start_o, &src_digit_end_o, &src_digit)
		    || !find_last_wire_digit(s_conn_printf_buf[i].dest_conn_point, &dest_digit_start_o, &dest_digit_end_o, &dest_digit))
			continue;

		// Search for a contiguous sequence of increasing numbers, but support
		// skipping over unrelated pairs that may be inside our sequence due
		// to sorting.
		sequence[0] = i;
		sequence_size = 1;
		for (j = i+1; j < s_conn_printf_entries; j++) {
			if (j > sequence[sequence_size-1]+4) // search over at most 4 non-matches
				break;
			if (s_conn_printf_buf[j].src_x < 0)
				continue;
			// is the j connection from and to the same tiles as the i connection?
			if (s_conn_printf_buf[i].src_x != s_conn_printf_buf[j].src_x
			    || s_conn_printf_buf[i].src_y != s_conn_printf_buf[j].src_y
			    || s_conn_printf_buf[i].dest_x != s_conn_printf_buf[j].dest_x
			    || s_conn_printf_buf[i].dest_y != s_conn_printf_buf[j].dest_y)
				continue;
			if (!find_last_wire_digit(s_conn_printf_buf[j].src_conn_point, &second_src_digit_start_o, &second_src_digit_end_o, &second_src_digit))
				continue;
			if (!find_last_wire_digit(s_conn_printf_buf[j].dest_conn_point, &second_dest_digit_start_o, &second_dest_digit_end_o, &second_dest_digit))
				continue;
			if (second_src_digit != src_digit+sequence_size
			    || second_dest_digit != dest_digit+sequence_size)
				continue;
			if (src_digit_start_o != second_src_digit_start_o
			    || strncmp(s_conn_printf_buf[i].src_conn_point, s_conn_printf_buf[j].src_conn_point, src_digit_start_o))
				continue;
			if (strcmp(&s_conn_printf_buf[i].src_conn_point[src_digit_end_o], &s_conn_printf_buf[j].src_conn_point[second_src_digit_end_o]))
				continue;
			if (dest_digit_start_o != second_dest_digit_start_o
			    || strncmp(s_conn_printf_buf[i].dest_conn_point, s_conn_printf_buf[j].dest_conn_point, dest_digit_start_o))
				continue;
			if (strcmp(&s_conn_printf_buf[i].dest_conn_point[dest_digit_end_o], &s_conn_printf_buf[j].dest_conn_point[second_dest_digit_end_o]))
				continue;
			if (sequence_size >= sizeof(sequence)/sizeof(sequence[0])) {
				fprintf(stderr, "Internal error - too long sequence in line %i\n", __LINE__);
				break;
			}
			sequence[sequence_size++] = j;
		}
		if (sequence_size < 2)
			continue;
		strcpy(old_suffix, &s_conn_printf_buf[sequence[0]].src_conn_point[src_digit_end_o]);
		sprintf(&s_conn_printf_buf[sequence[0]].src_conn_point[src_digit_start_o], "%i:%i%s", src_digit, src_digit+sequence_size-1, old_suffix);
		strcpy(old_suffix, &s_conn_printf_buf[sequence[0]].dest_conn_point[dest_digit_end_o]);
		sprintf(&s_conn_printf_buf[sequence[0]].dest_conn_point[dest_digit_start_o], "%i:%i%s", dest_digit, dest_digit+sequence_size-1, old_suffix);
		s_conn_printf_buf[sequence[0]].num_combined_wires = sequence_size;
		for (i = 1; i < sequence_size; i++)
			s_conn_printf_buf[sequence[i]].src_x = -1;
		i = sequence[0];
	}
	
	// Second round of sorting, this time the largest numbered wire
	// sets are defined and will move up.
	qsort(s_conn_printf_buf, s_conn_printf_entries, sizeof(s_conn_printf_buf[0]),
		sort_by_tile);
}

int printf_static_conns(struct fpga_model* model)
{
	struct fpga_tile* tile;
	char tmp_line[512];
	const char* conn_point_name_src, *other_tile_connpt_str;
	uint16_t other_tile_connpt_str_i;
	int x, y, i, j, conn_point_dests_o, num_dests_for_this_conn_point;
	int other_tile_x, other_tile_y, first_conn_printed;

	for (x = 0; x < model->tile_x_range; x++) {
		for (y = 0; y < model->tile_y_range; y++) {
			tile = &model->tiles[y*model->tile_x_range + x];

			s_conn_printf_entries = 0;
			for (i = 0; i < tile->num_conn_point_names; i++) {
				conn_point_dests_o = tile->conn_point_names[i*2];
				if (i < tile->num_conn_point_names-1)
					num_dests_for_this_conn_point = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
				else
					num_dests_for_this_conn_point = tile->num_conn_point_dests - conn_point_dests_o;
				if (!num_dests_for_this_conn_point)
					continue;
				conn_point_name_src = strarray_lookup(&model->str, tile->conn_point_names[i*2+1]);
				if (!conn_point_name_src) {
					fprintf(stderr, "Cannot lookup src conn point name index %i, x%i y%i i%i\n",
						tile->conn_point_names[i*2+1], x, y, i);
					continue;
				}
				for (j = 0; j < num_dests_for_this_conn_point; j++) {
					other_tile_x = tile->conn_point_dests[(conn_point_dests_o+j)*3];
					other_tile_y = tile->conn_point_dests[(conn_point_dests_o+j)*3+1];
					other_tile_connpt_str_i = tile->conn_point_dests[(conn_point_dests_o+j)*3+2];

					other_tile_connpt_str = strarray_lookup(&model->str, other_tile_connpt_str_i);
					if (!other_tile_connpt_str) {
						fprintf(stderr, "Lookup err line %i, dest pt %i, dest x%i y%i, from x%i y%i j%i num_dests %i src_pt %s\n",
							__LINE__, other_tile_connpt_str_i, other_tile_x, other_tile_y, x, y, j, num_dests_for_this_conn_point, conn_point_name_src);
						continue;
					}
					s_conn_printf_buf[s_conn_printf_entries].src_y = y;
					s_conn_printf_buf[s_conn_printf_entries].src_x = x;
					strcpy(s_conn_printf_buf[s_conn_printf_entries].src_conn_point, conn_point_name_src);
					s_conn_printf_buf[s_conn_printf_entries].num_combined_wires = 0;
					s_conn_printf_buf[s_conn_printf_entries].dest_y = other_tile_y;
					s_conn_printf_buf[s_conn_printf_entries].dest_x = other_tile_x;
					strcpy(s_conn_printf_buf[s_conn_printf_entries].dest_conn_point, other_tile_connpt_str);
					s_conn_printf_entries++;
				}
			}
			sort_and_reduce_printf_buf();
			first_conn_printed = 0;
			for (i = 0; i < s_conn_printf_entries; i++) {
				if (s_conn_printf_buf[i].src_x < 0)
					continue;
				if (!first_conn_printed) {
					printf("\n");
					first_conn_printed = 1;
				}
				sprintf(tmp_line, "static_conn y%02i-x%02i-%s ",
					s_conn_printf_buf[i].src_y,
					s_conn_printf_buf[i].src_x,
					s_conn_printf_buf[i].src_conn_point);
				j = strlen(tmp_line);
				while (j < 45)
					tmp_line[j++] = ' ';
				sprintf(&tmp_line[j], "y%02i-x%02i-%s\n",
					s_conn_printf_buf[i].dest_y,
					s_conn_printf_buf[i].dest_x,
					s_conn_printf_buf[i].dest_conn_point);
				printf(tmp_line);
			}
		}
	}
	return 0;
}
