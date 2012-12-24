//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "helper.h"

#define LINE_LENGTH	1024

static int s_numlines;
static char s_lines[1000][LINE_LENGTH];

// returns 0 if no number found
static int find_rightmost_num(const char* s, int s_len,
	int* dig_start, int* dig_end)
{
	int i;

	if (s_len < 2) return 0;
	i = s_len;
	while (i > 0 && (s[i-1] < '0' || s[i-1] > '9'))
		i--;
	if (!i) return 0;
	*dig_end = i;
	while (i > 0 && s[i-1] >= '0' && s[i-1] <= '9')
		i--;
	if (!i) return 0;
	if ((s[i-1] < 'A' || s[i-1] > 'Z') && s[i-1] != '_')
		return 0;
	*dig_start = i;
	return 1;
}

// Finds the position of a number in a string, searching from
// the right, meeting:
// - not part of a known suffix if there is another number to
//   the left of it
// - prefixed by at least one capital 'A'-'Z' or '_'
// If none is found, both *num_start and *num_end will be returned as 0.
static void find_number(const char* s, int s_len, int* num_start, int* num_end)
{
	int result, dig_start, dig_end, found_num, search_more;
	int next_dig_start, next_dig_end;

	*num_start = 0;
	*num_end = 0;

	if (s_len >= 13 && !strncmp("_DSP48A1_SITE", &s[s_len-13], 13))
		s_len -= 13;
	else if (s_len >= 15 && !strncmp("_DSP48A1_B_SITE", &s[s_len-15], 15))
		s_len -= 15;

	result = find_rightmost_num(s, s_len, &dig_start, &dig_end);
	if (!result) return;

	// If the found number is not part of a potential
	// suffix, we can take it.
	found_num = to_i(&s[dig_start], dig_end-dig_start);

	// The remaining suffixes all reach the right end of
	// the string, so if our digits don't, we can take them.
	if (dig_end < s_len) {
		*num_start = dig_start;
		*num_end = dig_end;
		return;
	}
	search_more = 0;
	// _
	if (dig_start >= 2
	    && s[dig_start-1] == '_'
	    && ((s[dig_start-2] >= 'A' && s[dig_start-2] <= 'Z')
	        || (s[dig_start-2] >= '0' && s[dig_start-2] <= '9')))
		search_more = 1;
	// _S0
	else if (found_num == 0 && dig_start >= 3
	    && s[dig_start-1] == 'S' && s[dig_start-2] == '_'
	    && ((s[dig_start-3] >= 'A' && s[dig_start-3] <= 'Z')
	        || (s[dig_start-3] >= '0' && s[dig_start-3] <= '9')))
		search_more = 1;
	// _N3
	else if (found_num == 3 && dig_start >= 3
	    && s[dig_start-1] == 'N' && s[dig_start-2] == '_'
	    && ((s[dig_start-3] >= 'A' && s[dig_start-3] <= 'Z')
	        || (s[dig_start-3] >= '0' && s[dig_start-3] <= '9')))
		search_more = 1;
	// _INT0 _INT1 _INT2 _INT3
	else if ((found_num >= 0 && found_num <= 3) && dig_start >= 5
		 && s[dig_start-1] == 'T' && s[dig_start-2] == 'N'
		 && s[dig_start-3] == 'I' && s[dig_start-4] == '_'
		 && ((s[dig_start-5] >= 'A' && s[dig_start-5] <= 'Z')
		     || (s[dig_start-5] >= '0' && s[dig_start-5] <= '9')))
		search_more = 1;
	if (!search_more
	    || !find_rightmost_num(s, dig_start, &next_dig_start, &next_dig_end)) {
		*num_start = dig_start;
		*num_end = dig_end;
	} else {
		*num_start = next_dig_start;
		*num_end = next_dig_end;
	}
}

