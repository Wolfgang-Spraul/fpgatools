//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include "helper.h"
#include "parts.h"

const char* bitstr(uint32_t value, int digits)
{
        static char str[2 /* "0b" */ + 32 + 1 /* '\0' */];
        int i;

        str[0] = '0';
        str[1] = 'b';
        for (i = 0; i < digits; i++)
                str[digits-i+1] = (value & (1<<i)) ? '1' : '0';
        str[digits+2] = 0;
        return str;
}

void hexdump(int indent, const uint8_t* data, int len)
{
	int i, j;
	char fmt_str[16] = "%s@%05x %02x";
	char indent_str[16];

	if (indent > 15)
		indent = 15;
	for (i = 0; i < indent; i++)
		indent_str[i] = ' ';
	indent_str[i] = 0;

	i = 0;
	if (len <= 0x100)
		fmt_str[5] = '2';
	else if (len <= 0x10000)
		fmt_str[5] = '4';
	else
		fmt_str[5] = '6';

	while (i < len) {
		printf(fmt_str, indent_str, i, data[i]);
		for (j = 1; (j < 8) && (i + j < len); j++) {
			if (i + j >= len) break;
			printf(" %02x", data[i+j]);
		}
		printf("\n");
		i += 8;
	}
}

uint16_t __swab16(uint16_t x)
{
        return (((x & 0x00ffU) << 8) | \
            ((x & 0xff00U) >> 8)); \
}

uint32_t __swab32(uint32_t x)
{
        return (((x & 0x000000ffUL) << 24) | \
            ((x & 0x0000ff00UL) << 8) | \
            ((x & 0x00ff0000UL) >> 8) | \
            ((x & 0xff000000UL) >> 24)); \
}

int atom_found(char* bits, const cfg_atom_t* atom)
{
	int i;
	for (i = 0; atom->must_0[i] != -1; i++)
		if (bits[atom->must_0[i]])
			break;
	if (atom->must_0[i] != -1)
		return 0;
	for (i = 0; atom->must_1[i] != -1; i++)
		if (!bits[atom->must_1[i]])
			break;
	return atom->must_1[i] == -1;
}

void atom_remove(char* bits, const cfg_atom_t* atom)
{
	int i;
	for (i = 0; atom->must_1[i] != -1; i++) {
		if (bits[atom->must_1[i]])
			bits[atom->must_1[i]] = 0;
	}
}

// for an equivalent schematic, see lut.svg
const int lut_base_vars[6] = {0 /* A1 */, 1, 0 /* A3 - not used */,
				0, 0, 1 /* A6 */};

static int bool_nextlen(const char* expr, int len)
{
	int i, depth;

	if (!len) return -1;
	i = 0;
	if (expr[i] == '~') {
		i++;
		if (i >= len) return -1;
	}
	if (expr[i] == '(') {
		if (i+2 >= len) return -1;
		i++;
		for (depth = 1; depth && i < len; i++) {
			if (expr[i] == '(')
				depth++;
			else if (expr[i] == ')')
				depth--;
		}
		if (depth) return -1;
		return i;
	}
	if (expr[i] == 'A') {
		i++;
		if (i >= len) return -1;
		if (expr[i] < '1' || expr[i] > '6') return -1;
		return i+1;
	}
	return -1;
}

// + or, * and, @ xor, ~ not
// var must point to array of A1..A6 variables
static int bool_eval(const char* expr, int len, const int* var)
{
	int i, negate, result, oplen;

	oplen = bool_nextlen(expr, len);
	if (oplen < 1) goto fail;
	i = 0;
	negate = 0;
	if (expr[i] == '~') {
		negate = 1;
		if (++i >= oplen) goto fail;
	}
	if (expr[i] == '(') {
		if (i+2 >= oplen) goto fail;
		result = bool_eval(&expr[i+1], oplen-i-2, var);
		if (result == -1) goto fail;
	} else if (expr[i] == 'A') {
		if (i+1 >= oplen) goto fail;
		if (expr[i+1] < '1' || expr[i+1] > '6')
			goto fail;
		result = var[expr[i+1]-'1'];
		if (oplen != i+2) goto fail;
	} else goto fail;
	if (negate) result = !result;
	i = oplen;
	while (i < len) {
		if (expr[i] == '+') {
			if (result) return 1;
			return bool_eval(&expr[i+1], len-i-1, var);
		}
		if (expr[i] == '@') {
			int right_side = bool_eval(&expr[i+1], len-i-1, var);
			if (right_side == -1) goto fail;
			return (result && !right_side) || (!result && right_side);
		}
		if (expr[i] != '*') goto fail;
		if (!result) break;
		if (++i >= len) goto fail;

		oplen = bool_nextlen(&expr[i], len-i);
		if (oplen < 1) goto fail;
		result = bool_eval(&expr[i], oplen, var);
		if (result == -1) goto fail;
		i += oplen;
	}
	return result;
fail:
	return -1;
}

