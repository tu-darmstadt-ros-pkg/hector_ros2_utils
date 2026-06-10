# HECTOR ROS2 Utils

This library contains some small helper and convenience functions that may help
reduce boilerplate code in ROS2.

It is header-only and can be used either as a package through the modular headers
below or through a single amalgamated header (see [Single header](#single-header)).

## Parameters

To enable easier use of reconfigurable parameters, we provide a helper function:

```cpp
ParameterSubscription subscription = hector::createReconfigurableParameter(
  node, "my_parameter",
  std::ref( my_parameter ), "A parameter that can be modified",
  hector::ParameterOptions<std::string>()
  .onValidate([]( const auto &value ) {
    return value == "my_preferred_value" || value == "comfort_zone"; /* I HATE UPDATES! */
  })
);
```

`my_parameter` is automatically initialized and updated with value changes as long as the `subscription`
object lives, and the `onValidate` method (if provided) accepts the update by returning true.
If the parameter is not overwritten in the launch configuration, `my_parameter` will keep its initial value.

The first argument is the node, accepted either by reference or as a shared pointer (e.g., a
`rclcpp::Node::SharedPtr`). Any node type exposing the rclcpp parameter interface works, i.e., both
`rclcpp::Node` and `rclcpp_lifecycle::LifecycleNode`.

> [!IMPORTANT]
> Please make sure the subscription does not outlive the `node`!

## Node

Using the `hector::Node` as a base class, we get access to more convenience and thought-through
functions, e.g., for declaring reconfigurable parameters.

Example:

```cpp
class MyNode : public hector::Node {
public:
MyNode() : Node("my_node") {
  // Declare serial parameters
  declare_readonly_parameter( "port_name", port_name_, "Serial port name" );
  declare_readonly_parameter( "baud_rate", baud_rate_, "Serial baud rate" );
  declare_reconfigurable_parameter(
      "controller", std::ref( controller_type_ ), "Controller type",
      hector::ParameterOptions<std::string>()
          .setAdditionalConstraints( "Allowed values: diff_drive" )
          .onValidate( []( const auto &value ) { return value == "diff_drive"; } )
          .onUpdate( [this]( const std::string &value ) { setupController( value ); } ) );
}

void setupController( const std::string &name ) { /* do something */ }

private:
std::string port_name_ = "/dev/ttyACM0";
int baud_rate_ = 115200;
std::string controller_type_ = "diff_drive";
};
```

The passed parameters are automatically initialized with the parameter value, defaulting to their
current value.  
The parameters are also automatically updated for reconfigurable parameters.  
Using the options, you have full control over validation and update callbacks.  
Note that the reconfigurable parameter requires a `std::reference_wrapper` obtained using `std::ref`
to make it explicit that this is a reference and reduce the user's risk of using a local variable
without noticing.  
The read-only version also uses a reference, but since it is only initialized with the value once,
it does not have to live past the function call.

The same convenience functions are available on `hector::LifecycleNode`, which derives from
`rclcpp_lifecycle::LifecycleNode`. `createReconfigurableParameter` works with any node type that
exposes the rclcpp parameter interface, i.e., both `rclcpp::Node` and
`rclcpp_lifecycle::LifecycleNode`.

## Single header

In addition to the modular headers, the full public API is also available as a single amalgamated
header `hector_ros2_utils.hpp`. Its purpose is to be copied directly into another project without
depending on this package; it only requires `rclcpp` (and `rclcpp_lifecycle` if the lifecycle node
is used) to be available. Just drop the file in and include it:

```cpp
#include "hector_ros2_utils.hpp"
```

The header is not checked into the repository. Get it by either:

- **Download** it from the assets of the
  [latest GitHub release](https://github.com/tu-darmstadt-ros-pkg/hector_ros2_utils/releases/latest), or
- **Generate** it locally from the modular headers (writes `dist/hector_ros2_utils.hpp`):

  ```bash
  python3 generate_single_header.py
  ```
