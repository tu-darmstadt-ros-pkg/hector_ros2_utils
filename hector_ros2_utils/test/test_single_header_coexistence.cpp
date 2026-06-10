// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Regression test: the single header and the modular package headers share the same include
// guards, so including both in one translation unit must NOT cause redefinition conflicts.
#include "hector_ros2_utils.hpp"

#include <hector_ros2_utils/lifecycle_node.hpp>
#include <hector_ros2_utils/node.hpp>
#include <hector_ros2_utils/parameters/reconfigurable_parameter.hpp>
#include <hector_ros2_utils/utils/uuidv4.h>

#include <gtest/gtest.h>

TEST( SingleHeaderCoexistence, compilesAndWorks )
{
  EXPECT_EQ( hector::uuidv4().length(), 36u );
  auto node = std::make_shared<hector::Node>( "coexistence_node" );
  int value = 0;
  node->declare_reconfigurable_parameter( "p", std::ref( value ), "desc" );
  node->set_parameter( rclcpp::Parameter( "p", 3 ) );
  EXPECT_EQ( value, 3 );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  rclcpp::init( argc, argv );
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}