static int parse_boolexpr(const char* expr, uint64_t* lut)
{
	int i, j, result, vars[6];

	*lut = 0;
	for (i = 0; i < 64; i++) {
		memcpy(vars, lut_base_vars, sizeof(vars));
		for (j = 0; j < 6; j++) {
			if (j != 2 && (i & (1<<j)))
				vars[j] = !vars[j];
		}
		if (((i&8) != 0) ^ ((i&4) != 0))
			vars[2] = 1;
		// todo: flip_b0 and different base values missing
		result = bool_eval(expr, strlen(expr), vars);
		if (result == -1) return -1;
		if (result) *lut |= 1LL<<i;
	}
	return 0;
}

void printf_lut6(const char* cfg)
{
	uint64_t lut;
	uint32_t first_major, second_major;
	int i;

	first_major = 0;
	second_major = 0;
	// todo: this is missing the different base_values, flip_b0 etc.
	parse_boolexpr(cfg, &lut);

	for (i = 0; i < 16; i++) {
		if (lut & (1LL<<(i*4)))
			first_major |= 1<<(i*2);
		if (lut & (1LL<<(i*4+1)))
			first_major |= 1<<(i*2+1);
		if (lut & (1LL<<(i*4+2)))
			second_major |= 1<<(i*2);
		if (lut & (1LL<<(i*4+3)))
			second_major |= 1<<(i*2+1);
	}
	printf("first_major 0x%X second_major 0x%X\n", first_major, second_major);
}

typedef struct _minterm_entry
{
	char a[6]; // 0=A1, 5=A6. value can be 0, 1 or 2 for 'removed'
	int merged;
} minterm_entry;

