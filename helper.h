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
#include <sys/types.h>

#define PROGRAM_REVISION "2012-06-27"

void printf_help();

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

int printf_header(uint8_t* d, int len, int inpos, int* outdelta);

void printf_lut6(const char* cfg);
// bits is tested only for 32 and 64
const char* lut2bool(const uint64_t lut, int bits,
	int (*logic_base)[6], int flip_b0);

int printf_iob(uint8_t* d, int len, int inpos, int num_entries);
void printf_ramb16_data(uint8_t* bits, int inpos);

int is_empty(uint8_t* d, int l);
int count_bits(uint8_t* d, int l);
int bit_set(uint8_t* frame_d, int bit);

// if row is negative, it's an absolute frame number and major and
// minor are ignored
int printf_frames(uint8_t* bits, int max_frames, int row, int major,
	int minor, int print_empty);
void printf_clock(uint8_t* frame, int row, int major, int minor);
int clb_empty(uint8_t* maj_bits, int idx);
void printf_singlebits(uint8_t* maj_bits, int start_minor, int num_minors,
	int start_bit, int num_bits, int row, int major);
uint64_t read_lut64(uint8_t* two_minors, int off_in_frame);
