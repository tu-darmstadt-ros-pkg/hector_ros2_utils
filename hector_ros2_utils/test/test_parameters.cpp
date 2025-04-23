// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/parameters/reconfigurable_parameter.hpp"

#include <gtest/gtest.h>

// Currently only tests that it compiles.

TEST( Parameters, simpleTypes )
{
  auto node = rclcpp::Node::make_shared( "test_parameters_node_simple" );
  int int_param = 0;
  auto int_param_sub =
      hector::createReconfigurableParameter( node, "test_int", std::ref( int_param ), "Test int" );
}

TEST( Parameters, arrayTypes )
{
  auto node = rclcpp::Node::make_shared( "test_parameters_node_array" );

  std::vector<bool> bool_param;
  rclcpp::ParameterValue bool_param_value( bool_param );
  auto bool_param_sub = hector::createReconfigurableParameter(
      node, "test_bool_array", std::ref( bool_param ), "Test bool array" );

  std::vector<uint8_t> byte_param;
  rclcpp::ParameterValue byte_param_value( byte_param );
  auto byte_param_sub = hector::createReconfigurableParameter(
      node, "test_byte_array", std::ref( byte_param ), "Test byte array" );

  std::vector<int64_t> int_param;
  rclcpp::ParameterValue int_param_value( int_param );
  auto int_param_sub = hector::createReconfigurableParameter(
      node, "test_int_array", std::ref( int_param ), "Test int array" );

  std::vector<double> double_param;
  rclcpp::ParameterValue double_param_value( double_param );
  auto double_param_sub = hector::createReconfigurableParameter(
      node, "test_double_array", std::ref( double_param ), "Test double array" );

  std::vector<std::string> string_param;
  rclcpp::ParameterValue string_param_value( string_param );
  auto string_param_sub = hector::createReconfigurableParameter(
      node, "test_string_array", std::ref( string_param ), "Test string array" );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  int result = RUN_ALL_TESTS();
  return result;
}
