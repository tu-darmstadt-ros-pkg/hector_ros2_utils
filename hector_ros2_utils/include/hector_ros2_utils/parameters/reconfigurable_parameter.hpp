// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP
#define HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/parameter.hpp>

namespace hector
{

struct ParameterSubscription {
  rclcpp::Parameter parameter;
  rclcpp::node_interfaces::PreSetParametersCallbackHandle::SharedPtr pre_set_callback;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr on_set_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr post_set_callback;

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr validate_value_callback;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr update_value_callback;
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

template<typename ParameterT>
[[nodiscard]] ParameterSubscription
createReconfigurableParameter( const rclcpp::Node::SharedPtr &node, const std::string &name,
                               std::reference_wrapper<ParameterT> param,
                               const std::string &description,
                               const ParameterOptions<ParameterT> &options = {} )
{
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
  param.get() = node->declare_parameter( name, parameter_value, param_desc, options.ignore_override )
                    .template get<ParameterT>();

  ParameterSubscription subscription;
  subscription.parameter = node->get_parameter( name );
  if ( options.pre_set_callback )
    subscription.pre_set_callback = node->add_pre_set_parameters_callback( options.pre_set_callback );
  if ( options.on_set_callback )
    subscription.on_set_callback = node->add_on_set_parameters_callback( options.on_set_callback );
  if ( options.post_set_callback )
    subscription.post_set_callback =
        node->add_post_set_parameters_callback( options.post_set_callback );
  subscription.update_value_callback = node->add_post_set_parameters_callback(
      [node, name, param, updated_callback = options.updated_callback](
          const std::vector<rclcpp::Parameter> &parameters ) {
        for ( const auto &parameter : parameters ) {
          if ( parameter.get_name() == name ) {
            RCLCPP_DEBUG_STREAM( node->get_logger(),
                                 "Updating parameter " << name << " to "
                                                       << parameter.get_value<ParameterT>() << "." );
            param.get() = parameter.get_value<ParameterT>();
            if ( updated_callback ) {
              updated_callback( param );
            }
          }
        }
      } );
  if ( options.validator_callback ) {
    subscription.validate_value_callback =
        node->add_on_set_parameters_callback( [name, validator = options.validator_callback](
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
  return subscription;
}
} // namespace hector

#endif // HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP
