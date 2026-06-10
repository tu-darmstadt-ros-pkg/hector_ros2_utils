// Copyright (c) 2025 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_NODE_HPP
#define HECTOR_ROS2_UTILS_NODE_HPP

#include "hector_ros2_utils/parameters/reconfigurable_parameter.hpp"

namespace hector
{

class Node : public rclcpp::Node
{
public:
  using SharedPtr = std::shared_ptr<Node>;
  using ConstSharedPtr = std::shared_ptr<const Node>;
  using WeakPtr = std::weak_ptr<Node>;

  using rclcpp::Node::Node;

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

#endif // HECTOR_ROS2_UTILS_NODE_HPP