static int is_known_suffix(const char* str, int str_len)
{
	int i;

	if (str_len < 1) return 0;
	if (str[0] != '_') return 0;
	if (str_len < 2) return 0;

	// Special case _<digits> - we detect this as a
	// known suffix here because our number finding
	// function already found a better match to the
	// left of it, so we can assume the _<digits> to
	// be a suffix.
	i = 1;
	while (i < str_len) {
		if (str[i] < '0' || str[i] > '9')
			break;
		i++;
	}
	if (i >= str_len)
		return 1;

	if (str_len == 2) {
		// _E _W _S _N _M
		if (str[1] == 'E' || str[1] == 'W' || str[2] == 'S'
		    || str[1] == 'N' || str[1] == 'M')
			return 1;
	}
	if (str_len < 3) return 0;
	if (str_len == 3) {
		// _S0 _N3 _UP
		if ((str[1] == 'S' && str[2] == '0')
		    || (str[1] == 'N' && str[2] == '3')
		    || (str[1] == 'U' && str[2] == 'P'))
			return 1;
	}
	if (str_len < 4) return 0;
	if (str_len == 4) {
		// _CLB _DSP _EXT _INT _MCP _BRK _BUF
		if ((str[1] == 'C' && str[2] == 'L' && str[3] == 'B')
		    || (str[1] == 'D' && str[2] == 'S' && str[3] == 'P')
		    || (str[1] == 'E' && str[2] == 'X' && str[3] == 'T')
		    || (str[1] == 'I' && str[2] == 'N' && str[3] == 'T')
		    || (str[1] == 'M' && str[2] == 'C' && str[3] == 'B')
		    || (str[1] == 'B' && str[2] == 'R' && str[3] == 'K')
		    || (str[1] == 'B' && str[2] == 'U' && str[3] == 'F'))
			return 1;
	}
	if (str_len < 5) return 0;
	if (str_len == 5) {
		// _INT0 _INT1 _INT2 _INT3 _TEST _FOLD _BRAM _DOWN _PINW
		if ((str[1] == 'I' && str[2] == 'N' && str[3] == 'T'
		     && str[4] >= '0' && str[4] <= '3')
		    || (str[1] == 'T' && str[2] == 'E' && str[3] == 'S'
			&& str[4] == 'T')
		    || (str[1] == 'F' && str[2] == 'O' && str[3] == 'L'
			&& str[4] == 'D')
		    || (str[1] == 'B' && str[2] == 'R' && str[3] == 'A'
			&& str[4] == 'M')
		    || (str[1] == 'D' && str[2] == 'O' && str[3] == 'W'
			&& str[4] == 'N')
		    || (str[1] == 'P' && str[2] == 'I' && str[3] == 'N'
			&& str[4] == 'W'))
			return 1;
	}
	if (str_len < 11) return 0;
	if (str_len == 11) {
		// _BRAM_INTER
		if (str[1] == 'B' && str[2] == 'R' && str[3] == 'A'
		    && str[4] == 'M' && str[5] == '_' && str[6] == 'I'
		    && str[7] == 'N' && str[8] == 'T' && str[9] == 'E'
		    && str[10] == 'R')
			return 1;
	}
	return 0;
}

static void next_unequal_word(
	const char* a, int a_start, int* a_beg, int* a_end,
	const char* b, int b_start, int* b_beg, int* b_end)
{
	*a_beg = a_start;
	*a_end = a_start;
	*b_beg = b_start;
	*b_end = b_start;

	// find the first non-matching word
	while (1) {
		next_word(a, *a_beg, a_beg, a_end);
		next_word(b, *b_beg, b_beg, b_end);

		if (*a_end-*a_beg <= 0
		    || *b_end-*b_beg <= 0)
			return;

		if (str_cmp(&a[*a_beg], *a_end-*a_beg, &b[*b_beg], *b_end-*b_beg))
			return;
		*a_beg = *a_end;
		*b_beg = *b_end;
	}
}

