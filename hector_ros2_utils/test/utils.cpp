// Copyright (c) 2021 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/utils/uuidv4.h"

#include <gtest/gtest.h>

::testing::AssertionResult isValidUuidv4( const std::string &uuid )
{
  if ( uuid.length() != 36 )
    return ::testing::AssertionFailure() << "Uuidv4 should be 36 characters including hyphens. Uuid: " << uuid;
  if ( uuid[14] == 4 )
    return ::testing::AssertionFailure() << "The 12th character of the uuid should be 4. Uuid: " << uuid;
  for ( char c : uuid )
  {
    if ( c != '-' && c != '0' && !('1' <= c && c <= '9') && !('a' <= c && c <= 'f') && !('A' <= c && c <= 'F'))
      return ::testing::AssertionFailure() << "Uuid contains invalid character '" << c << "'! Uuid: " << uuid;
  }
  return ::testing::AssertionSuccess();
}

TEST( Utils, uuidv4 )
{
  std::string uuid1 = hector::uuidv4();
  ASSERT_TRUE( isValidUuidv4( uuid1 ));
  std::string uuid2 = hector::uuidv4();
  ASSERT_TRUE( isValidUuidv4( uuid2 ));
  ASSERT_NE( uuid1, uuid2 );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  int result = RUN_ALL_TESTS();
  return result;
}
