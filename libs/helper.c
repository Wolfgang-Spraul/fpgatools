//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>
#include <errno.h>
#include "model.h"

const char *bitstr(uint32_t value, int digits)
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

void hexdump(int indent, const uint8_t *data, int len)
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

int atom_found(char *bits, const cfg_atom_t *atom)
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

void atom_remove(char *bits, const cfg_atom_t *atom)
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

static int bool_nextlen(const char *expr, int len)
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
static int bool_eval(const char *expr, int len, const int *var)
{
	int i, negate, result, oplen;

	if (len == 1) {
		if (*expr == '1') return 1;
		if (*expr == '0') return 0;
	}
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

uint64_t map_bits(uint64_t u64, int num_bits, int *src_pos)
{
	uint64_t result;
	int i;

	result = 0;
	for (i = 0; i < num_bits; i++) {
		if (u64 & (1ULL<<(src_pos[i])))
			result |= 1ULL<<i;
	}
	return result;
}

int bool_str2bits(const char *str, uint64_t *u64, int num_bits)
{
	int i, j, bool_res, rc, str_len, vars[6];

	if (num_bits == 64)
		*u64 = 0;
	else if (num_bits == 32)
		*u64 &= 0xFFFFFFFF00000000;
	else FAIL(EINVAL);

	str_len = strlen(str);
	for (i = 0; i < num_bits; i++) {
		for (j = 0; j < sizeof(vars)/sizeof(*vars); j++)
			vars[j] = (i & (1<<j)) != 0;
		bool_res = bool_eval(str, str_len, vars);
		if (bool_res == -1) FAIL(EINVAL);
		if (bool_res) *u64 |= 1ULL<<i;
	}
	return 0;
fail:
	return rc;
}

typedef struct _minterm_entry
{
	char a[6]; // 0=A1, 5=A6. value can be 0, 1 or 2 for 'removed'
	int merged;
} minterm_entry;

const char* bool_bits2str(uint64_t u64, int num_bits)
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
	int str_end, first_op, bit_width;
	static char str[2048];

	if (!u64) return "0";
	if (num_bits == 64) {
		if (u64 == 0xFFFFFFFFFFFFFFFFULL) return "1";
		bit_width = 6;
	} else if (num_bits == 32) {
		if (u64 & 0xFFFFFFFF00000000ULL) {
			// upper 32 bits should be 0
			HERE();
			return "0";
		}
		if (u64 == 0x00000000FFFFFFFFULL) return "1";
		bit_width = 5;
	} else {
		HERE();
		return "0";
	}

	memset(mt, 0, sizeof(mt));
	memset(mt_size, 0, sizeof(mt_size));

	// set starting minterms
	for (i = 0; i < num_bits; i++) {
		if (u64 & (1ULL<<i)) {
			for (j = 0; j < bit_width; j++) {
				if (i&(1<<j))
					mt[0][mt_size[0]].a[j] = 1;
			}
			mt_size[0]++;
		}
	}

	// go through five rounds of merging
	for (round = 1; round < 7; round++) {
		for (i = 0; i < mt_size[round-1]; i++) {
			for (j = i+1; j < mt_size[round-1]; j++) {
				only_diff_bit = -1;
				for (k = 0; k < bit_width; k++) {
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
	
					for (k = 0; k < bit_width; k++)
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

	str_end = 0;
	for (round = 0; round < 7; round++) {
		for (i = 0; i < mt_size[round]; i++) {
			if (!mt[round][i].merged) {
				if (str_end)
					str[str_end++] = '+';
				first_op = 1;
				for (j = 0; j < bit_width; j++) {
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

void printf_type2(uint8_t *d, int len, int inpos, int num_entries)
{
	uint64_t u64;
	uint16_t u16;
	int i, j;

	for (i = 0; i < num_entries; i++) {
		u64 = frame_get_u64(&d[inpos+i*8]);
		if (!u64) continue;
		printf("type2 8*%i 0x%016lX\n", i, u64);
		for (j = 0; j < 4; j++) {
			u16 = frame_get_u16(&d[inpos+i*8+j*2]);
			if (u16)
				printf("type2 2*%i 8*%i+%i 0x%04X\n", i*4+j, i, j, u16);
		}
	}
}

void printf_ramb16_data(uint8_t *bits, int inpos)
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

int is_empty(const uint8_t *d, int l)
{
	while (--l >= 0)
		if (d[l]) return 0;
	return 1;
}

int count_bits(const uint8_t *d, int l)
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

int frame_get_bit(const uint8_t *frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	return (frame_d[(bit/16)*2 + !((bit/8)%2)] & v) != 0;
}

void frame_clear_bit(uint8_t *frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	frame_d[(bit/16)*2 + !((bit/8)%2)] &= ~v;
}

void frame_set_bit(uint8_t *frame_d, int bit)
{
	uint8_t v = 1<<(7-(bit%8));
	frame_d[(bit/16)*2 + !((bit/8)%2)] |= v;
}

// see ug380, table 2-5, bit ordering
int frame_get_pinword(const void *bits)
{
	int byte0, byte1;
	byte0 = ((uint8_t*)bits)[0];
	byte1 = ((uint8_t*)bits)[1];
	return byte0 << 8 | byte1;
}

void frame_set_pinword(void* bits, int v)
{
	((uint8_t*)bits)[0] = v >> 8;
	((uint8_t*)bits)[1] = v & 0xFF;
}

uint8_t frame_get_u8(const uint8_t *frame_d)
{
	uint8_t v = 0;
	int i;
	for (i = 0; i < 8; i++)
		if (*frame_d & (1<<i)) v |= 1 << (7-i);
	return v;
}

// see ug380, table 2-5, bit ordering
uint16_t frame_get_u16(const uint8_t *frame_d)
{
	uint16_t high_b, low_b;
	high_b = frame_get_u8(frame_d);
	low_b = frame_get_u8(frame_d+1);
	return (high_b << 8) | low_b;
}

uint32_t frame_get_u32(const uint8_t *frame_d)
{
	uint32_t high_w, low_w;
	low_w = frame_get_u16(frame_d);
	high_w = frame_get_u16(frame_d+2);
	return (high_w << 16) | low_w;
}

uint64_t frame_get_u64(const uint8_t *frame_d)
{
	uint64_t high_w, low_w;
	low_w = frame_get_u32(frame_d);
	high_w = frame_get_u32(frame_d+4);
	return (high_w << 32) | low_w;
}

void frame_set_u8(uint8_t *frame_d, uint8_t v)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (v & (1<<(7-i)))
			(*frame_d) |= 1<<i;
		else
			(*frame_d) &= ~(1<<i);
	}
}

void frame_set_u16(uint8_t *frame_d, uint16_t v)
{
	uint16_t high_b, low_b;
	high_b = v >> 8;
	low_b = v & 0xFF;
	frame_set_u8(frame_d, high_b);
	frame_set_u8(frame_d+1, low_b);
}

void frame_set_u32(uint8_t* frame_d, uint32_t v)
{
	uint32_t high_w, low_w;
	high_w = v >> 16;
	low_w = v & 0xFFFF;
	frame_set_u16(frame_d, low_w);
	frame_set_u16(frame_d+2, high_w);
}

void frame_set_u64(uint8_t* frame_d, uint64_t v)
{
	uint32_t high_w, low_w;
	low_w = v & 0xFFFFFFFF;
	high_w = v >> 32;
	frame_set_u32(frame_d, low_w);
	frame_set_u32(frame_d+4, high_w);
}

uint64_t frame_get_lut64(const uint8_t* two_minors, int v32)
{
	int off_in_frame, i;
	uint32_t m0, m1;
	uint64_t lut64;

	off_in_frame = v32*4;
	if (off_in_frame >= 64)
		off_in_frame += XC6_HCLK_BYTES;

	m0 = frame_get_u32(&two_minors[off_in_frame]);
	m1 = frame_get_u32(&two_minors[FRAME_SIZE + off_in_frame]);
	lut64 = 0;
	for (i = 0; i < 32; i++) {
		if (m0 & (1<<i)) lut64 |= 1ULL << (2*i);
		if (m1 & (1<<i)) lut64 |= 1ULL << (2*i+1);
	}
	return lut64;
}

void frame_set_lut64(uint8_t* two_minors, int v32, uint64_t v)
{
	int off_in_frame, i;
	uint32_t m0, m1;

	m0 = 0;
	m1 = 0;
	for (i = 0; i < 64; i++) {
		if (v & (1ULL << i)) {
			if (i%2)
				m1 |= 1<<(i/2);
			else
				m0 |= 1<<(i/2);
		}
	}
	off_in_frame = v32*4;
	if (off_in_frame >= 64)
		off_in_frame += XC6_HCLK_BYTES;
	frame_set_u32(&two_minors[off_in_frame], m0);
	frame_set_u32(&two_minors[FRAME_SIZE + off_in_frame], m1);
}

// see ug380, table 2-5, bit ordering
static int xc6_bit2pin(int bit)
{
	int pin = bit % XC6_WORD_BITS;
	if (pin >= 8) return 15-(pin-8);
	return 7-pin;
}

int printf_frames(const uint8_t* bits, int max_frames,
	int row, int major, int minor, int print_empty, int no_clock)
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
	// value 128 chosen randomly for readability to decide
	// between printing individual bits or a hex block.
	if (count_bits(bits, 130) <= 128) {
		for (i = 0; i < FRAME_SIZE*8; i++) {
			if (!frame_get_bit(bits, i)) continue;
			if (i >= 512 && i < 528) { // hclk
				if (!no_clock)
					printf("%sbit %i\n", prefix, i);
				continue;
			}
			i_without_clk = i;
			if (i_without_clk >= 528)
				i_without_clk -= 16;
			snprintf(suffix, sizeof(suffix), "64*%i+%i 256*%i+%i pin %i", 
				i_without_clk/64, i_without_clk%64,
				i_without_clk/256, i_without_clk%256,
				xc6_bit2pin(i_without_clk));
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

void printf_clock(const uint8_t* frame, int row, int major, int minor)
{
	int i;
	for (i = 0; i < 16; i++) {
		if (frame_get_bit(frame, 512 + i))
			printf("r%i ma%i mi%i clock %i pin %i\n",
				row, major, minor, i, xc6_bit2pin(i));
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

void printf_extrabits(const uint8_t* maj_bits, int start_minor, int num_minors,
	int start_bit, int num_bits, int row, int major)
{
	int minor, bit, bit_no_clk;

	for (minor = start_minor; minor < start_minor + num_minors; minor++) {
		for (bit = start_bit; bit < start_bit + num_bits; bit++) {
			if (frame_get_bit(&maj_bits[minor*FRAME_SIZE], bit)) {
				bit_no_clk = bit;
				if (bit_no_clk >= 528)
					bit_no_clk -= XC6_HCLK_BITS;
				printf("r%i ma%i mi%i bit %i 64*%i+%i 256*%i+%i\n",
					row, major, minor, bit,
					bit_no_clk/64, bit_no_clk%64,
					bit_no_clk/256, bit_no_clk%256);
			}
		}
	}
}

void write_lut64(uint8_t* two_minors, int off_in_frame, uint64_t u64)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (u64 & (1LL << (i*4)))
			frame_set_bit(two_minors, off_in_frame+i*2);
		if (u64 & (1LL << (i*4+1)))
			frame_set_bit(two_minors, off_in_frame+(i*2)+1);
		if (u64 & (1LL << (i*4+2)))
			frame_set_bit(two_minors + FRAME_SIZE, off_in_frame+i*2);
		if (u64 & (1LL << (i*4+3)))
			frame_set_bit(two_minors + FRAME_SIZE, off_in_frame+(i*2)+1);
	}
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

int mod4_calc(int a, int b)
{
	return (unsigned int) (a+b)%4;
}

int all_zero(const void* d, int num_bytes)
{
	int i;
	for (i = 0; i < num_bytes; i++)
		if (((uint8_t*)d)[i]) return 0;
	return 1;
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
		fprintf(f, "%s", line);
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

int strarray_find(struct hashed_strarray* array, const char* str)
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
					return STRIDX_NO_ENTRY;
				}
				return i+1;
			}
			search_off += *(uint16_t*)&array->bin_strings[bin][search_off-2];
		}
	}
	return STRIDX_NO_ENTRY;
}

int s_stash_at_bin(struct hashed_strarray* array, const char* str, int idx, int bin);

int strarray_add(struct hashed_strarray* array, const char* str, int* idx)
{
	int bin, i, free_index, rc, start_index;
	unsigned long hash;

	*idx = strarray_find(array, str);
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

int row_pos_to_y(int num_rows, int row, int pos)
{
	int y;

	if (row >= num_rows) {
		HERE();
		return -1;
	}
	y = TOP_IO_TILES;
	if (row < num_rows/2)
		y++; // below center
	y += num_rows - row - 1; // hclk in full rows from top
	if (pos >= HALF_ROW)
		y++; // hclk in row
	return y;
}

int cmdline_help(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help")) {
			printf( "\n"
				"%s floorplan\n"
				"\n"
				"Usage: %s [--part=xc6slx9]\n"
				"       %*s [--package=tqg144|ftg256]\n"
				"       %*s [--help]\n",
				*argv, *argv, (int) strlen(*argv), "",
				(int) strlen(*argv), "");
			return 1;
		}
	}
	return 0;
}

int cmdline_part(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--part=xc6slx9"))
			return XC6SLX9;
	}
	return XC6SLX9;
}

int cmdline_package(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--package=tqg144"))
			return TQG144;
		if (!strcmp(argv[i], "--package=ftg256"))
			return FTG256;
	}
	return FTG256;
}

const char *cmdline_strvar(int argc, char **argv, const char *var)
{
	enum { NUM_BUFS = 32, BUF_SIZE = 256 };
	static char buf[NUM_BUFS][BUF_SIZE];
	static int last_buf = 0;
	char scan_str[128];
	int i, next_buf;

	next_buf = (last_buf+1)%NUM_BUFS;
	snprintf(scan_str, sizeof(scan_str), "-D%s=%%s", var);
	for (i = 1; i < argc; i++) {
		if (sscanf(argv[i], scan_str, buf[next_buf]) == 1) {
			last_buf = next_buf;
			return buf[last_buf];
		}
	}
	return 0;
}

int cmdline_intvar(int argc, char **argv, const char *var)
{
	char buf[128];
	int i, out_int;

	snprintf(buf, sizeof(buf), "-D%s=%%i", var);
	for (i = 1; i < argc; i++) {
		if (sscanf(argv[i], buf, &out_int) == 1)
			return out_int;
	}
	return 0;
}
