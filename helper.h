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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define PROGRAM_REVISION "2012-06-27"
#define MACRO_STR(arg)	#arg

#define OUT_OF_MEM()	{ fprintf(stderr, \
	"Out of memory in %s:%i\n", __FILE__, __LINE__); }
#define EXIT(expr)	if (expr) { fprintf(stderr, \
	"Internal error in %s:%i\n", __FILE__, __LINE__); exit(1); }

#define HERE() fprintf(stderr, "%s:%i\n", __FILE__, __LINE__)
#define FAIL(code) do { HERE(); rc = (code); goto fail; } while (0)
#define XOUT() do { HERE(); goto xout; } while (0)

const char* bitstr(uint32_t value, int digits);
void hexdump(int indent, const uint8_t* data, int len);

uint16_t __swab16(uint16_t x);
uint32_t __swab32(uint32_t x);

#define __be32_to_cpu(x) __swab32((uint32_t)(x))
#define __be16_to_cpu(x) __swab16((uint16_t)(x))
#define __cpu_to_be32(x) __swab32((uint32_t)(x))
#define __cpu_to_be16(x) __swab16((uint16_t)(x))

#define __le32_to_cpu(x) ((uint32_t)(x))
#define __le16_to_cpu(x) ((uint16_t)(x))
#define __cpu_to_le32(x) ((uint32_t)(x))
#define __cpu_to_le16(x) ((uint16_t)(x))

#define ATOM_MAX_BITS	32+1 // -1 signals end of array

typedef struct _cfg_atom
{
	int must_0[ATOM_MAX_BITS];
	int must_1[ATOM_MAX_BITS];
	const char* str;
	int flag; // used to remember a state such as 'found'
} cfg_atom_t;

int atom_found(char* bits, const cfg_atom_t* atom);
void atom_remove(char* bits, const cfg_atom_t* atom);

void printf_lut6(const char* cfg);
// bits is tested only for 32 and 64
const char* lut2bool(const uint64_t lut, int bits,
	int (*logic_base)[6], int flip_b0);

int printf_iob(uint8_t* d, int len, int inpos, int num_entries);
void printf_ramb16_data(uint8_t* bits, int inpos);

int is_empty(uint8_t* d, int l);
int count_bits(uint8_t* d, int l);

int frame_get_bit(uint8_t* frame_d, int bit);
void frame_clear_bit(uint8_t* frame_d, int bit);
void frame_set_bit(uint8_t* frame_d, int bit);

uint8_t frame_get_u8(uint8_t* frame_d);
uint16_t frame_get_u16(uint8_t* frame_d);
uint32_t frame_get_u32(uint8_t* frame_d);
uint64_t frame_get_u64(uint8_t* frame_d);

// if row is negative, it's an absolute frame number and major and
// minor are ignored
int printf_frames(uint8_t* bits, int max_frames, int row, int major,
	int minor, int print_empty);
void printf_clock(uint8_t* frame, int row, int major, int minor);
int clb_empty(uint8_t* maj_bits, int idx);
void printf_extrabits(uint8_t* maj_bits, int start_minor, int num_minors,
	int start_bit, int num_bits, int row, int major);
uint64_t read_lut64(uint8_t* two_minors, int off_in_frame);

int get_vm_mb(void);
int get_random(void);
int compare_with_number(const char* a, const char* b);
// If no next word is found, *end == *beg
void next_word(const char* s, int start, int* beg, int* end);
// if a_len or b_len are -1, the length is until 0-termination
#define ZTERM -1
int str_cmp(const char* a, int a_len, const char* b, int b_len);
// all_digits() returns 0 if len == 0
int all_digits(const char* a, int len);
int to_i(const char* s, int len);

uint32_t hash_djb2(const unsigned char* str);

// Strings are distributed among bins. Each bin is
// one continuous stream of zero-terminated strings
// prefixed with a 32+16=48-bit header. The allocation
// increment for each bin is 32k.
struct hashed_strarray
{
	int highest_index;
	uint32_t* bin_offsets; // min offset is 4, 0 means no entry
	uint16_t* index_to_bin;
	char** bin_strings;
	int* bin_len;
	int num_bins;
};

#define STRIDX_64K	0xFFFF
#define STRIDX_1M	1000000

typedef uint16_t str16_t;

int strarray_init(struct hashed_strarray* array, int highest_index);
void strarray_free(struct hashed_strarray* array);

const char* strarray_lookup(struct hashed_strarray* array, int idx);
// The found or created index will never be 0, so the caller
// can use 0 as a special value to indicate 'no string'.
#define STRIDX_NO_ENTRY 0
int strarray_find(struct hashed_strarray* array, const char* str, int* idx);
int strarray_add(struct hashed_strarray* array, const char* str, int* idx);
// If you stash a string to a fixed index, you cannot use strarray_find()
// anymore, only strarray_lookup().
int strarray_stash(struct hashed_strarray* array, const char* str, int idx);
int strarray_used_slots(struct hashed_strarray* array);
