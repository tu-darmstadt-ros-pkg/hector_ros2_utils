// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/parameters/reconfigurable_parameter.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using hector::createReconfigurableParameter;
using hector::ParameterOptions;
using hector::ParameterSubscription;

namespace
{
rclcpp::Node::SharedPtr makeNode( const std::string &name,
                                  const std::vector<rclcpp::Parameter> &overrides = {} )
{
  rclcpp::NodeOptions options;
  if ( !overrides.empty() )
    options.parameter_overrides( overrides );
  return rclcpp::Node::make_shared( name, options );
}

// Mirrors downstream usage (e.g. a plugin) that receives the node as a const shared pointer
// reference and passes it straight through.
ParameterSubscription declareViaConstSharedPtr( const rclcpp::Node::SharedPtr &node,
                                                const std::string &name, int &value )
{
  return createReconfigurableParameter( node, name, std::ref( value ), "desc" );
}

// Compile-time guard: the node may be passed either by reference or as a (shared) pointer. This
// keeps downstream code that passes a rclcpp::Node::SharedPtr compiling, while the hector::Node and
// hector::LifecycleNode helpers pass the node by reference. A regression in the signature would
// drop one of these forms and fail to compile here.
template<typename Node, typename = void>
struct createCallableWith : std::false_type {
};
template<typename Node>
struct createCallableWith<
    Node, std::void_t<decltype( createReconfigurableParameter(
              std::declval<Node>(), std::declval<const std::string &>(),
              std::ref( std::declval<int &>() ), std::declval<const std::string &>() ) )>>
    : std::true_type {
};

static_assert( createCallableWith<rclcpp::Node &>::value, "Node must be accepted by reference." );
static_assert( createCallableWith<rclcpp::Node::SharedPtr &>::value,
               "Node must be accepted as a shared pointer lvalue." );
static_assert( createCallableWith<const rclcpp::Node::SharedPtr &>::value,
               "Node must be accepted as a const shared pointer reference (downstream style)." );
static_assert( createCallableWith<rclcpp::Node::SharedPtr>::value,
               "Node must be accepted as a shared pointer rvalue." );
} // namespace

TEST( ReconfigurableParameter, initializesWithDefault )
{
  auto node = makeNode( "rp_default" );
  int value = 42;
  auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 42 );
  EXPECT_EQ( node->get_parameter( "p" ).as_int(), 42 );
  EXPECT_TRUE( sub.isValid() );
}

TEST( ReconfigurableParameter, initializesFromOverride )
{
  auto node = makeNode( "rp_override", { rclcpp::Parameter( "p", 7 ) } );
  int value = 42;
  auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 7 ) << "Override value should be written back to the referenced variable.";
}

TEST( ReconfigurableParameter, updatesReferencedValue )
{
  auto node = makeNode( "rp_update" );
  int value = 0;
  auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );

  auto result = node->set_parameter( rclcpp::Parameter( "p", 123 ) );
  EXPECT_TRUE( result.successful );
  EXPECT_EQ( value, 123 ) << "Reference must be kept in sync after a successful update.";
}

TEST( ReconfigurableParameter, updateCallbackFires )
{
  auto node = makeNode( "rp_cb" );
  int value = 0;
  int callback_value = -1;
  auto sub = createReconfigurableParameter(
      *node, "p", std::ref( value ), "desc",
      ParameterOptions<int>().onUpdate( [&]( const int &v ) { callback_value = v; } ) );

  node->set_parameter( rclcpp::Parameter( "p", 5 ) );
  EXPECT_EQ( value, 5 );
  EXPECT_EQ( callback_value, 5 ) << "onUpdate callback must receive the new value.";
}

TEST( ReconfigurableParameter, validatorRejectsInvalidValue )
{
  auto node = makeNode( "rp_validate" );
  int value = 1;
  auto sub = createReconfigurableParameter(
      *node, "p", std::ref( value ), "desc",
      ParameterOptions<int>().onValidate( []( const int &v ) { return v >= 0; } ) );

  auto rejected = node->set_parameter( rclcpp::Parameter( "p", -5 ) );
  EXPECT_FALSE( rejected.successful );
  EXPECT_EQ( value, 1 ) << "Reference must stay unchanged when validation fails.";

  auto accepted = node->set_parameter( rclcpp::Parameter( "p", 9 ) );
  EXPECT_TRUE( accepted.successful );
  EXPECT_EQ( value, 9 );
}

TEST( ReconfigurableParameter, integerRangeDescriptor )
{
  auto node = makeNode( "rp_int_range" );
  int value = 5;
  auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc",
                                            ParameterOptions<int>().setRange( 0, 10, 1 ) );
  auto desc = node->describe_parameter( "p" );
  ASSERT_EQ( desc.integer_range.size(), 1u );
  EXPECT_EQ( desc.integer_range[0].from_value, 0 );
  EXPECT_EQ( desc.integer_range[0].to_value, 10 );
  EXPECT_EQ( desc.integer_range[0].step, 1 );
}

