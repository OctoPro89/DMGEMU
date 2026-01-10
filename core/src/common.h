#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define f32 float
#define f64 double

#ifndef __cplusplus
	#define bool u8

	#define true 1
	#define false 0
#endif // __cplusplus

#define LOBYTE(w) ((u8)(w & 0xFF))
#define HIBYTE(w) ((u8)((w >> 8) & 0xFF))
#define GETBIT(x, n) (((x) >> (n)) & 1)
#define SETBIT(x, n) (x |= (1 << n))
#define CLEARBIT(x, n) (x &= ~(1 << n))
#define U16MSBLSB8(msb, lsb) ((msb << 8) | lsb)
#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

#ifdef _MSC_VER
	#define INLINE _forceinline
#else
	#define INLINE inline
#endif // _MSC_VER

void printaddr(u16 addr);