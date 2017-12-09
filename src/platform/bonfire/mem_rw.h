#ifndef __MEM_RW_H__
#define __MEM_RW_H__


#include <stdint.h>


inline void _write_word(void* address,uint32_t value)
{
  *(( volatile uint32_t* )( address ))=value;
}

inline uint32_t _read_word(void* address)
{
  return  *((volatile uint32_t* )( address ));

}


inline void _mem_and(void *address,uint32_t value)
{
   *(( volatile uint32_t* )( address ))&=value;
}

inline void _mem_or(void *address,uint32_t value)
{
   *(( volatile uint32_t* )( address ))|=value;
}

inline void _set_bit(void * address,int bit)
{
  _mem_or( address,1<<bit );
}

inline void _clear_bit(void * address,int bit)
{
  _mem_and(address,~(1<<bit) );
}

inline int _get_bit(void * address,int bit)
{
  return (_read_word(address) >> bit) & 0x1;
}




inline void _write_byte(void* address,uint8_t value)
{
  *(( volatile uint8_t* )( address ))=value;
}


inline uint8_t _read_byte(void* address)
{
  return  *((volatile uint8_t* )( address ));

}



#endif