// bits is tested only for 32 and 64
const char* lut2bool(const uint64_t lut, int bits,
	int (*logic_base)[6], int flip_b0)
{
	// round 0 needs 64 entries
	// round 1 (size2): 192
	// round 2 (size4): 240
	// round 3 (size8): 160
	// round 4 (size16): 60
	// round 5 (size32): 12
	// round 6 (size64): 1
	minterm_entry mt[7][256];
	int mt_size[7];
	int i, j, k, round, only_diff_bit;
	int str_end, first_op;
	static char str[2048];

	memset(mt, 0, sizeof(mt));
	memset(mt_size, 0, sizeof(mt_size));

	for (i = 0; i < bits; i++) {
		if (lut & (1LL<<i)) {
			mt[0][mt_size[0]].a[0] = (*logic_base)[0];
			mt[0][mt_size[0]].a[1] = (*logic_base)[1];
			mt[0][mt_size[0]].a[2] = (*logic_base)[2];
			mt[0][mt_size[0]].a[3] = (*logic_base)[3];
			mt[0][mt_size[0]].a[4] = (*logic_base)[4];
			mt[0][mt_size[0]].a[5] = (*logic_base)[5];
			for (j = 0; j < 6; j++) {
				if (j != 2 && (i&(1<<j)))
					mt[0][mt_size[0]].a[j]
						= !mt[0][mt_size[0]].a[j];
			}
			if (((i&8) != 0) ^ ((i&4) != 0))
				mt[0][mt_size[0]].a[2] = 1;
			if (flip_b0
			   && (mt[0][mt_size[0]].a[2] ^ mt[0][mt_size[0]].a[3]))
			  mt[0][mt_size[0]].a[0] = !mt[0][mt_size[0]].a[0];
			mt_size[0]++;
		}
	}

	// special case: no minterms -> empty string
	if (mt_size[0] == 0) {
		str[0] = 0;
		return str;
	}

	// go through five rounds of merging
	for (round = 1; round < 7; round++) {
		for (i = 0; i < mt_size[round-1]; i++) {
			for (j = i+1; j < mt_size[round-1]; j++) {
				only_diff_bit = -1;
				for (k = 0; k < 6; k++) {
					if (mt[round-1][i].a[k] != mt[round-1][j].a[k]) {
						if (only_diff_bit != -1) {
							only_diff_bit = -1;
							break;
						}
						only_diff_bit = k;
					}
				}
				if (only_diff_bit != -1) {
					char new_term[6];
	
					for (k = 0; k < 6; k++)
						new_term[k] =
						  (k == only_diff_bit) ? 2 
						    : mt[round-1][i].a[k];
					for (k = 0; k < mt_size[round]; k++) {
						if (new_term[0] == mt[round][k].a[0]
						    && new_term[1] == mt[round][k].a[1]
						    && new_term[2] == mt[round][k].a[2]
						    && new_term[3] == mt[round][k].a[3]
						    && new_term[4] == mt[round][k].a[4]
						    && new_term[5] == mt[round][k].a[5])
							break;
					}
					if (k >= mt_size[round]) {
						mt[round][mt_size[round]].a[0] = new_term[0];
						mt[round][mt_size[round]].a[1] = new_term[1];
						mt[round][mt_size[round]].a[2] = new_term[2];
						mt[round][mt_size[round]].a[3] = new_term[3];
						mt[round][mt_size[round]].a[4] = new_term[4];
						mt[round][mt_size[round]].a[5] = new_term[5];
						mt_size[round]++;
					}
					mt[round-1][i].merged = 1;
					mt[round-1][j].merged = 1;
				}
			}
		}
	}
	// special case: 222222 -> (A6+~A6)
	for (i = 0; i < mt_size[6]; i++) {
		if (mt[6][i].a[0] == 2
		    && mt[6][i].a[1] == 2
		    && mt[6][i].a[2] == 2
		    && mt[6][i].a[3] == 2
		    && mt[6][i].a[4] == 2
		    && mt[6][i].a[5] == 2) {
			strcpy(str, "A6+~A6");
			return str;
		}
	}

	str_end = 0;
	for (round = 0; round < 7; round++) {
		for (i = 0; i < mt_size[round]; i++) {
			if (!mt[round][i].merged) {
				if (str_end)
					str[str_end++] = '+';
				first_op = 1;
				for (j = 0; j < 6; j++) {
					if (mt[round][i].a[j] != 2) {
						if (!first_op)
							str[str_end++] = '*';
						if (!mt[round][i].a[j])
							str[str_end++] = '~';
						str[str_end++] = 'A';
						str[str_end++] = '1' + j;
						first_op = 0;
					}
				}
			}
		}
	}
	str[str_end] = 0;
	// TODO: This could be further simplified, see Petrick's method.
	// XOR don't simplify well, try A2@A3
	return str;
}

int printf_iob(uint8_t* d, int len, int inpos, int num_entries)
{
	int i, j, num_printed;

	num_printed = 0;
	for (i = 0; i < num_entries; i++) {
		if (*(uint32_t*)&d[inpos+i*8] || *(uint32_t*)&d[inpos+i*8+4]) {
			printf("iob i%i", i);
			for (j = 0; j < 8; j++)
				printf(" %02X", d[inpos+i*8+j]);
			printf("\n");
			num_printed++;
		}
	}
	return num_printed;
}

