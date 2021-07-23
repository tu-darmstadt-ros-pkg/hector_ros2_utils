// Copyright (c) 2021 Stefan Fabian. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hector_ros2_utils/communication/wait_for.hpp"

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose.hpp>

class Node : public rclcpp::Node
{
public:
  Node() : rclcpp::Node( "message_decoding_test" )
  {
    pose_pub_ = create_publisher<geometry_msgs::msg::Pose>( "/test_pose", hector::latched());
  }

  void publishPose()
  {
    geometry_msgs::msg::Pose pose;
    pose.position.x = 1;
    pose.position.y = 1.5;
    pose.position.z = 2;
    pose.orientation.w = 1;
    pose_pub_->publish( pose );
  }

  rclcpp::Publisher<geometry_msgs::msg::Pose>::SharedPtr pose_pub_;
};

std::shared_ptr<Node> node;

class CommunicationTest : public ::testing::Test
{
protected:

  void SetUp() override
  {
    node->publishPose();
  }

};

TEST_F( CommunicationTest, waitForMessageLatched )
{
  using namespace std::chrono_literals;
  auto msg = hector::waitForMessage<geometry_msgs::msg::Pose>( "/test_pose", 200ms );
  ASSERT_EQ( msg, nullptr );

  msg = hector::waitForMessage<geometry_msgs::msg::Pose>( "/test_pose", hector::latched(), 200ms );
  ASSERT_NE( msg, nullptr );
  EXPECT_DOUBLE_EQ( msg->position.x, 1 );
  EXPECT_DOUBLE_EQ( msg->position.y, 1.5 );
  EXPECT_DOUBLE_EQ( msg->position.z, 2 );
  EXPECT_DOUBLE_EQ( msg->orientation.w, 1 );
}

TEST_F( CommunicationTest, waitForMessagePeriodic )
{
  using namespace std::chrono_literals;
  bool got_message = false;
  std::thread pub_thread( [ &got_message ]()
                          {
                            while ( rclcpp::ok() && !got_message )
                            {
                              node->publishPose();
                              rclcpp::sleep_for( 10ms );
                            }
                          } );
  auto msg = hector::waitForMessage<geometry_msgs::msg::Pose>( "/test_pose", 200ms );
  got_message = true;
  ASSERT_NE( msg, nullptr );
  EXPECT_DOUBLE_EQ( msg->position.x, 1 );
  EXPECT_DOUBLE_EQ( msg->position.y, 1.5 );
  EXPECT_DOUBLE_EQ( msg->position.z, 2 );
  EXPECT_DOUBLE_EQ( msg->orientation.w, 1 );
  pub_thread.join();
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  rclcpp::init( argc, argv );
  node = std::make_shared<Node>();
  std::thread node_thread( []()
                           {
                             rclcpp::spin( node );
                           } );
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  node_thread.join();
  return result;
}
