// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_LIFECYCLE_NODE_HPP
#define HECTOR_ROS2_UTILS_LIFECYCLE_NODE_HPP

#include "hector_ros2_utils/parameters/reconfigurable_parameter.hpp"

// rclcpp_lifecycle is an optional dependency: the LifecycleNode helper is only available when the
// rclcpp_lifecycle headers are present. This lets the single-header export be used in projects that
// do not have rclcpp_lifecycle available.
#if __has_include( <rclcpp_lifecycle/lifecycle_node.hpp> )
  #include <rclcpp_lifecycle/lifecycle_node.hpp>

namespace hector
{

class LifecycleNode : public rclcpp_lifecycle::LifecycleNode
{
public:
  using SharedPtr = std::shared_ptr<LifecycleNode>;
  using ConstSharedPtr = std::shared_ptr<const LifecycleNode>;
  using WeakPtr = std::weak_ptr<LifecycleNode>;

  using rclcpp_lifecycle::LifecycleNode::LifecycleNode;

  template<typename ParameterT>
  void declare_reconfigurable_parameter( const std::string &name,
                                         std::reference_wrapper<ParameterT> value,
                                         const std::string &description,
                                         const ParameterOptions<ParameterT> &options = {} )
  {
    try {
      ParameterSubscription subscription =
          createReconfigurableParameter( *this, name, value, description, options );
      reconfigurable_parameters_.push_back( std::move( subscription ) );
    } catch ( const rclcpp::ParameterTypeException &ex ) {
      throw rclcpp::exceptions::InvalidParameterTypeException( name, ex.what() );
    }
  }

  /*!
   * Declares a parameter that is read-only and cannot be changed at runtime.
   * Updates the given value with the parameter value using the original value as default.
   * @param options Options to configure the parameter.
   */
  template<typename ParameterT>
  auto declare_readonly_parameter( const std::string &name, ParameterT &value,
                                   const std::string &description,
                                   const ReadOnlyParameterOptions &options = {} )
  {
    try {
      rcl_interfaces::msg::ParameterDescriptor param_desc;
      param_desc.description = description;
      param_desc.read_only = true;
      rclcpp::ParameterValue parameter_value( value );
      value = this->declare_parameter( name, parameter_value, param_desc, options.ignore_override )
                  .get<ParameterT>();
      return value;
    } catch ( const rclcpp::ParameterTypeException &ex ) {
      throw rclcpp::exceptions::InvalidParameterTypeException( name, ex.what() );
    }
  }

private:
  std::vector<ParameterSubscription> reconfigurable_parameters_;
};

} // namespace hector

#endif // __has_include( <rclcpp_lifecycle/lifecycle_node.hpp> )

#endif // HECTOR_ROS2_UTILS_LIFECYCLE_NODE_HPP
