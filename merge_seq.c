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

struct line_buf
{
	// buf[0] == 0 signals 'no line'
	char buf[LINE_LENGTH];
	// left_digit_start_o and right_digit_start_o will be -1
	// if left/right is not initialized.
	int left_digit_start_o, left_digit_end_o, left_digit_base;
	int right_digit_start_o, right_digit_end_o, right_digit_base;
	// sequence_size == 0 means no sequence detected, 1 means
	// two members in sequence (e.g. 0:1), etc.
	int sequence_size;
};

static int print_line(const struct line_buf* line)
{
	char buf[LINE_LENGTH];

	if (!line->buf[0]) return 0;
	if (!line->sequence_size || line->left_digit_start_o < 0) {
		printf("%s", line->buf);
		return 0;
	}
	if (line->right_digit_start_o < 0)
		snprintf(buf, sizeof(buf), "%.*s%i:%i%s",
			line->left_digit_start_o,
			line->buf,
			line->left_digit_base,
			line->left_digit_base+line->sequence_size,
			&line->buf[line->left_digit_end_o]);
	else
		snprintf(buf, sizeof(buf), "%.*s%i:%i%.*s%i:%i%s",
			line->left_digit_start_o,
			line->buf,
			line->left_digit_base,
			line->left_digit_base+line->sequence_size,
			line->right_digit_start_o-line->left_digit_end_o,
			&line->buf[line->left_digit_end_o],
			line->right_digit_base,
			line->right_digit_base+line->sequence_size,
			&line->buf[line->right_digit_end_o]);
	printf("%s", buf);
	return 0;
}

// Finds the positions of two non-equal numbers that must meet
// the following two criteria:
// - prefixed by at least one capital 'A'-'Z' or '_'
// - suffixed by matching or empty strings
static void find_non_matching_number(const char* a, int a_len,
	const char* b, int b_len, int* ab_start, int* a_end, int* b_end)
{
	int a_o, b_o, digit_start, a_num, b_num;

	*ab_start = -1;
	a_o = 0;

	// from the left side, search for the first non-matching
	// character
	while (a[a_o] == b[a_o] && a_o < a_len && a_o < b_len)
		a_o++;
	
	// if the strings match entirely, return
	if (a_o >= a_len && a_o >= b_len) return;

	// If neither of the non-matching characters is a digit, return
	if ((a[a_o] < '0' || a[a_o] > '9')
	    && (b[a_o] < '0' || b[a_o] > '9'))
		return;

	// go back to beginning of numeric section
	// (first and second must be identical going backwards)
	while (a_o && a[a_o-1] >= '0' && a[a_o-1] <= '9')
		a_o--;

	// If there is not at least one capital 'A'-'Z' or '_'
	// before the number, return
	if (!a_o
	    || ((a[a_o-1] < 'A' || a[a_o-1] > 'Z')
		&& a[a_o-1] != '_')) return;

	// now skip over all digits in left and right string
	digit_start = a_o;
	while (a[a_o] >= '0' && a[a_o] <= '9' && a_o < a_len)
		a_o++;
	b_o = digit_start;
	while (b[b_o] >= '0' && b[b_o] <= '9' && b_o < b_len)
		b_o++;

	// there must be at least one digit on each side
	if (a_o <= digit_start || b_o <= digit_start) return;

	a_num = to_i(&a[digit_start], a_o-digit_start);
	b_num = to_i(&b[digit_start], b_o-digit_start);
	if (a_num == b_num) {
		fprintf(stderr, "Strange parsing issue with '%.*s' and '%.*s'\n", a_len, a, b_len, b);
		return;
	}

	// the trailing part after the two numbers must match
	if (a_len - a_o != b_len - b_o) return;
	if ((a_len - a_o) && strncmp(&a[a_o], &b[b_o], a_len-a_o)) return;

	// some known suffixes include numbers and must never be
	// part of merging
	if (a_len - a_o == 0) {
		// _S0 _N3
		if (a_o > 3
		    && ((a[a_o-3] == '_' && a[a_o-2] == 'S' && a[a_o-1] == '0')
		        || (a[a_o-3] == '_' && a[a_o-2] == 'N' && a[a_o-1] == '3')))
			return;
		// _INT0 _INT1 _INT2 _INT3
		if (a_o > 5
		    && a[a_o-5] == '_' && a[a_o-4] == 'I' && a[a_o-3] == 'N'
		    && a[a_o-2] == 'T'
		    && a[a_o-1] >= '0' && a[a_o-1] <= '3')
			return;
	}

	*ab_start = digit_start;
	*a_end = a_o;
	*b_end = b_o;
}

