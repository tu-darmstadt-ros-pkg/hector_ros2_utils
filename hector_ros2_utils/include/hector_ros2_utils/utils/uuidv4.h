// Copyright (c) 2021 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_UUIDV4_H
#define HECTOR_ROS2_UTILS_UUIDV4_H

#include <random>
#include <string>

namespace hector
{

std::string uuidv4( bool include_hyphens = true )
{
  static std::random_device rd;
  static std::mt19937_64 gen( rd());
  static std::uniform_int_distribution<> dis( 0, 15 );
  static std::uniform_int_distribution<> dis2( 8, 11 );
  static const char hex[17] = "0123456789abcdef";
  char result[36] = {};
  int i;
  for ( i = 0; i < 8; i++ ) result[i] = hex[dis( gen )];
  if ( include_hyphens )
  {
    result[i] = '-';
    ++i;
  }
  for ( ; i < (include_hyphens ? 13 : 12); i++ ) result[i] = hex[dis( gen )];
  if ( include_hyphens )
  {
    result[i] = '-';
    ++i;
  }
  result[i] = '4';
  ++i;
  for ( ; i < (include_hyphens ? 18 : 16); i++ ) result[i] = hex[dis( gen )];
  if ( include_hyphens )
  {
    result[i] = '-';
    ++i;
  }
  result[i] = hex[dis2( gen )];
  ++i;
  for ( ; i < (include_hyphens ? 23 : 20); i++ ) result[i] = hex[dis( gen )];
  if ( include_hyphens )
  {
    result[i] = '-';
    ++i;
  }
  for ( ; i < (include_hyphens ? 36 : 32); i++ ) result[i] = hex[dis( gen )];
  return std::string( result, 36 );
}
}

#endif //HECTOR_ROS2_UTILS_UUIDV4_H
