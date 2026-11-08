#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdint.h>

typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

#ifndef NULL
#define NULL    ((void *)0)
#endif

typedef enum BOOLType{
	FALSE,
	TRUE
}BOOLType;

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int BOOL;

#endif