static int merge_line(struct line_buf* first_l, struct line_buf* second_l)
{
	int first_o, second_o, fs_start, f_end, s_end, first_num, second_num;
	int first_eow, second_eow, f_start, s_start;
	int left_start, left_end, left_num;

	if (!first_l->buf[0] || !second_l->buf[0]) return 0;
	// go through word by word, find first non-equal word
	first_o = 0;
	second_o = 0;
	while (1) {
		next_word(first_l->buf, first_o, &first_o, &first_eow);
		next_word(second_l->buf, second_o, &second_o, &second_eow);
		if (first_eow <= first_o || second_eow <= second_o) return 0;
		if (first_eow-first_o != second_eow-second_o
		    || strncmp(&first_l->buf[first_o], &second_l->buf[second_o], first_eow-first_o))
			break;
		first_o = first_eow;
		second_o = second_eow;
	}
	// non-matching number inside?
	fs_start = -1;
	find_non_matching_number(&first_l->buf[first_o], first_eow-first_o,
		&second_l->buf[second_o], second_eow-second_o,
		&fs_start, &f_end, &s_end);
	if (fs_start == -1) return 0; // no: cannot merge
	f_start = first_o+fs_start;
	f_end += first_o;
	s_start = second_o+fs_start;
	s_end += second_o;
	first_o = first_eow;
	second_o = second_eow;

	// in sequence? if not, cannot merge
	second_num = to_i(&second_l->buf[s_start], s_end-s_start);
	if (first_l->sequence_size) {
		if (first_l->left_digit_start_o < 0) {
			fprintf(stderr, "Internal error in %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
		// We must be looking at the same digit, for example
		// if we have a sequence SW2M0:3, and now the second
		// line is SW4M0 - the '4' must not be seen as a
		// continuation of the '3'.
		if (s_start != first_l->left_digit_start_o)
			return 0;
		if (second_num != first_l->left_digit_base
					+ first_l->sequence_size + 1)
			return 0;
		first_num = -1; // to suppress compiler warning
	} else {
		first_num = to_i(&first_l->buf[f_start], f_end-f_start);
		if (second_num != first_num + 1)
			return 0;
	}

	// find next non-equal word
	while (1) {
		next_word(first_l->buf, first_o, &first_o, &first_eow);
		next_word(second_l->buf, second_o, &second_o, &second_eow);
		if (first_eow <= first_o && second_eow <= second_o) {
			// reached end of line
			if (first_l->sequence_size) {
				if (first_l->right_digit_start_o != -1) return 0;
				first_l->sequence_size++;
			} else {
				first_l->left_digit_start_o = f_start;
				first_l->left_digit_end_o = f_end;
				first_l->left_digit_base = first_num;
				first_l->right_digit_start_o = -1;
				first_l->sequence_size = 1;
			}
			second_l->buf[0] = 0;
			return 0;
		}
		if (first_eow <= first_o || second_eow <= second_o) return 0;
		if (first_eow-first_o != second_eow-second_o
		    || strncmp(&first_l->buf[first_o], &second_l->buf[second_o], first_eow-first_o))
			break;
		first_o = first_eow;
		second_o = second_eow;
	}

	// now we must find a second number matching the sequence
	left_start = f_start;
	left_end = f_end;
	left_num = first_num;

	// non-matching number inside?
	fs_start = -1;
	find_non_matching_number(&first_l->buf[first_o], first_eow-first_o,
		&second_l->buf[second_o], second_eow-second_o,
		&fs_start, &f_end, &s_end);
	if (fs_start == -1) return 0; // no: cannot merge
	f_start = first_o+fs_start;
	f_end += first_o;
	s_start = second_o+fs_start;
	s_end += second_o;
	first_o = first_eow;
	second_o = second_eow;

	// in sequence? if not, cannot merge
	second_num = to_i(&second_l->buf[s_start], s_end-s_start);
	if (first_l->sequence_size) {
		if (first_l->right_digit_start_o < 0
		    || second_num != first_l->right_digit_base + first_l->sequence_size + 1)
			return 0;
	} else {
		first_num = to_i(&first_l->buf[f_start], f_end-f_start);
		if (second_num != first_num + 1)
			return 0;
	}

	// find next non-equal word
	while (1) {
		next_word(first_l->buf, first_o, &first_o, &first_eow);
		next_word(second_l->buf, second_o, &second_o, &second_eow);
		if (first_eow <= first_o && second_eow <= second_o) {
			// reached end of line
			if (first_l->sequence_size)
				first_l->sequence_size++;
			else {
				first_l->left_digit_start_o = left_start;
				first_l->left_digit_end_o = left_end;
				first_l->left_digit_base = left_num;
				first_l->right_digit_start_o = f_start;
				first_l->right_digit_end_o = f_end;
				first_l->right_digit_base = first_num;
				first_l->sequence_size = 1;
			}
			second_l->buf[0] = 0;
			return 0;
		}
		if (first_eow <= first_o || second_eow <= second_o) return 0;
		if (first_eow-first_o != second_eow-second_o
		    || strncmp(&first_l->buf[first_o], &second_l->buf[second_o], first_eow-first_o))
			break;
		first_o = first_eow;
		second_o = second_eow;
	}
	// found another non-matching word, cannot merge
	return 0;
}

static void read_line(FILE* fp, struct line_buf* line)
{
	*line->buf = 0;
	line->left_digit_start_o = -1;
	line->right_digit_start_o = -1;
	line->sequence_size = 0;
	if (!fgets(line->buf, sizeof(line->buf), fp))
		*line->buf = 0;
}

static void increment(int *off, struct line_buf* lines, int num_lines, int end_of_ringbuf)
{
	if (++(*off) >= num_lines)
		*off = 0;
	while (!lines[*off].buf[0] && *off != end_of_ringbuf) {
		if (++(*off) >= num_lines)
			*off = 0;
	}
}

#define READ_AHEAD_SIZE		100

int main(int argc, char** argv)
{
	struct line_buf read_ahead[READ_AHEAD_SIZE];
	int read_ahead_get, read_ahead_put, second_line, eof_reached, try_count;
	FILE* fp = 0;
	int last_merge_try, rc = -1;

	if (argc < 2) {
		fprintf(stderr,
			"merge_seq - merge sequence (needs presorted file)\n"
			"Usage: %s <data_file> | - for stdin [--try 2]\n", argv[0]);
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
	// how far to look ahead for mergable sequences
	if (argc >= 4 && !strcmp(argv[2], "--try"))
		last_merge_try = atoi(argv[3]);
	else
		last_merge_try = 2;

	read_line(fp, &read_ahead[0]);
	if (!read_ahead[0].buf[0]) goto out;
	read_ahead_get = 0;
	read_ahead_put = 1;
	eof_reached = 0;
	while (1) {
		// fill up read ahead buffer
		while (!eof_reached
		       && read_ahead_put != read_ahead_get) {
			read_line(fp, &read_ahead[read_ahead_put]);
			if (!read_ahead[read_ahead_put].buf[0]) {
				eof_reached = 1;
				break;
			}
			if (++read_ahead_put >= READ_AHEAD_SIZE)
				read_ahead_put = 0;
		}

		// find second line in read ahead buffer
		second_line = read_ahead_get;
		increment(&second_line, read_ahead, READ_AHEAD_SIZE, read_ahead_put);

		if (!read_ahead[second_line].buf[0]) {
			// if no more lines, print first one and exit
			rc = print_line(&read_ahead[read_ahead_get]);
			if (rc) goto xout;
			break;
		}

		try_count = 0;
		while (1) {
			// try to merge
			rc = merge_line(&read_ahead[read_ahead_get], &read_ahead[second_line]);
			if (rc) goto xout;
			if (!read_ahead[second_line].buf[0]) // merge successful
				break;

			// try next one
			increment(&second_line, read_ahead,
				READ_AHEAD_SIZE, read_ahead_put);

			if (second_line == read_ahead_put
			    || ++try_count >= last_merge_try) {
				// read-ahead empty or stop trying
				rc = print_line(&read_ahead[read_ahead_get]);
				if (rc) goto xout;
				read_ahead[read_ahead_get].buf[0] = 0;
				increment(&read_ahead_get, read_ahead,
					READ_AHEAD_SIZE, read_ahead_put);
				break;
			}
		}
	}
out:
	return EXIT_SUCCESS;
xout:
	return rc;
}
