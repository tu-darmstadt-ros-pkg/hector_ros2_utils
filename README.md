# HECTOR ROS2 Utils

This library contains some small helper and convenience functions that may help
reduce boilerplate code in ROS2.

## Parameters

To enable easier use of reconfigurable parameters we provide a helper function:

```cpp
ParameterSubscription subscription = hector::createReconfigurableParameter(
  node, "my_parameter",
  my_parameter, "A parameter that can be modified",
  hector::ParameterOptions<std::string>()
  .onValidate([]( const auto &value ) {
    return false; /* I HATE UPDATES! */
  })
);
```

`my_parameter` will automatically be kept up to date with value changes as long as the `subscription` object lives.
Please make sure it does not outlive the `node`.

## Node

Using the `hector::Node` as base class, we get access to more convenience and thought-through functions e.g. for declaring reconfigurable parameters.

Example:

```cpp
class MyNode : public hector::Node {
public:
MyNode() : Node("my_node") {
  // Declare serial parameters
  declare_readonly_parameter( "port_name", port_name_, "Serial port name" );
  declare_readonly_parameter( "baud_rate", baud_rate_, "Serial baud rate" );
  declare_reconfigurable_parameter(
      "controller", controller_type_, "Controller type",
      hector::ParameterOptions<std::string>()
          .additionalConstraints( "Allowed values: diff_drive" )
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

The passed parameters are automatically initialized with the parameter value, defaulting to their current value.
For reconfigurable parameters, the parameters are also automatically updated.
Using the options, you have simplified full control over validation and update callbacks.