void printf_ramb16_data(uint8_t* bits, int inpos)
{
	int nonzero_head, nonzero_tail;
	uint8_t init_byte;
	char init_str[65];
	int i, j, k, bit_off;

	// check head and tail
	nonzero_head = 0;
	for (i = 0; i < 18; i++) {
		if (bits[inpos + i]) {
			nonzero_head = 1;
			break;
		}
	}
	nonzero_tail = 0;
	for (i = 0; i < 18; i++) {
		if (bits[inpos + 18*130-18 + i]) {
			nonzero_tail = 1;
			break;
		}
	}
	if (nonzero_head || nonzero_tail)
		printf(" #W Unexpected data.\n");
	if (nonzero_head) {
		printf(" head");
		for (i = 0; i < 18; i++)
			printf(" %02X", bits[inpos + i]);
		printf("\n");
	}
	if (nonzero_tail) {
		printf(" tail");
		for (i = 0; i < 18; i++)
			printf(" %02X", bits[inpos + 18*130-18 + i]);
		printf("\n");
	}

	// 8 parity configs
	for (i = 0; i < 8; i++) {
		// 32 bytes per config
		for (j = 0; j < 32; j++) {
			init_byte = 0;
			for (k = 0; k < 8; k++) {
				bit_off = (i*(2048+256)) + (31-j)*4*18;
				bit_off += 1+(k/2)*18-(k&1);
				if (bits[inpos+18+bit_off/8]
				    & (1<<(7-(bit_off%8))))
					init_byte |= 1<<k;
			}
			sprintf(&init_str[j*2], "%02X", init_byte);
		}
		for (j = 0; j < 64; j++) {
			if (init_str[j] != '0') {
				printf(" parity 0x%02X \"%s\"\n", i, init_str);
				break;
			}
		}
	}

	for (i = 0; i < 64; i++) {
		// 32 bytes per config
		for (j = 0; j < 32; j++) {
			init_byte = 0;
			for (k = 0; k < 8; k++) {
				bit_off = (i*(256+32)) + ((31-j)/2)*18;
				bit_off += (8-((31-j)&1)*8) + 2 + k;
				if (bits[inpos+18+bit_off/8]
				    & (1<<(7-(bit_off%8))))
					init_byte |= 1<<(7-k);
			}
			sprintf(&init_str[j*2], "%02X", init_byte);
		}
		for (j = 0; j < 64; j++) {
			if (init_str[j] != '0') {
				printf(" init 0x%02X \"%s\"\n", i, init_str);
				break;
			}
		}
	}
}

int is_empty(uint8_t* d, int l)
{
	while (--l >= 0)
		if (d[l]) return 0;
	return 1;
}

int count_bits(uint8_t* d, int l)
{
	int bits = 0;
	while (--l >= 0) {
		if (d[l] & 0x01) bits++;
		if (d[l] & 0x02) bits++;
		if (d[l] & 0x04) bits++;
		if (d[l] & 0x08) bits++;
		if (d[l] & 0x10) bits++;
		if (d[l] & 0x20) bits++;
		if (d[l] & 0x40) bits++;
		if (d[l] & 0x80) bits++;
	}
	return bits;
}

int frame_get_bit(uint8_t* frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	return (frame_d[(bit/16)*2 + !((bit/8)%2)] & v) != 0;
}

void frame_clear_bit(uint8_t* frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	frame_d[(bit/16)*2 + !((bit/8)%2)] &= ~v;
}

void frame_set_bit(uint8_t* frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	frame_d[(bit/16)*2 + !((bit/8)%2)] |= v;
}

uint8_t frame_get_u8(uint8_t* frame_d)
{
	uint8_t v = 0;
	int i;
	for (i = 0; i < 8; i++)
		if (*frame_d & (1<<i)) v |= 1 << (7-i);
	return v;
}

uint16_t frame_get_u16(uint8_t* frame_d)
{
	uint16_t high_b, low_b;
	high_b = frame_get_u8(frame_d);
	low_b = frame_get_u8(frame_d+1);
	return (high_b << 8) | low_b;
}

uint32_t frame_get_u32(uint8_t* frame_d)
{
	uint32_t high_w, low_w;
	low_w = frame_get_u16(frame_d);
	high_w = frame_get_u16(frame_d+2);
	return (high_w << 16) | low_w;
}

uint64_t frame_get_u64(uint8_t* frame_d)
{
	uint64_t high_w, low_w;
	low_w = frame_get_u32(frame_d);
	high_w = frame_get_u32(frame_d+4);
	return (high_w << 32) | low_w;
}

int printf_frames(uint8_t* bits, int max_frames,
	int row, int major, int minor, int print_empty)
{
	int i, i_without_clk;
	char prefix[128], suffix[128];

	if (row < 0)
		sprintf(prefix, "f%i ", abs(row));
	else
		sprintf(prefix, "r%i ma%i mi%i ", row, major, minor);

	if (is_empty(bits, 130)) {
		for (i = 1; i < max_frames; i++) {
			if (!is_empty(&bits[i*130], 130))
				break;
		}
		if (print_empty) {
			if (i > 1)
				printf("%s- *%i\n", prefix, i);
			else
				printf("%s-\n", prefix);
		}
		return i;
	}
	if (count_bits(bits, 130) <= 32) {
		for (i = 0; i < FRAME_SIZE*8; i++) {
			if (!frame_get_bit(bits, i)) continue;
			if (i >= 512 && i < 528) { // hclk
				printf("%sbit %i\n", prefix, i);
				continue;
			}
			i_without_clk = i;
			if (i_without_clk >= 528)
				i_without_clk -= 16;
			snprintf(suffix, sizeof(suffix), "64*%i+%i 256*%i+%i", 
				i_without_clk/64, i_without_clk%64,
				i_without_clk/256, i_without_clk%256);
			printf("%sbit %i %s\n", prefix, i, suffix);
		}
		return 1;
	}
	printf("%shex\n", prefix);
	printf("{\n");
	hexdump(1, bits, 130);
	printf("}\n");
	return 1;
}

