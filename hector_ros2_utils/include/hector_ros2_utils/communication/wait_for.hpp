// Copyright (c) 2021 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HECTOR_ROS2_UTILS_WAIT_FOR_HPP
#define HECTOR_ROS2_UTILS_WAIT_FOR_HPP

#include "hector_ros2_utils/utils/uuidv4.h"

#include <rcl/wait.h>
#include <rclcpp/duration.hpp>
#include <rclcpp/guard_condition.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/wait_set.hpp>

namespace hector
{

/*!
 * Used as a convenience method to enable latched like behavior in waitForMessage.
 * Note that ROS 2 has no actual concept of latched!
 * The closest is transient_local which has to be enabled for both the publisher and the subscriber.
 * A transient_local subscriber can NOT subscribe to a publisher that is not transient_local.
 * That means if you try to use latched in waitForMessage on a publisher that is NOT transient_local, you will not
 * receive a message as it will not be able to connect!
 *
 * For more information check the following issue:
 * https://github.com/ros2/ros2/issues/464
 *
 * @param history_depth How many messages should be kept in the history. For waitForMessage any number greater 1 makes no sense.
 * @return A rclcpp::QoS with the given history_depth and transient_local set.
 */
rclcpp::QoS latched( size_t history_depth = 1 )
{
  return rclcpp::QoS( history_depth ).transient_local();
}

template<typename Message, typename Rep=int64_t, typename Period=std::milli>
std::shared_ptr<Message>
waitForMessage( const std::string &topic,
                rclcpp::Node::SharedPtr node,
                rclcpp::QoS qos,
                const std::chrono::duration<Rep, Period> &duration = std::chrono::duration<Rep, Period>( -1 ))
{
  if ( node == nullptr ) node = std::make_shared<rclcpp::Node>( "wait_node_" + uuidv4( false ));
  auto sub = node->create_subscription<Message>( topic, qos, []( std::unique_ptr<Message> ) { } );
  rclcpp::ThreadSafeWaitSet wait_set( std::vector<rclcpp::ThreadSafeWaitSet::SubscriptionEntry>{{ sub }} );
  auto wait_result = wait_set.template wait( duration );
  if ( wait_result.kind() != rclcpp::WaitResultKind::Ready ) return nullptr;
  std::shared_ptr<Message> result = std::make_shared<Message>();
  rclcpp::MessageInfo info;
  if ( !sub->take( *result, info )) return nullptr;
  return result;
}

template<typename Message, typename Rep=int64_t, typename Period=std::milli>
std::shared_ptr<Message>
waitForMessage( const std::string &topic,
                const std::chrono::duration<Rep, Period> &duration = std::chrono::duration<Rep, Period>( -1 ))
{
  return waitForMessage<Message, Rep, Period>( topic, nullptr, 1, duration );
}

template<typename Message, typename Rep=int64_t, typename Period=std::milli>
std::shared_ptr<Message>
waitForMessage( const std::string &topic, rclcpp::QoS qos,
                const std::chrono::duration<Rep, Period> &duration = std::chrono::duration<Rep, Period>( -1 ))
{
  return waitForMessage<Message, Rep, Period>( topic, nullptr, qos, duration );
}

template<typename Message, typename Rep=int64_t, typename Period=std::milli>
std::shared_ptr<Message>
waitForMessage( const std::string &topic,
                const rclcpp::Node::SharedPtr &node,
                const std::chrono::duration<Rep, Period> &duration = std::chrono::duration<Rep, Period>( -1 ))
{
  return waitForMessage<Message, Rep, Period>( topic, node, rclcpp::QoS( 1 ), duration );
}
}

#endif //HECTOR_ROS2_UTILS_WAIT_FOR_HPP
