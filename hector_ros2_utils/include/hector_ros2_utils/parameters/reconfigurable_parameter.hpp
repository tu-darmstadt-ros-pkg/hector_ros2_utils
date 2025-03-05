// Copyright (c) 2024 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP
#define HECTOR_ROS2_UTILS_RECONFIGURABLE_PARAMETER_HPP

#include <memory>
#include <string>
#include <utility>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/parameter.hpp>

namespace hector
{

struct ReconfigurableParameterSubscription {
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
struct ReconfigurableParameterOptions : public ReadOnlyParameterOptions {
  std::string additional_constraints;
  rclcpp::Node::PreSetParametersCallbackType pre_set_callback;
  rclcpp::Node::OnSetParametersCallbackType on_set_callback;
  rclcpp::Node::PostSetParametersCallbackType post_set_callback;
  std::function<bool( const ParameterT & )> validator_callback;
  std::function<void( const ParameterT & )> updated_callback;

  ReconfigurableParameterOptions &additionalConstraints( const std::string &constraints )
  {
    additional_constraints = constraints;
    return *this;
  }

  ReconfigurableParameterOptions &onValidate( std::function<bool( const ParameterT & )> value )
  {
    validator_callback = std::move( value );
    return *this;
  }

  ReconfigurableParameterOptions &onUpdate( std::function<void( const ParameterT & )> value )
  {
    updated_callback = std::move( value );
    return *this;
  }

  ReconfigurableParameterOptions &onPreSet( rclcpp::Node::PreSetParametersCallbackType value )
  {
    pre_set_callback = std::move( value );
    return *this;
  }

  ReconfigurableParameterOptions &onSet( rclcpp::Node::OnSetParametersCallbackType value )
  {
    on_set_callback = std::move( value );
    return *this;
  }

  ReconfigurableParameterOptions &onPostSet( rclcpp::Node::PostSetParametersCallbackType value )
  {
    post_set_callback = std::move( value );
    return *this;
  }
};

template<typename ParameterT>
[[nodiscard]] ReconfigurableParameterSubscription
createReconfigurableParameter( const rclcpp::Node::SharedPtr &node, const std::string &name,
                               ParameterT &param, const std::string &description,
                               const ReconfigurableParameterOptions<ParameterT> &options = {} )
{
  rcl_interfaces::msg::ParameterDescriptor param_desc;
  param_desc.description = description;
  param_desc.additional_constraints = options.additional_constraints;
  param_desc.read_only = false;
  rclcpp::ParameterValue parameter_value( param );
  param = node->declare_parameter( name, parameter_value, param_desc, options.ignore_override )
              .template get<ParameterT>();

  ReconfigurableParameterSubscription subscription;
  subscription.parameter = node->get_parameter( name );
  if ( options.pre_set_callback )
    subscription.pre_set_callback = node->add_pre_set_parameters_callback( options.pre_set_callback );
  if ( options.on_set_callback )
    subscription.on_set_callback = node->add_on_set_parameters_callback( options.on_set_callback );
  if ( options.post_set_callback )
    subscription.post_set_callback =
        node->add_post_set_parameters_callback( options.post_set_callback );
  subscription.update_value_callback = node->add_post_set_parameters_callback(
      [node, name, &param, updated_callback = options.updated_callback](
          const std::vector<rclcpp::Parameter> &parameters ) {
        for ( const auto &parameter : parameters ) {
          if ( parameter.get_name() == name ) {
            RCLCPP_DEBUG_STREAM( node->get_logger(),
                                 "Updating parameter " << name << " to "
                                                       << parameter.get_value<ParameterT>() << "." );
            param = parameter.get_value<ParameterT>();
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
