cmake_minimum_required(VERSION 3.16)
project(diffdrive_deliverybot LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra)
endif()

# find dependencies
set(THIS_PACKAGE_INCLUDE_DEPENDS
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  zlac_ros2
)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(zlac_ros2 REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()


## COMPILE
add_library(
  diffdrive_deliverybot
  SHARED
  hardware/diffbot_system.cpp
)
target_compile_features(diffdrive_deliverybot PUBLIC cxx_std_17)
target_include_directories(diffdrive_deliverybot PUBLIC
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/hardware/include>
$<INSTALL_INTERFACE:include/diffdrive_deliverybot>
)
ament_target_dependencies(
  diffdrive_deliverybot PUBLIC zlac_ros2
  ${THIS_PACKAGE_INCLUDE_DEPENDS}
)

target_link_libraries(diffdrive_deliverybot PUBLIC)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME} PRIVATE "DIFFDRIVE_DELIVERYBOT_BUILDING_DLL")

# Export hardware plugins
pluginlib_export_plugin_description_file(hardware_interface diffdrive_deliverybot.xml)

# INSTALL
install(
  DIRECTORY hardware/include/
  DESTINATION include/diffdrive_deliverybot
)
install(
  DIRECTORY description/launch description/ros2_control description/urdf description/rviz
  DESTINATION share/diffdrive_deliverybot
)
install(
  DIRECTORY bringup/launch bringup/config
  DESTINATION share/diffdrive_deliverybot
)
install(TARGETS diffdrive_deliverybot
  EXPORT export_diffdrive_deliverybot
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
endif()

## EXPORTS
ament_export_targets(export_diffdrive_deliverybot HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