void printf_clock(uint8_t* frame, int row, int major, int minor)
{
	int i;
	for (i = 0; i < 16; i++) {
		if (frame_get_bit(frame, 512 + i))
			printf("r%i ma%i mi%i clock %i\n",
				row, major, minor, i);
	}
}

int clb_empty(uint8_t* maj_bits, int idx)
{
	int byte_off, i, minor;

	byte_off = idx * 8;
	if (idx >= 8) byte_off += 2;

	for (i = 0; i < 16; i++) {
		for (minor = 20; minor < 31; minor++) {
			if (!is_empty(&maj_bits[minor*130 + byte_off], 8))
				return 0;
		}
	}
	return 1;
}

void printf_extrabits(uint8_t* maj_bits, int start_minor, int num_minors,
	int start_bit, int num_bits, int row, int major)
{
	int minor, bit;

	for (minor = start_minor; minor < start_minor + num_minors; minor++) {
		for (bit = start_bit; bit < start_bit + num_bits; bit++) {
			if (frame_get_bit(&maj_bits[minor*130], bit))
				printf("r%i ma%i extra mi%i bit %i\n",
					row, major, minor, bit);
		}
	}
}

uint64_t read_lut64(uint8_t* two_minors, int off_in_frame)
{
	uint64_t lut64 = 0;
	int j;

	for (j = 0; j < 16; j++) {
		if (frame_get_bit(two_minors, off_in_frame+j*2))
			lut64 |= 1LL << (j*4);
		if (frame_get_bit(two_minors, off_in_frame+(j*2)+1))
			lut64 |= 1LL << (j*4+1);
		if (frame_get_bit(&two_minors[130], off_in_frame+j*2))
			lut64 |= 1LL << (j*4+2);
		if (frame_get_bit(&two_minors[130], off_in_frame+(j*2)+1))
			lut64 |= 1LL << (j*4+3);
	}
	return lut64;
}

int get_vm_mb(void)
{
	FILE* statusf = fopen("/proc/self/status", "r");
	char line[1024];
	int vm_size = 0;
	while (fgets(line, sizeof(line), statusf)) {
		if (sscanf(line, "VmSize: %i kB", &vm_size) == 1)
			break;
	}
	fclose(statusf);
	if (!vm_size) return 0;
	return (vm_size+1023)/1024;
}

int get_random(void)
{
	int random_f, random_num;
	random_f = open("/dev/urandom", O_RDONLY);
	read(random_f, &random_num, sizeof(random_num));
	close(random_f);
	return random_num;
}

int compare_with_number(const char* a, const char* b)
{
	int i, a_i, b_i, non_numeric_result, a_num, b_num;

	for (i = 0; a[i] && (a[i] == b[i]); i++);
	if (a[i] == b[i]) {
		if (a[i]) fprintf(stderr, "Internal error in line %i\n", __LINE__);
		return 0;
	}
	non_numeric_result = a[i] - b[i];

	// go back to beginning of numeric section
	while (i && a[i-1] >= '0' && a[i-1] <= '9')
		i--;

	// go forward to first non-digit
	for (a_i = i; a[a_i] >= '0' && a[a_i] <= '9'; a_i++ );
	for (b_i = i; b[b_i] >= '0' && b[b_i] <= '9'; b_i++ );

	// There must be at least one digit in both a and b
	// and the suffix must match.
	if (a_i <= i || b_i <= i
	    || strcmp(&a[a_i], &b[b_i]))
		return non_numeric_result;

	a_num = strtol(&a[i], 0 /* endptr */, 10);
	b_num = strtol(&b[i], 0 /* endptr */, 10);
	return a_num - b_num;
}

