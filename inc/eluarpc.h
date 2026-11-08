// Lightweight remote procedure call layer

#ifndef __ELUARPC_H__
#define __ELUARPC_H__

#include "type.h"

#define   PACKET_SIG          ( ( u32 )0x18AFC284U )
#define   PACKET_END_SIG      ( ( u32 )~PACKET_SIG )

// Error codes
#define   ELUARPC_OK          0
#define   ELUARPC_ERR         1

#define   ELUARPC_OP_RES_MOD  0x80

// Protocol constants
#define   ELUARPC_START_OFFSET    4
#define   ELUARPC_START_SIZE      6
#define   ELUARPC_END_SIZE        6
#define   ELUARPC_RESPONSE_SIZE   1
#define   ELUARPC_PTR_HEADER_SIZE 6
#define   ELUARPC_SMALL_PTR_HEADER_SIZE 4
#define   ELUARPC_U32_SIZE        5
#define   ELUARPC_U16_SIZE        3
#define   ELUARPC_U8_SIZE         2
#define   ELUARPC_OP_ID_SIZE      2
#define   ELUARPC_READ_BUF_OFFSET ( ELUARPC_START_OFFSET + ELUARPC_START_SIZE + ELUARPC_RESPONSE_SIZE + ELUARPC_PTR_HEADER_SIZE )
#define   ELUARPC_SMALL_READ_BUF_OFFSET ( ELUARPC_START_OFFSET + ELUARPC_START_SIZE + ELUARPC_RESPONSE_SIZE + ELUARPC_SMALL_PTR_HEADER_SIZE )
#define   ELUARPC_WRITE_REQUEST_EXTRA ( ELUARPC_START_OFFSET + ELUARPC_START_SIZE + ELUARPC_OP_ID_SIZE + ELUARPC_U32_SIZE + ELUARPC_PTR_HEADER_SIZE + ELUARPC_END_SIZE )

// Public interface
// Get request ID
int eluarpc_get_request_id( const u8 *p, u8 *pid );

// Replace a flag with another flag
u32 eluarpc_replace_flag( u32 val, u32 origflag, u32 newflag );

// Get packet size
int eluarpc_get_packet_size( const u8 *p, u16 *psize );

// Generic write function. Returns ELUARPC_ERR when the encoded packet is
// larger than the u16 wire-size field. The caller must provide a buffer large
// enough for the complete packet, including an oversized packet that is
// rejected after encoding.
// Specifiers: o - operation
//             r - response
//             c - u8
//             h - u16
//             l - u32
//             i - int
//             L - s32
//             p - ptr (given as ptr, len, len is an u32)
//             P - ptr (given as ptr, len, len is an u16)
int eluarpc_gen_write( u8 *p, const char *fmt, ... );

// Generic read function
// Specifiers: o - operation
//             r - response
//             c - u8
//             h - u16
//             l - u32
//             L - s32
//             i - int
//             p - ptr (returned as ptr, len, len is an u32)
//             P - ptr (returned as ptr, len, len is an u16)
int eluarpc_gen_read( const u8 *p, const char *fmt, ... );

#endif
