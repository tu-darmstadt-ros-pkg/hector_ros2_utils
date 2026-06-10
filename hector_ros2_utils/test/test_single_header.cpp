// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Only the generated single-header export is included here on purpose, to verify it is
// self-contained and provides the full public API.
#include "hector_ros2_utils.hpp"

#include <gtest/gtest.h>

TEST( SingleHeader, providesUuidv4 ) { EXPECT_EQ( hector::uuidv4().length(), 36u ); }

TEST( SingleHeader, providesReconfigurableParameter )
{
  auto node = rclcpp::Node::make_shared( "single_header_node" );
  int value = 0;
  auto sub = hector::createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );
  node->set_parameter( rclcpp::Parameter( "p", 5 ) );
  EXPECT_EQ( value, 5 );
}

TEST( SingleHeader, providesNodeAndLifecycleNode )
{
  auto node = std::make_shared<hector::Node>( "single_header_hector_node" );
  int value = 1;
  node->declare_reconfigurable_parameter( "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 1 );

  auto lc_node = std::make_shared<hector::LifecycleNode>( "single_header_lifecycle_node" );
  int lc_value = 2;
  lc_node->declare_reconfigurable_parameter( "p", std::ref( lc_value ), "desc" );
  EXPECT_EQ( lc_value, 2 );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  rclcpp::init( argc, argv );
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}