void next_word(const char*s, int start, int* beg, int* end)
{
	int i = start;
	while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n') i++;
	*beg = i;
	while (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i]) i++;
	*end = i;
}

int str_cmp(const char* a, int a_len, const char* b, int b_len)
{
	int i = 0;

	if (a_len == -1) {
		a_len = 0;
		while (a[a_len]) a_len++;
	}
	if (b_len == -1) {
		b_len = 0;
		while (b[b_len]) b_len++;
	}
	while (1) {
		if (i >= a_len) {
			if (i >= b_len)
				return 0;
			return -1;
		}
		if (i >= b_len) {
			if (i >= a_len)
				return 0;
			return 1;
		}
		if (a[i] != b[i])
			return a[i] - b[i];
		i++;
	}
}

int all_digits(const char* a, int len)
{
	int i;

	if (!len) return 0;
	for (i = 0; i < len; i++) {
		if (a[i] < '0' || a[i] > '9')
			return 0;
	}
	return 1;
}

int to_i(const char* s, int len)
{
	int num, base;
	for (base = 1, num = 0; len; num += base*(s[--len]-'0'), base *= 10);
	return num;
}

void printf_wrap(FILE* f, char* line, int prefix_len,
	const char* fmt, ...)
{
	va_list list;
	int i;

	va_start(list, fmt);
	i = strlen(line);
	if (i >= 80) {
		line[i] = '\n';
		line[i+1] = 0;
		fprintf(f, line);
		line[prefix_len] = 0;
		i = prefix_len;
	}	
	line[i] = ' ';
	vsnprintf(&line[i+1], 256, fmt, list);
	va_end(list);
}

// Dan Bernstein's hash function
uint32_t hash_djb2(const unsigned char* str)
{
	uint32_t hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        return hash;
}

//
// The format of each entry in a bin is.
//   uint32_t idx
//   uint16_t entry len including 4-byte header
//   char[]   zero-terminated string
//
// Offsets point to the zero-terminated string, so the len
// is at off-2, the index at off-6. offset0 can thus be
// used as a special value to signal 'no entry'.
//

#define BIN_STR_HEADER	(4+2)
#define BIN_MIN_OFFSET	BIN_STR_HEADER
#define BIN_INCREMENT	32768

const char* strarray_lookup(struct hashed_strarray* array, int idx)
{
	int bin, offset;

	if (!array->index_to_bin || !array->bin_offsets || idx==STRIDX_NO_ENTRY)
		return 0;

	bin = array->index_to_bin[idx-1];
	offset = array->bin_offsets[idx-1];

	// bin 0 offset 0 is a special value that signals 'no
	// entry'. Normal offsets cannot be less than BIN_MIN_OFFSET.
	if (!bin && !offset) return 0;

	if (!array->bin_strings[bin] || offset >= array->bin_len[bin]
	    || offset < BIN_MIN_OFFSET) {
		// This really should never happen and is an internal error.
		fprintf(stderr, "Internal error.\n");
		return 0;
	}

	return &array->bin_strings[bin][offset];
}

int strarray_find(struct hashed_strarray* array, const char* str, int* idx)
{
	int bin, search_off, i;
	uint32_t hash;

	hash = hash_djb2((const unsigned char*) str);
	bin = hash % array->num_bins;
	// iterate over strings in bin to find match
	if (array->bin_strings[bin]) {
		search_off = BIN_MIN_OFFSET;
		while (search_off < array->bin_len[bin]) {
			if (!strcmp(&array->bin_strings[bin][search_off], str)) {
				i = *(uint32_t*)&array->bin_strings[bin][search_off-6];
				if (!i) {
					fprintf(stderr, "Internal error - index 0.\n");
					return -1;
				}
				*idx = i+1;
				return 0;
			}
			search_off += *(uint16_t*)&array->bin_strings[bin][search_off-2];
		}
	}
	*idx = STRIDX_NO_ENTRY;
	return 0;
}

int s_stash_at_bin(struct hashed_strarray* array, const char* str, int idx, int bin);

