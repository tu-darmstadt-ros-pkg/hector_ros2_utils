// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP
#define HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/parameter.hpp>

namespace hector
{

class ParameterSubscription
{
public:
  ParameterSubscription() = default;
  ParameterSubscription(
      rclcpp::Parameter parameter,
      rclcpp::node_interfaces::PreSetParametersCallbackHandle::SharedPtr pre_set_callback,
      rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr on_set_callback,
      rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr post_set_callback,
      rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr validate_value_callback,
      rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr update_value_callback )
      : parameter( std::move( parameter ) ), pre_set_callback( std::move( pre_set_callback ) ),
        on_set_callback( std::move( on_set_callback ) ),
        post_set_callback( std::move( post_set_callback ) ),
        validate_value_callback( std::move( validate_value_callback ) ),
        update_value_callback( std::move( update_value_callback ) ), valid_( true )
  {
  }
  ParameterSubscription( const ParameterSubscription & ) = delete;
  ParameterSubscription( ParameterSubscription &&other ) { *this = std::move( other ); }

  bool isValid() const { return valid_; }

  ParameterSubscription &operator=( const ParameterSubscription & ) = delete;
  ParameterSubscription &operator=( ParameterSubscription &&other )
  {
    if ( this == &other )
      return *this;

    parameter = std::move( other.parameter );
    pre_set_callback = std::move( other.pre_set_callback );
    on_set_callback = std::move( other.on_set_callback );
    post_set_callback = std::move( other.post_set_callback );
    validate_value_callback = std::move( other.validate_value_callback );
    update_value_callback = std::move( other.update_value_callback );
    valid_ = other.valid_;
    other.valid_ = false;
    return *this;
  }

private:
  rclcpp::Parameter parameter;
  rclcpp::node_interfaces::PreSetParametersCallbackHandle::SharedPtr pre_set_callback;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr on_set_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr post_set_callback;

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr validate_value_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr update_value_callback;

  //! Whether this subscription is active, i.e., not default-constructed or moved-from.
  bool valid_ = false;
};

struct ReadOnlyParameterOptions {
  bool ignore_override = false;

  ReadOnlyParameterOptions &ignoreOverride( bool value = true )
  {
    ignore_override = value;
    return *this;
  }
};

template<typename ParameterT>
struct ParameterRange {
  ParameterT min;
  ParameterT max;
  ParameterT step;
};

template<typename ParameterT>
struct ParameterOptions : public ReadOnlyParameterOptions {
  std::string additional_constraints;
  rclcpp::Node::PreSetParametersCallbackType pre_set_callback;
  rclcpp::Node::OnSetParametersCallbackType on_set_callback;
  rclcpp::Node::PostSetParametersCallbackType post_set_callback;
  std::function<bool( const ParameterT & )> validator_callback;
  std::function<void( const ParameterT & )> updated_callback;
  std::optional<ParameterRange<ParameterT>> range;

  //! Additional constraints for the parameter. Description of the constraints.
  //! Enforcing the constraints is up to the user in onValidate.
  ParameterOptions &setAdditionalConstraints( const std::string &constraints )
  {
    additional_constraints = constraints;
    return *this;
  }

  ParameterOptions &setRange( const ParameterT &min, const ParameterT &max, const ParameterT &step )
  {
    range = ParameterRange<ParameterT>{ min, max, step };
    return *this;
  }

  //! Callback that is called before the parameter is set. Can be used to enforce additional
  //! constraints. Return false to reject the parameter update.
  ParameterOptions &onValidate( std::function<bool( const ParameterT & )> value )
  {
    validator_callback = std::move( value );
    return *this;
  }

  //! Callback that is called after the parameter is updated.
  ParameterOptions &onUpdate( std::function<void( const ParameterT & )> value )
  {
    updated_callback = std::move( value );
    return *this;
  }

  ParameterOptions &onPreSet( rclcpp::Node::PreSetParametersCallbackType value )
  {
    pre_set_callback = std::move( value );
    return *this;
  }

  ParameterOptions &onSet( rclcpp::Node::OnSetParametersCallbackType value )
  {
    on_set_callback = std::move( value );
    return *this;
  }

  ParameterOptions &onPostSet( rclcpp::Node::PostSetParametersCallbackType value )
  {
    post_set_callback = std::move( value );
    return *this;
  }
};

template<typename T>
struct is_vector : std::false_type {
};

template<typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type {
};

template<typename T>
struct vector_type {
};

template<typename T, typename A>
struct vector_type<std::vector<T, A>> {
  using type = T;
};

template<typename T>
struct is_shared_ptr : std::false_type {
};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
};

