//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include "helper.h"

void printf_help()
{
	printf("\n"
	       "bit2txt %s - convert FPGA bitstream to text\n"
	       "Public domain work by Wolfgang Spraul\n"
	       "\n"
	       "bit2txt [options] <path to .bit file>\n"
	       "  --help                 print help message\n"
	       "  --version              print version number\n"
	       "  --info                 add extra info to output (marked #I)\n"
	       "  <path to .bit file>    bitstream to print on stdout\n"
	       "                         (proposing extension .b2t)\n"
	       "\n", PROGRAM_REVISION);
}

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

int printf_header(uint8_t* d, int len, int inpos, int* outdelta)
{
	int i, str_len;

	*outdelta = 0;
	if (inpos + 13 > len) {
		fprintf(stderr, "#E File size %i below minimum of 13 bytes.\n",
			len);
		return -1;
	}
	printf("hex");
	for (i = 0; i < 13; i++)
		printf(" %.02x", d[inpos+*outdelta+i]);
	printf("\n");
	*outdelta += 13;

	// 4 strings 'a' - 'd', 16-bit length
	for (i = 'a'; i <= 'd'; i++) {
		if (inpos + *outdelta + 3 > len) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", len);
			return -1;
		}
		if (d[inpos + *outdelta] != i) {
			fprintf(stderr, "#E Expected string code '%c', got "
				"'%c'.\n", i, d[inpos + *outdelta]);
			return -1;
		}
		str_len = __be16_to_cpu(*(uint16_t*)&d[inpos + *outdelta + 1]);
		if (inpos + *outdelta + 3 + str_len > len) {
			fprintf(stderr, "#E Unexpected EOF at %i.\n", len);
			return -1;
		}
		if (d[inpos + *outdelta + 3 + str_len - 1]) {
			fprintf(stderr, "#E z-terminated string ends with %0xh"
				".\n", d[inpos + *outdelta + 3 + str_len - 1]);
			return -1;
		}
		printf("header_str_%c %s\n", i, &d[inpos + *outdelta + 3]);
		*outdelta += 3 + str_len;
	}
	return 0;
}

// for an equivalent schematic, see lut.svg
const int lut_base_vars[6] = {0 /* A1 */, 1, 0 /* A3 - not used */,
				0, 0, 1 /* A6 */};

int bool_nextlen(const char* expr, int len)
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
int bool_eval(const char* expr, int len, const int* var)
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

int parse_boolexpr(const char* expr, uint64_t* lut)
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

int bit_set(uint8_t* frame_d, int bit)
{
	return (frame_d[(bit/16)*2 + !((bit/8)%2)] & 1<<(7-(bit%8))) != 0;
}

int printf_frames(uint8_t* bits, int max_frames,
	int row, int major, int minor, int print_empty)
{
	int i;
	char prefix[32];

	if (row < 0)
		sprintf(prefix, "f%i ", abs(row));
	else
		sprintf(prefix, "r%i m%i-%i ", row, major, minor);

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
		printf_clock(bits, row, major, minor);
		for (i = 0; i < 1024; i++) {
			if (bit_set(bits, (i >= 512) ? i + 16 : i))
				printf("%sbit %i\n", prefix, i);
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
		if (bit_set(frame, 512 + i))
			printf("r%i m%i-%i clock %i\n", row, major, minor, i);
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

void printf_singlebits(uint8_t* maj_bits, int start_minor, int num_minors,
	int start_bit, int num_bits, int row, int major)
{
	int minor, bit;

	for (minor = start_minor; minor < start_minor + num_minors; minor++) {
		for (bit = start_bit; bit < start_bit + num_bits; bit++) {
			if (bit_set(&maj_bits[minor*130], bit))
				printf("r%i m%i-%i b%i\n",
					row, major, minor, bit);
		}
	}
}

uint64_t read_lut64(uint8_t* two_minors, int off_in_frame)
{
	uint64_t lut64 = 0;
	int j;

	for (j = 0; j < 16; j++) {
		if (bit_set(two_minors, off_in_frame+j*2))
			lut64 |= 1LL << (j*4);
		if (bit_set(two_minors, off_in_frame+(j*2)+1))
			lut64 |= 1LL << (j*4+1);
		if (bit_set(&two_minors[130], off_in_frame+j*2))
			lut64 |= 1LL << (j*4+2);
		if (bit_set(&two_minors[130], off_in_frame+(j*2)+1))
			lut64 |= 1LL << (j*4+3);
	}
	return lut64;
}