int strarray_add(struct hashed_strarray* array, const char* str, int* idx)
{
	int bin, i, free_index, rc, start_index;
	unsigned long hash;

	rc = strarray_find(array, str, idx);
	if (rc) return rc;
	if (*idx != STRIDX_NO_ENTRY) return 0;

	hash = hash_djb2((const unsigned char*) str);

	// search free index
	start_index = hash % array->highest_index;
	for (i = 0; i < array->highest_index; i++) {
		int cur_i = (start_index+i)%array->highest_index;
		if (!cur_i) // never issue index 0
			continue;
		if (!array->bin_offsets[cur_i])
			break;
	}
	if (i >= array->highest_index) {
		fprintf(stderr, "All array indices full.\n");
		return -1;
	}
	free_index = (start_index+i)%array->highest_index;
	bin = hash % array->num_bins;
	rc = s_stash_at_bin(array, str, free_index, bin);
	if (rc) return rc;
	*idx = free_index + 1;
	return 0;
}

int s_stash_at_bin(struct hashed_strarray* array, const char* str, int idx, int bin)
{
	int str_len = strlen(str);
	// check whether bin needs expansion
	if (!(array->bin_len[bin]%BIN_INCREMENT)
	    || array->bin_len[bin]%BIN_INCREMENT + BIN_STR_HEADER+str_len+1 > BIN_INCREMENT)
	{
		int new_alloclen;
		void* new_ptr;
		new_alloclen = ((array->bin_len[bin]
				+ BIN_STR_HEADER+str_len+1)/BIN_INCREMENT + 1)
			  * BIN_INCREMENT;
		new_ptr = realloc(array->bin_strings[bin], new_alloclen);
		if (!new_ptr) {
			fprintf(stderr, "Out of memory.\n");
			return -1;
		}
		if (new_alloclen > array->bin_len[bin])
			memset(new_ptr+array->bin_len[bin], 0, new_alloclen-array->bin_len[bin]);
		array->bin_strings[bin] = new_ptr;
	}
	// append new string at end of bin
	*(uint32_t*)&array->bin_strings[bin][array->bin_len[bin]] = idx;
	*(uint16_t*)&array->bin_strings[bin][array->bin_len[bin]+4] = BIN_STR_HEADER+str_len+1;
	strcpy(&array->bin_strings[bin][array->bin_len[bin]+BIN_STR_HEADER], str);
	array->index_to_bin[idx] = bin;
	array->bin_offsets[idx] = array->bin_len[bin]+BIN_STR_HEADER;
	array->bin_len[bin] += BIN_STR_HEADER+str_len+1;
	return 0;
}

int strarray_stash(struct hashed_strarray* array, const char* str, int idx)
{
	// The bin is just a random number here, because find
	// cannot be used after stash anyway, only lookup can.
	return s_stash_at_bin(array, str, idx-1, idx % array->num_bins);
}

int strarray_used_slots(struct hashed_strarray* array)
{
	int i, num_used_slots;
	num_used_slots = 0;
	if (!array->bin_offsets) return 0;
	for (i = 0; i < array->highest_index; i++) {
		if (array->bin_offsets[i])
			num_used_slots++;
	}
	return num_used_slots;
}

int strarray_init(struct hashed_strarray* array, int highest_index)
{
	memset(array, 0, sizeof(*array));
	array->highest_index = highest_index;
	array->num_bins = highest_index / 64;

	array->bin_strings = calloc(array->num_bins,sizeof(*array->bin_strings));
	array->bin_len = calloc(array->num_bins,sizeof(*array->bin_len));
	array->bin_offsets = calloc(array->highest_index,sizeof(*array->bin_offsets));
	array->index_to_bin = calloc(array->highest_index,sizeof(*array->index_to_bin));
	
	if (!array->bin_strings || !array->bin_len
	    || !array->bin_offsets || !array->index_to_bin) {
		fprintf(stderr, "Out of memory in %s:%i\n", __FILE__, __LINE__);
		free(array->bin_strings);
		free(array->bin_len);
		free(array->bin_offsets);
		free(array->index_to_bin);
		return -1;
	}
	return 0;
}

void strarray_free(struct hashed_strarray* array)
{
	int i;
	for (i = 0; i < sizeof(array->bin_strings)/
		sizeof(array->bin_strings[0]); i++) {
		free(array->bin_strings[i]);
		array->bin_strings[i] = 0;
	}
	free(array->bin_strings);
	array->bin_strings = 0;
	free(array->bin_len);
	array->bin_len = 0;
	free(array->bin_offsets);
	array->bin_offsets = 0;
	free(array->index_to_bin);
	array->index_to_bin = 0;
}