/*!
 * Creates a reconfigurable parameter on the given node and keeps the passed value reference
 * in sync with the parameter value as long as the returned subscription lives.
 *
 * Works with any node type that exposes the rclcpp parameter interface, i.e., both
 * rclcpp::Node and rclcpp_lifecycle::LifecycleNode. The node may be passed either by reference or
 * as a (shared) pointer.
 *
 * @param node_ref The node the parameter is declared on. Must outlive the returned subscription.
 * @param name Name of the parameter.
 * @param param Reference to the value that is initialized and kept in sync with the parameter.
 * @param description Description of the parameter.
 * @param options Options to configure validation, update callbacks and the parameter range.
 */
template<typename ParameterT, typename NodeT>
[[nodiscard]] ParameterSubscription createReconfigurableParameter(
    NodeT &&node_ref, const std::string &name, std::reference_wrapper<ParameterT> param,
    const std::string &description, const ParameterOptions<ParameterT> &options = {} )
{
  // Accept the node either by reference or as a (shared) pointer and normalize to a reference so
  // the body can uniformly use the rclcpp parameter interface.
  auto &node = [&]() -> auto & {
    if constexpr ( is_shared_ptr<std::remove_cv_t<std::remove_reference_t<NodeT>>>::value )
      return *node_ref;
    else
      return node_ref;
  }();
  if constexpr ( is_vector<ParameterT>::value ) {
    using VT = typename vector_type<ParameterT>::type;
    // Use static assert here, because ParameterValue also supports int to initialize but returns
    // int64_t when getting the value.
    static_assert( std::is_same_v<bool, VT> || std::is_same_v<uint8_t, VT> ||
                       std::is_same_v<int64_t, VT> || std::is_same_v<double, VT> ||
                       std::is_same_v<std::string, VT>,
                   "Only bool, long, double and string are supported as vector types" );
  }
  rcl_interfaces::msg::ParameterDescriptor param_desc;
  param_desc.name = name;
  param_desc.description = description;
  param_desc.additional_constraints = options.additional_constraints;
  param_desc.read_only = false;
  if ( options.range ) {
    if constexpr ( std::is_floating_point_v<ParameterT> ) {
      rcl_interfaces::msg::FloatingPointRange range;
      range.from_value = options.range->min;
      range.to_value = options.range->max;
      range.step = options.range->step;
      param_desc.floating_point_range.push_back( range );
    } else if constexpr ( std::is_integral_v<ParameterT> ) {
      rcl_interfaces::msg::IntegerRange range;
      range.from_value = options.range->min;
      range.to_value = options.range->max;
      range.step = options.range->step;
      param_desc.integer_range.push_back( range );
    }
  }

  rclcpp::ParameterValue parameter_value( param.get() );
  param.get() = node.declare_parameter( name, parameter_value, param_desc, options.ignore_override )
                    .template get<ParameterT>();

  rclcpp::Parameter rclcpp_parameter = node.get_parameter( name );
  rclcpp::node_interfaces::PreSetParametersCallbackHandle::SharedPtr pre_set_callback;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr on_set_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr post_set_callback;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr validate_value_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr update_value_callback;
  if ( options.pre_set_callback )
    pre_set_callback = node.add_pre_set_parameters_callback( options.pre_set_callback );
  if ( options.on_set_callback )
    on_set_callback = node.add_on_set_parameters_callback( options.on_set_callback );
  if ( options.post_set_callback )
    post_set_callback = node.add_post_set_parameters_callback( options.post_set_callback );
  update_value_callback = node.add_post_set_parameters_callback(
      [logger = node.get_logger(), name, param, updated_callback = options.updated_callback](
          const std::vector<rclcpp::Parameter> &parameters ) {
        for ( const auto &parameter : parameters ) {
          if ( parameter.get_name() == name ) {
            RCLCPP_DEBUG_STREAM( logger, "Updating parameter " << name << " to "
                                                               << parameter.value_to_string()
                                                               << "." );
            param.get() = parameter.get_value<ParameterT>();
            if ( updated_callback ) {
              updated_callback( param );
            }
          }
        }
      } );
  if ( options.validator_callback ) {
    validate_value_callback =
        node.add_on_set_parameters_callback( [name, validator = options.validator_callback](
                                                 const std::vector<rclcpp::Parameter> &parameters ) {
          for ( const auto &parameter : parameters ) {
            if ( parameter.get_name() == name && !validator( parameter.get_value<ParameterT>() ) ) {
              return rcl_interfaces::msg::SetParametersResult().set__successful( false ).set__reason(
                  "Parameter value is invalid." );
            }
          }
          return rcl_interfaces::msg::SetParametersResult().set__successful( true );
        } );
  }
  return { rclcpp_parameter,  pre_set_callback,        on_set_callback,
           post_set_callback, validate_value_callback, update_value_callback };
}
} // namespace hector

#endif // HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP
