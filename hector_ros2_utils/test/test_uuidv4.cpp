// Copyright (c) 2021 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/utils/uuidv4.h"

#include <gtest/gtest.h>

#include <set>

namespace
{

bool isHex( char c )
{
  return ( '0' <= c && c <= '9' ) || ( 'a' <= c && c <= 'f' ) || ( 'A' <= c && c <= 'F' );
}

::testing::AssertionResult isValidUuidv4( const std::string &uuid, bool with_hyphens )
{
  const size_t expected_length = with_hyphens ? 36 : 32;
  if ( uuid.length() != expected_length )
    return ::testing::AssertionFailure()
           << "Uuidv4 should be " << expected_length << " characters. Uuid: " << uuid;

  // Positions of the version ('4') and variant (8, 9, a or b) characters.
  const size_t version_pos = with_hyphens ? 14 : 12;
  const size_t variant_pos = with_hyphens ? 19 : 16;
  const std::set<size_t> hyphen_positions =
      with_hyphens ? std::set<size_t>{ 8, 13, 18, 23 } : std::set<size_t>{};

  for ( size_t i = 0; i < uuid.length(); ++i ) {
    const char c = uuid[i];
    if ( hyphen_positions.count( i ) ) {
      if ( c != '-' )
        return ::testing::AssertionFailure()
               << "Expected hyphen at index " << i << ". Uuid: " << uuid;
      continue;
    }
    if ( i == version_pos ) {
      if ( c != '4' )
        return ::testing::AssertionFailure()
               << "Version character at index " << i << " should be '4'. Uuid: " << uuid;
      continue;
    }
    if ( i == variant_pos ) {
      if ( c < '8' || c > 'b' )
        return ::testing::AssertionFailure()
               << "Variant character at index " << i << " should be in [8-b]. Uuid: " << uuid;
      continue;
    }
    if ( !isHex( c ) )
      return ::testing::AssertionFailure() << "Uuid contains invalid character '" << c
                                           << "' at index " << i << "! Uuid: " << uuid;
  }
  return ::testing::AssertionSuccess();
}

} // namespace

TEST( Uuidv4, withHyphens )
{
  std::string uuid1 = hector::uuidv4();
  ASSERT_TRUE( isValidUuidv4( uuid1, true ) );
  std::string uuid2 = hector::uuidv4();
  ASSERT_TRUE( isValidUuidv4( uuid2, true ) );
  ASSERT_NE( uuid1, uuid2 );
}

TEST( Uuidv4, withoutHyphens )
{
  std::string uuid = hector::uuidv4( false );
  ASSERT_TRUE( isValidUuidv4( uuid, false ) );
  ASSERT_EQ( uuid.find( '-' ), std::string::npos );
}

TEST( Uuidv4, uniqueness )
{
  std::set<std::string> uuids;
  for ( int i = 0; i < 1000; ++i ) {
    std::string uuid = hector::uuidv4();
    ASSERT_TRUE( isValidUuidv4( uuid, true ) );
    ASSERT_TRUE( uuids.insert( uuid ).second ) << "Duplicate uuid generated: " << uuid;
  }
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