static int sort_lines(const void* a, const void* b)
{
	const char* _a, *_b;
	int a_word_beg, a_word_end, b_word_beg, b_word_end;
	int a_num, b_num, a_num_start, b_num_start, a_num_end, b_num_end;
	int num_result, result, suffix_result;

	_a = a;
	_b = b;

	// find the first non-matching word
	a_word_beg = 0;
	b_word_beg = 0;
	next_unequal_word(_a, a_word_beg, &a_word_beg, &a_word_end,
		_b, b_word_beg, &b_word_beg, &b_word_end);

	if (a_word_end-a_word_beg <= 0) {
		if (b_word_end-b_word_beg <= 0)
			return 0;
		return -1;
	}
	if (b_word_end-b_word_beg <= 0) {
		if (a_word_end-a_word_beg <= 0)
			return 0;
		return 1;
	}

	// first try to find 2 numbers
	find_number(&_a[a_word_beg], a_word_end-a_word_beg,
		&a_num_start, &a_num_end);
	find_number(&_b[b_word_beg], b_word_end-b_word_beg,
		&b_num_start, &b_num_end);

	// if we cannot find both numbers, return a regular
	// string comparison over the entire word
	if (a_num_end <= a_num_start
	    || b_num_end <= b_num_start) {
		result = str_cmp(&_a[a_word_beg], a_word_end-a_word_beg,
			&_b[b_word_beg], b_word_end-b_word_beg);
		if (!result) {
			fprintf(stderr, "Internal error in %s:%i\n",
				__FILE__, __LINE__);
			exit(0);
		}
		return result;
	}
	// A number must always be prefixed by at least one character.
	if (!a_num_start || !b_num_start) {
		fprintf(stderr, "Internal error in %s:%i\n",
			__FILE__, __LINE__);
		exit(0);
	}
	// otherwise compare the string up to the 2 numbers,
	// if it does not match return that result
	result = str_cmp(&_a[a_word_beg], a_num_start,
		&_b[b_word_beg], b_num_start);
	if (result)
		return result;

	a_num_start += a_word_beg;
	a_num_end += a_word_beg;
	b_num_start += b_word_beg;
	b_num_end += b_word_beg;
	if (a_num_end > a_word_end
	    || b_num_end > b_word_end) {
		fprintf(stderr, "Internal error in %s:%i\n",
			__FILE__, __LINE__);
		fprintf(stderr, "sort_line_a: %s", _a);
		fprintf(stderr, "sort_line_b: %s", _b);
		exit(1);
	}
	if ((a_word_end-a_num_end == 0
	     || is_known_suffix(&_a[a_num_end],
		a_word_end-a_num_end))
	    && (b_word_end-b_num_end == 0
	     || is_known_suffix(&_b[b_num_end],
		b_word_end-b_num_end))) {
		// known suffix comes before number
		suffix_result = str_cmp(&_a[a_num_end],
			a_word_end-a_num_end,
			&_b[b_num_end], b_word_end-b_num_end);
		if (suffix_result)
			return suffix_result;
	}

	a_num = to_i(&_a[a_num_start], a_num_end-a_num_start);
	b_num = to_i(&_b[b_num_start], b_num_end-b_num_start);
	num_result = a_num-b_num;

	// if the non-known suffixes don't match, return numeric result
	// if numbers are not equal, otherwise suffix result
	suffix_result = str_cmp(&_a[a_num_end], a_word_end-a_num_end,
		&_b[b_num_end], b_word_end-b_num_end);
	if (suffix_result) {
		if (num_result) return num_result;
		return suffix_result;
	}
	// Should be impossible that both the number result and
	// suffix result are equal. How can the entire word then
	// be unequal?
	if (!num_result) {
		fprintf(stderr, "Internal error in %s:%i\n",
			__FILE__, __LINE__);
		fprintf(stderr, "sort_line_a: %s", _a);
		fprintf(stderr, "sort_line_b: %s", _b);
		exit(1);
	}

	// find second non-equal word
	next_unequal_word(_a, a_word_end, &a_word_beg, &a_word_end,
		_b, b_word_end, &b_word_beg, &b_word_end);
	if (a_word_end <= a_word_beg
	    || b_word_end <= b_word_beg)
		return num_result;
	
	// if no numbers in second non-equal words, fall back
	// to numeric result of first word
	find_number(&_a[a_word_beg], a_word_end-a_word_beg,
		&a_num_start, &a_num_end);
	find_number(&_b[b_word_beg], b_word_end-b_word_beg,
		&b_num_start, &b_num_end);
	if (a_num_end <= a_num_start
	    || b_num_end <= b_num_start)
		return num_result;
	// A number must always be prefixed by at least one character.
	if (!a_num_start || !b_num_start) {
		fprintf(stderr, "Internal error in %s:%i\n",
			__FILE__, __LINE__);
		exit(0);
	}
	// If the prefix string of the second word does not
	// match, fall back to numeric result of first word.
	result = str_cmp(&_a[a_word_beg], a_num_start,
		&_b[b_word_beg], b_num_start);
	if (result)
		return num_result;
	a_num_start += a_word_beg;
	a_num_end += a_word_beg;
	b_num_start += b_word_beg;
	b_num_end += b_word_beg;
	if (a_num_end > a_word_end
	    || b_num_end > b_word_end) {
		fprintf(stderr, "Internal error in %s:%i\n",
			__FILE__, __LINE__);
		exit(0);
	}
	// if there are known suffixes in second non-equal
	// words, compare those first
	if ((a_word_end-a_num_end == 0
	     || is_known_suffix(&_a[a_num_end],
		a_word_end-a_num_end))
	    && (b_word_end-b_num_end == 0
	     || is_known_suffix(&_b[b_num_end],
		b_word_end-b_num_end))) {
		// known suffix comes before number
		suffix_result = str_cmp(&_a[a_num_end],
			a_word_end-a_num_end,
			&_b[b_num_end], b_word_end-b_num_end);
		if (suffix_result)
			return suffix_result;
	}
	// otherwise fall back to numeric result of first word
	return num_result;
}

int main(int argc, char** argv)
{
	FILE* fp = 0;
	int i;

	if (argc < 2) {
		fprintf(stderr,
			"sort_seq - sort by sequence\n"
			"Usage: %s <data_file> | - for stdin\n", argv[0]);
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
	if(fp) fclose(fp);
	return EXIT_FAILURE;
}
