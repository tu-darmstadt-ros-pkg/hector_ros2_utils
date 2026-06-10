cmake_minimum_required(VERSION 3.10)
project(hector_ros2_utils)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

add_library(hector_ros2_utils INTERFACE)
add_library(hector_ros2_utils::hector_ros2_utils ALIAS hector_ros2_utils)
target_include_directories(hector_ros2_utils INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(hector_ros2_utils INTERFACE
  rclcpp::rclcpp
  rclcpp_lifecycle::rclcpp_lifecycle
)

#############
## Install ##
#############

install(TARGETS hector_ros2_utils EXPORT hector_ros2_utils-targets)
install(
  EXPORT hector_ros2_utils-targets
  FILE hector_ros2_utils-targets.cmake
  NAMESPACE hector_ros2_utils::
  DESTINATION lib/cmake/hector_ros2_utils
)

install(DIRECTORY include/ DESTINATION include/)


#############
## Testing ##
#############

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  # Linting is handled by clang-format and cppcheck (see .pre-commit-config.yaml and CI),
  # so the ament linters are intentionally not run here.

  # Each test brings its own main (rclcpp::init/shutdown), so skip linking gtest_main.
  ament_add_gtest(test_uuidv4 test/test_uuidv4.cpp SKIP_LINKING_MAIN_LIBRARIES)
  target_link_libraries(test_uuidv4 hector_ros2_utils)

  ament_add_gtest(test_parameters test/test_parameters.cpp SKIP_LINKING_MAIN_LIBRARIES)
  target_link_libraries(test_parameters hector_ros2_utils)

  ament_add_gtest(test_node test/test_node.cpp SKIP_LINKING_MAIN_LIBRARIES)
  target_link_libraries(test_node hector_ros2_utils)

  ament_add_gtest(test_lifecycle_node test/test_lifecycle_node.cpp SKIP_LINKING_MAIN_LIBRARIES)
  target_link_libraries(test_lifecycle_node hector_ros2_utils)

  # Verifies the generated single-header export is self-contained and usable. The header is
  # generated into the build tree just for this test and only that directory (not the modular
  # headers) is on the include path, so the test proves the single header stands on its own.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(single_header_dir ${CMAKE_CURRENT_BINARY_DIR}/single_header)
  file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)
  add_custom_command(
    OUTPUT ${single_header_dir}/hector_ros2_utils.hpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../generate_single_header.py
      --output ${single_header_dir}/hector_ros2_utils.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../generate_single_header.py ${public_headers}
    COMMENT "Generating single header for test"
    VERBATIM
  )
  add_custom_target(test_single_header_gen
    DEPENDS ${single_header_dir}/hector_ros2_utils.hpp)

  ament_add_gtest(test_single_header test/test_single_header.cpp SKIP_LINKING_MAIN_LIBRARIES)
  add_dependencies(test_single_header test_single_header_gen)
  target_include_directories(test_single_header PRIVATE ${single_header_dir})
  target_link_libraries(test_single_header rclcpp::rclcpp rclcpp_lifecycle::rclcpp_lifecycle)

  # Including the single header together with the modular headers in one translation unit must
  # not produce redefinition conflicts (they share include guards).
  ament_add_gtest(test_single_header_coexistence test/test_single_header_coexistence.cpp
    SKIP_LINKING_MAIN_LIBRARIES)
  add_dependencies(test_single_header_coexistence test_single_header_gen)
  target_include_directories(test_single_header_coexistence PRIVATE ${single_header_dir})
  target_link_libraries(test_single_header_coexistence hector_ros2_utils)
endif()

ament_export_targets(hector_ros2_utils-targets)
ament_export_dependencies(rclcpp rclcpp_lifecycle)
ament_export_include_directories(include)
ament_package()
