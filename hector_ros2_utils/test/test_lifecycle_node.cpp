// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/lifecycle_node.hpp"

#include <gtest/gtest.h>

namespace
{
std::shared_ptr<hector::LifecycleNode>
makeNode( const std::string &name, const std::vector<rclcpp::Parameter> &overrides = {} )
{
  rclcpp::NodeOptions options;
  if ( !overrides.empty() )
    options.parameter_overrides( overrides );
  return std::make_shared<hector::LifecycleNode>( name, options );
}
} // namespace

TEST( LifecycleNode, declareReconfigurableParameter )
{
  auto node = makeNode( "lc_reconfigurable" );
  int value = 3;
  node->declare_reconfigurable_parameter( "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 3 );

  node->set_parameter( rclcpp::Parameter( "p", 11 ) );
  EXPECT_EQ( value, 11 ) << "hector::LifecycleNode must keep the reference in sync.";
}

TEST( LifecycleNode, declareReconfigurableParameterFromOverride )
{
  auto node = makeNode( "lc_override", { rclcpp::Parameter( "p", 7 ) } );
  int value = 3;
  node->declare_reconfigurable_parameter( "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 7 );
}

TEST( LifecycleNode, declareReconfigurableParameterWithValidator )
{
  auto node = makeNode( "lc_validator" );
  int value = 0;
  node->declare_reconfigurable_parameter(
      "p", std::ref( value ), "desc",
      hector::ParameterOptions<int>().onValidate( []( const int &v ) { return v < 100; } ) );

  EXPECT_FALSE( node->set_parameter( rclcpp::Parameter( "p", 200 ) ).successful );
  EXPECT_EQ( value, 0 );
  EXPECT_TRUE( node->set_parameter( rclcpp::Parameter( "p", 50 ) ).successful );
  EXPECT_EQ( value, 50 );
}

TEST( LifecycleNode, declareReadonlyParameter )
{
  auto node = makeNode( "lc_readonly" );
  std::string value = "default";
  node->declare_readonly_parameter( "p", value, "desc" );
  EXPECT_TRUE( node->describe_parameter( "p" ).read_only );
  EXPECT_FALSE( node->set_parameter( rclcpp::Parameter( "p", "other" ) ).successful );
  EXPECT_EQ( node->get_parameter( "p" ).as_string(), "default" );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  rclcpp::init( argc, argv );
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}
