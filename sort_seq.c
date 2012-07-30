//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_LENGTH	1024

static int s_numlines;
static char s_lines[1000][LINE_LENGTH];

static int is_known_suffix(const char* str)
{
	static const char known_suffix[32][16] =
		{ "_S0", "_N3", "_INT0", "_INT1", "_INT2", "_INT3",
		  "_TEST", "_BRK", "_BUF", "_FOLD", "_BRAM", "_BRAM_INTER",
		  "_CLB", "_DSP", "_INT", "_MCB", "_DOWN", "_UP",
		  "_E", "_W", "_S", "_N", "_M", "_EXT", "_PINW",
		  "" };
	int i;

	if (str[0] != '_') return 0;
	for (i = 0; known_suffix[i][0]; i++) {
		if (!strcmp(known_suffix[i], str))
			return 1;
	}
	return 0;
}

static void copy_word(char* buf, const char* s)
{
	int i = 0;
	while (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i]) {
		buf[i] = s[i];
		i++;
	}
	buf[i] = 0;
}

int sort_lines(const void* a, const void* b)
{
	const char* _a, *_b;
	int i, a_i, b_i, a_num, b_num, rc;
	char a_word[1024], b_word[1024];

	_a = a;
	_b = b;

	// search first non-matching character
	for (i = 0; _a[i] && _a[i] == _b[i]; i++);

	// if entire string matches, return 0
	if (!_a[i] && !_b[i]) return 0;

	// if neither of the non-matching characters is a digit, return
	if ((_a[i] < '0' || _a[i] > '9')
	    && (_b[i] < '0' || _b[i] > '9'))
		return _a[i] - _b[i];

	// go back to beginning of numeric section
	// (a and b must be identical going backwards)
	while (i && _a[i-1] >= '0' && _a[i-1] <= '9')
		i--;

	// go forward to first non-digit
	for (a_i = i; _a[a_i] >= '0' && _a[a_i] <= '9'; a_i++ );
	for (b_i = i; _b[b_i] >= '0' && _b[b_i] <= '9'; b_i++ );

	// there must be at least one digit on each side
	if (a_i <= i || b_i <= i) {
		// We move numbers before all other characters.
		if (_a[i] >= '0' && _a[i] <= '9'
		    && (_b[i] < '0' || _b[i] > '9')) return 1;
		if (_b[i] >= '0' && _b[i] <= '9'
		    && (_a[i] < '0' || _a[i] > '9')) return -1;
		return _a[i] - _b[i];
	}

	// for known suffixes, the suffix comes before the number
	copy_word(a_word, &_a[a_i]);
	copy_word(b_word, &_b[b_i]);
	if ((!a_word[0] || is_known_suffix(a_word))
	    && (!b_word[0] || is_known_suffix(b_word))) {
		rc = strcmp(a_word, b_word);
		if (rc) return rc;
	}

	a_num = strtol(&_a[i], 0 /* endptr */, 10);
	b_num = strtol(&_b[i], 0 /* endptr */, 10);
	if (a_num != b_num)
		return a_num - b_num;

	return strcmp(&_a[a_i], &_b[b_i]);
}

int main(int argc, char** argv)
{
	FILE* fp = 0;
	int i;

	if (argc < 2) {
		fprintf(stderr,
			"sort_seq - sort by sequence\n"
			"Usage: %s <data_file>\n", argv[0]);
		goto xout;
	}
	fp = fopen(argv[1], "r");
	if (!fp) {
		fprintf(stderr, "Error opening %s.\n", argv[1]);
		goto xout;
	}
	s_numlines = 0;
	// read 200 lines to beginning of buffer
	while (s_numlines < 200
	       && fgets(s_lines[s_numlines], sizeof(s_lines[0]), fp))
		s_numlines++;
	while (1) {
		// read another 800 lines
		while (s_numlines < 1000
		       && fgets(s_lines[s_numlines], sizeof(s_lines[0]), fp))
			s_numlines++;
		if (!s_numlines) break;
		// sort 1000 lines
		qsort(s_lines, s_numlines, sizeof(s_lines[0]), sort_lines);
		// print first 800 lines
		for (i = 0; i < 800; i++) {
			if (i >= s_numlines) break;
			printf(s_lines[i]);
		}
		// move up last 200 lines to beginning of buffer
		if (s_numlines > i) {
			memmove(s_lines[0], s_lines[i],
				(s_numlines-i)*sizeof(s_lines[0]));
			s_numlines -= i;
		} else
			s_numlines = 0;
	}
	fclose(fp);
	return EXIT_SUCCESS;
xout:
	return EXIT_FAILURE;
}
