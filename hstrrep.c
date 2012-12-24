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

int main(int argc, char** argv)
{
	char line[1024], search_str[1024], replace_str[1024];
	char* next_wrd, *lasts;
	const char* replace_ptr;
	struct hashed_strarray search_arr, replace_arr;
	FILE* fp = 0;
	int i, rc, search_idx;

	if (argc < 3) {
		fprintf(stderr,
			"\n"
			"hstrrep - hashed string replace\n"
			"Usage: %s <data_file> <token_file>\n"
			"  token_file is parsed into two parts: first word is the string\n"
			"  that is to be replaced, rest of line the replacement.\n", argv[0]);
		goto xout;
	}

	if (strarray_init(&search_arr, STRIDX_1M)) {
		fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
		goto xout;
	}
	if (strarray_init(&replace_arr, STRIDX_1M)) {
		fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
		goto xout;
	}

	//
	// Read search and replace tokens into hashed strarray
	//

	fp = fopen(argv[2], "r");
	if (!fp) {
		fprintf(stderr, "Error opening %s.\n", argv[2]);
		goto xout;
	}
	while (fgets(line, sizeof(line), fp)) {
		memset(replace_str, 0, sizeof(replace_str));
		i = sscanf(line, " %[^ ] %1023c", search_str, replace_str);
		if (i == 2) {
			i = strlen(replace_str);
			if (i && replace_str[i-1] == '\n')
				replace_str[i-1] = 0;
			rc = strarray_add(&search_arr, search_str, &search_idx);
			if (rc) {
				fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
				goto xout;
			}
			rc = strarray_stash(&replace_arr, replace_str, search_idx);
			if (rc) {
				fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
				goto xout;
			}
		}
	}
	fclose(fp);

	//
	// Go through data file and search and replace
	//

	fp = fopen(argv[1], "r");
	if (!fp) {
		fprintf(stderr, "Error opening %s.\n", argv[1]);
		goto xout;
	}
	while (fgets(line, sizeof(line), fp)) {
		next_wrd = strtok_r(line, " \n", &lasts);
		if (next_wrd) {
			do {
				search_idx = strarray_find(&search_arr, next_wrd);
				if (search_idx == STRIDX_NO_ENTRY)
					fputs(next_wrd, stdout);
				else {
					replace_ptr = strarray_lookup(&replace_arr, search_idx);
					if (!replace_ptr) {
						fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
						goto xout;
					}
					fputs(replace_ptr, stdout);
				}
				next_wrd = strtok_r(0, " \n", &lasts);
				if (next_wrd)
					putchar(' ');
			} while ( next_wrd );
			putchar('\n');
		}
	}
	fclose(fp);
	return EXIT_SUCCESS;
xout:
	if (fp) fclose(fp);
	return EXIT_FAILURE;
}
