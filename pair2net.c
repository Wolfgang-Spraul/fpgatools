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

#include "helper.h"

#define ALLOC_INCREMENT 16

static int add_entry(uint32_t** net, uint32_t new_idx)
{
	if (!*net) {
		*net = malloc(ALLOC_INCREMENT*sizeof(**net));
		if (!(*net)) {
			fprintf(stderr, "Out of memory in %s:%i\n",
				__FILE__, __LINE__);
			return -1;
		}
		**net = 1;
	} else {
		uint32_t num_entries = **net;
		if (!(num_entries % ALLOC_INCREMENT)) {
			void* new_ptr = realloc(*net,
				(num_entries + ALLOC_INCREMENT)*sizeof(**net));
			if (!new_ptr) {
				fprintf(stderr, "Out of memory in %s:%i\n",
					__FILE__, __LINE__);
				return -1;
			}
			*net = new_ptr;
		}
	}
	(*net)[**net] = new_idx;
	(**net)++;
	return 0;
}

static int print_nets(uint32_t** nets, struct hashed_strarray* connpt_names)
{
	int i, j, num_connpoints, num_nets, largest_net, total_net_connpoints;
	const char* str;

	num_connpoints = 0;
	num_nets = 0;
	largest_net = 0;
	total_net_connpoints = 0;

	for (i = 0; i < STRIDX_1M; i++) {
		if (nets[i]) {
			num_connpoints++;
			if (!((uint64_t)nets[i] & 1)) {
				num_nets++;
				if (!(*nets[i]))
					fprintf(stderr, "Internal error - 0 entries in net %i\n", i);
				total_net_connpoints += *nets[i] - 1;
				if (*nets[i] - 1 > largest_net)
					largest_net = *nets[i] - 1;
				for (j = 1; j < *nets[i]; j++) {
					str = strarray_lookup(connpt_names, nets[i][j]);
					if (!str) {
						fprintf(stderr, "Internal error - cannot find str %i\n", nets[i][j]);
						continue;
					}
					if (j > 1) fputc(' ', stdout);
					fputs(str, stdout);
				}
				fputc('\n', stdout);
			}
		}
	}
	return 0;
}

struct hashed_strarray* g_sort_connpt_names;

static int sort_net(const void* a, const void* b)
{
	const uint32_t* _a, *_b;
	const char* a_str, *b_str;

	_a = a;
	_b = b;
	a_str = strarray_lookup(g_sort_connpt_names, *_a);
	b_str = strarray_lookup(g_sort_connpt_names, *_b);
	if (!a_str || !b_str) {
		fprintf(stderr, "Internal error in %s:%i - cannot find str %i or %i\n",
			__FILE__, __LINE__, *_a, *_b);
		return 0;
	}
	return strcmp(a_str, b_str);
}

static int sort_nets(uint32_t** nets, struct hashed_strarray* connpt_names)
{
	int i;

	g_sort_connpt_names = connpt_names;
	for (i = 0; i < STRIDX_1M; i++) {
		if (nets[i] && !((uint64_t)nets[i] & 1)) {
			qsort(&nets[i][1], *nets[i]-1, sizeof(nets[i][1]), sort_net);
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	char line[1024], point_a[1024], point_b[1024];
	struct hashed_strarray connpt_names;
	FILE* fp = 0;
	int i, rc, point_a_idx, point_b_idx, existing_net_idx, new_net_idx;
	int net_data_idx;
	uint32_t** nets;

	if (argc < 2) {
		fprintf(stderr,
			"\n"
			"pair2net - finds all pairs connected to the same net\n"
			"Usage: %s <data_file> | '-' for stdin\n", argv[0]);
		goto xout;
	}
	if (strarray_init(&connpt_names, STRIDX_1M)) {
		fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
		goto xout;
	}

	nets = calloc(STRIDX_1M, sizeof(*nets));
	if (!nets) {
		fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
		goto xout;
	}
	if (!strcmp(argv[1], "-"))
		fp = stdin;
	else {
		fp = fopen(argv[1], "r");
		if (!fp) {
			fprintf(stderr, "Error opening %s.\n", argv[1]);
			goto xout;
		}
	}
	while (fgets(line, sizeof(line), fp)) {
		i = sscanf(line, "%s%s", point_a, point_b);
		if (i != 2) continue;

		i = strlen(point_b);
		if (i && point_b[i-1] == '\n')
			point_b[i-1] = 0;
		rc = strarray_add(&connpt_names, point_a, &point_a_idx);
		if (rc) {
			fprintf(stderr, "Out of memory in %s:%i\n",
				__FILE__, __LINE__);
			goto xout;
		}
		rc = strarray_add(&connpt_names, point_b, &point_b_idx);
		if (rc) {
			fprintf(stderr, "Out of memory in %s:%i\n",
				__FILE__, __LINE__);
			goto xout;
		}
		if (nets[point_a_idx] && nets[point_b_idx]) {
			continue;}
		if (nets[point_a_idx] || nets[point_b_idx]) {
			if (nets[point_a_idx]) {
				existing_net_idx = point_a_idx;
				new_net_idx = point_b_idx;
			} else { // point_b_idx exists
				existing_net_idx = point_b_idx;
				new_net_idx = point_a_idx;
			}
			if ((uint64_t) nets[existing_net_idx] & 1)
				net_data_idx = (uint64_t) nets[existing_net_idx] >> 32;
			else
				net_data_idx = existing_net_idx;
			// add new_net_idx to net data
			rc = add_entry(&nets[net_data_idx], new_net_idx);
			if (rc) goto xout;
			// point to net data from new_net_idx
			nets[new_net_idx] = (uint32_t*) (((uint64_t) net_data_idx << 32) | 1);
		} else {
			rc = add_entry(&nets[point_a_idx], point_a_idx);
			if (rc) goto xout;
			rc = add_entry(&nets[point_a_idx], point_b_idx);
			if (rc) goto xout;
			nets[point_b_idx] = (uint32_t*) (((uint64_t) point_a_idx << 32) | 1);
		}
	}
	rc = sort_nets(nets, &connpt_names);
	if (rc) goto xout;
	rc = print_nets(nets, &connpt_names);
	if (rc) goto xout;
	fclose(fp);
	return EXIT_SUCCESS;
xout:
	if(fp) fclose(fp);
	return EXIT_FAILURE;
}
