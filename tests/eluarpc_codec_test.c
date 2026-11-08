#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eluarpc.h"
#include "rtype.h"

#define CHECK(condition) \
  do { \
    if( !( condition ) ) { \
      fprintf( stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition ); \
      return 1; \
    } \
  } while( 0 )

static int test_round_trip_and_signatures( void )
{
  u8 packet[ 128 ];
  u8 corrupted[ sizeof( packet ) ];
  const char small_data[] = "small";
  const char data[] = "payload";
  u8 value8 = 0;
  u16 value16 = 0;
  u32 value32 = 0;
  s32 signed_value = 0;
  int int_value = 0;
  void *small_ptr = NULL;
  void *ptr = NULL;
  u16 small_len = 0;
  u32 len = 0;
  u16 packet_len = 0;

  CHECK( eluarpc_gen_write( packet, "ochlLiPp", 0x23, 0x7A, 0xBEEF,
                            ( u32 )0x89ABCDEFU, ( s32 )-42, -7,
                            small_data, ( int )( sizeof( small_data ) - 1 ),
                            data, ( u32 )( sizeof( data ) - 1 ) ) == ELUARPC_OK );
  CHECK( eluarpc_get_packet_size( packet, &packet_len ) == ELUARPC_OK );
  CHECK( packet_len <= sizeof( packet ) );
  CHECK( eluarpc_gen_read( packet, "ochlLiPp", 0x23, &value8, &value16,
                           &value32, &signed_value, &int_value, &small_ptr,
                           &small_len, &ptr, &len ) == ELUARPC_OK );
  CHECK( value8 == 0x7A );
  CHECK( value16 == 0xBEEF );
  CHECK( value32 == ( u32 )0x89ABCDEFU );
  CHECK( signed_value == -42 );
  CHECK( int_value == -7 );
  CHECK( small_len == sizeof( small_data ) - 1 );
  CHECK( len == sizeof( data ) - 1 );
  CHECK( memcmp( small_ptr, small_data, small_len ) == 0 );
  CHECK( memcmp( ptr, data, len ) == 0 );

  memcpy( corrupted, packet, packet_len );
  corrupted[ ELUARPC_START_OFFSET + 2 ] ^= 1;
  CHECK( eluarpc_gen_read( corrupted, "o", 0x23 ) == ELUARPC_ERR );

  memcpy( corrupted, packet, packet_len );
  corrupted[ packet_len - 1 ] ^= 1;
  CHECK( eluarpc_gen_read( corrupted, "ochlLiPp", 0x23, &value8, &value16,
                           &value32, &signed_value, &int_value, &small_ptr,
                           &small_len, &ptr, &len ) == ELUARPC_ERR );
  return 0;
}

static int test_oversized_packet( void )
{
  const u32 payload_len = 0x10000U;
  u8 *packet = calloc( 1, payload_len + 64U );
  u8 *payload = calloc( 1, payload_len );
  u16 packet_len = 1;

  CHECK( packet != NULL && payload != NULL );
  CHECK( eluarpc_gen_write( packet, "op", 1, payload, payload_len ) == ELUARPC_ERR );
  CHECK( eluarpc_get_packet_size( packet, &packet_len ) == ELUARPC_ERR );
  CHECK( packet_len == 0 );
  free( payload );
  free( packet );
  return 0;
}

int main( void )
{
  if( test_round_trip_and_signatures() != 0 )
    return 1;
  if( test_oversized_packet() != 0 )
    return 1;
  puts( "ELUARPC_CODEC_TEST_PASS" );
  return 0;
}