TEST( ReconfigurableParameter, floatingPointRangeDescriptor )
{
  auto node = makeNode( "rp_double_range" );
  double value = 0.5;
  auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc",
                                            ParameterOptions<double>().setRange( 0.0, 1.0, 0.1 ) );
  auto desc = node->describe_parameter( "p" );
  ASSERT_EQ( desc.floating_point_range.size(), 1u );
  EXPECT_DOUBLE_EQ( desc.floating_point_range[0].from_value, 0.0 );
  EXPECT_DOUBLE_EQ( desc.floating_point_range[0].to_value, 1.0 );
}

TEST( ReconfigurableParameter, additionalConstraintsInDescriptor )
{
  auto node = makeNode( "rp_constraints" );
  std::string value = "a";
  auto sub = createReconfigurableParameter(
      *node, "p", std::ref( value ), "desc",
      ParameterOptions<std::string>().setAdditionalConstraints( "only a or b" ) );
  EXPECT_EQ( node->describe_parameter( "p" ).additional_constraints, "only a or b" );
}

TEST( ReconfigurableParameter, arrayTypes )
{
  auto node = makeNode( "rp_arrays" );

  std::vector<bool> bool_param;
  auto bool_sub =
      createReconfigurableParameter( *node, "bool_arr", std::ref( bool_param ), "bool array" );
  std::vector<uint8_t> byte_param;
  auto byte_sub =
      createReconfigurableParameter( *node, "byte_arr", std::ref( byte_param ), "byte array" );
  std::vector<int64_t> int_param;
  auto int_sub =
      createReconfigurableParameter( *node, "int_arr", std::ref( int_param ), "int array" );
  std::vector<double> double_param;
  auto double_sub =
      createReconfigurableParameter( *node, "double_arr", std::ref( double_param ), "double array" );
  std::vector<std::string> string_param;
  auto string_sub =
      createReconfigurableParameter( *node, "string_arr", std::ref( string_param ), "string array" );

  node->set_parameter( rclcpp::Parameter( "int_arr", std::vector<int64_t>{ 1, 2, 3 } ) );
  ASSERT_EQ( int_param.size(), 3u );
  EXPECT_EQ( int_param[2], 3 );
}

TEST( ReconfigurableParameter, subscriptionStopsTrackingAfterReset )
{
  auto node = makeNode( "rp_reset" );
  int value = 0;
  {
    auto sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );
    node->set_parameter( rclcpp::Parameter( "p", 1 ) );
    EXPECT_EQ( value, 1 );
  }
  // After the subscription is destroyed the callback is unregistered, so the reference is
  // no longer updated.
  node->set_parameter( rclcpp::Parameter( "p", 2 ) );
  EXPECT_EQ( value, 1 );
}

TEST( ParameterSubscription, defaultConstructedIsInvalid )
{
  ParameterSubscription sub;
  EXPECT_FALSE( sub.isValid() );
}

TEST( ParameterSubscription, moveTransfersValidity )
{
  auto node = makeNode( "rp_move" );
  int value = 0;
  ParameterSubscription sub = createReconfigurableParameter( *node, "p", std::ref( value ), "desc" );
  EXPECT_TRUE( sub.isValid() );

  ParameterSubscription moved = std::move( sub );
  EXPECT_TRUE( moved.isValid() );
  EXPECT_FALSE( sub.isValid() );

  // Moved-to subscription keeps the tracking alive.
  node->set_parameter( rclcpp::Parameter( "p", 8 ) );
  EXPECT_EQ( value, 8 );
}

TEST( ReconfigurableParameter, acceptsSharedPtrNode )
{
  auto node = makeNode( "rp_shared_ptr" );
  int value = 3;
  // Pass the node as a shared pointer (the form downstream packages use) instead of *node.
  auto sub = createReconfigurableParameter( node, "p", std::ref( value ), "desc" );
  EXPECT_EQ( value, 3 );
  EXPECT_TRUE( sub.isValid() );

  node->set_parameter( rclcpp::Parameter( "p", 17 ) );
  EXPECT_EQ( value, 17 ) << "Shared pointer form must track updates like the reference form.";
}

TEST( ReconfigurableParameter, acceptsConstSharedPtrNode )
{
  auto node = makeNode( "rp_const_shared_ptr" );
  int value = 0;
  auto sub = declareViaConstSharedPtr( node, "p", value );
  EXPECT_TRUE( sub.isValid() );

  node->set_parameter( rclcpp::Parameter( "p", 4 ) );
  EXPECT_EQ( value, 4 );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  rclcpp::init( argc, argv );
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}
