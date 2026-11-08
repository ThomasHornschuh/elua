#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdbool.h>
#include <stdint.h>
enum { FALSE = 0, TRUE  = !FALSE };

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int BOOL;

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

#endif
