# HECTOR ROS2 Utils

This library contains some small helper and convenience functions that may help
reduce boilerplate code in ROS2.

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
