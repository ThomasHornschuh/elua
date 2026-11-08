#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdint.h>
#include "stm32f10x.h"

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int BOOL;

typedef uint64_t u64;
typedef int64_t  s64;

#endif
