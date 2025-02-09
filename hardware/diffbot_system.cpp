// Copyright 2021 ros2_control Development Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "diffdrive_deliverybot/diffbot_system.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace diffdrive_deliverybot
{
  hardware_interface::CallbackReturn DiffBotSystemHardware::on_init(
      const hardware_interface::HardwareInfo &info)
  {
    if (
        hardware_interface::SystemInterface::on_init(info) !=
        hardware_interface::CallbackReturn::SUCCESS)
    {
      return hardware_interface::CallbackReturn::ERROR;
    }

    hw_positions_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
    hw_velocities_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
    hw_commands_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());

    for (const hardware_interface::ComponentInfo &joint : info_.joints)
    {
      // DiffBotSystem has exactly two states and one command interface on each joint
      if (joint.command_interfaces.size() != 1)
      {
        RCLCPP_FATAL(
            rclcpp::get_logger("DiffBotSystemHardware"),
            "Joint '%s' has %zu command interfaces found. 1 expected.", joint.name.c_str(),
            joint.command_interfaces.size());
        return hardware_interface::CallbackReturn::ERROR;
      }

      if (joint.command_interfaces[0].name != hardware_interface::HW_IF_VELOCITY)
      {
        RCLCPP_FATAL(
            rclcpp::get_logger("DiffBotSystemHardware"),
            "Joint '%s' have %s command interfaces found. '%s' expected.", joint.name.c_str(),
            joint.command_interfaces[0].name.c_str(), hardware_interface::HW_IF_VELOCITY);
        return hardware_interface::CallbackReturn::ERROR;
      }

      if (joint.state_interfaces.size() != 2)
      {
        RCLCPP_FATAL(
            rclcpp::get_logger("DiffBotSystemHardware"),
            "Joint '%s' has %zu state interface. 2 expected.", joint.name.c_str(),
            joint.state_interfaces.size());
        return hardware_interface::CallbackReturn::ERROR;
      }

      if (joint.state_interfaces[0].name != hardware_interface::HW_IF_POSITION)
      {
        RCLCPP_FATAL(
            rclcpp::get_logger("DiffBotSystemHardware"),
            "Joint '%s' have '%s' as first state interface. '%s' expected.", joint.name.c_str(),
            joint.state_interfaces[0].name.c_str(), hardware_interface::HW_IF_POSITION);
        return hardware_interface::CallbackReturn::ERROR;
      }

      if (joint.state_interfaces[1].name != hardware_interface::HW_IF_VELOCITY)
      {
        RCLCPP_FATAL(
            rclcpp::get_logger("DiffBotSystemHardware"),
            "Joint '%s' have '%s' as second state interface. '%s' expected.", joint.name.c_str(),
            joint.state_interfaces[1].name.c_str(), hardware_interface::HW_IF_VELOCITY);
        return hardware_interface::CallbackReturn::ERROR;
      }
    }

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  std::vector<hardware_interface::StateInterface> DiffBotSystemHardware::export_state_interfaces()
  {
    std::vector<hardware_interface::StateInterface> state_interfaces;
    for (auto i = 0u; i < info_.joints.size(); i++)
    {
      state_interfaces.emplace_back(hardware_interface::StateInterface(
          info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_positions_[i]));
      state_interfaces.emplace_back(hardware_interface::StateInterface(
          info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_velocities_[i]));
    }

    return state_interfaces;
  }

  std::vector<hardware_interface::CommandInterface> DiffBotSystemHardware::export_command_interfaces()
  {
    std::vector<hardware_interface::CommandInterface> command_interfaces;
    for (auto i = 0u; i < info_.joints.size(); i++)
    {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(
          info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_commands_[i]));
    }

    return command_interfaces;
  }

  hardware_interface::CallbackReturn DiffBotSystemHardware::on_configure(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Configuring ...please wait...");
    int kp = 750;
    int ki = 100;
    int acc_time = 1;
    int dec_time = 1;
    int max_speed = 100;
    
    motorL.begin("/dev/zlac", 115200, 0x01);
    motorL.set_vel_mode();

    motorL.set_acc_time(acc_time);
    motorL.set_decc_time(dec_time);
    motorL.set_kp(kp);
    motorL.set_ki(ki);
    motorL.max_speed(max_speed);
    motorL.enable();

    motorR.begin("/dev/zlac", 115200, 0x02);
    motorR.set_vel_mode();
    if (motorR.set_acc_time(acc_time))
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "set acc time ERROR");
    if (motorR.set_decc_time(dec_time))
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "set decc time ERROR");
    if (motorR.set_kp(kp))
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "set kp ERROR");
    if (motorR.set_ki(ki))
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "set ki ERROR");
    motorR.max_speed(max_speed);
    motorR.enable();

    motors_enabled = true;
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "motors_enabled '%d'" , motors_enabled);

    for (auto i = 0u; i < hw_positions_.size(); i++)
    {
      if (std::isnan(hw_positions_[i]))
      {
        hw_positions_[i] = 0;
        hw_velocities_[i] = 0;
        hw_commands_[i] = 0;
      }
    }

    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Successfully configured!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn DiffBotSystemHardware::on_cleanup(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Cleaning up ...please wait...");
    motorL.disable();
    motorR.disable();
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Successfully cleaned!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn DiffBotSystemHardware::on_activate(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Activating ...please wait...");

    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Successfully activated!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn DiffBotSystemHardware::on_deactivate(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Deactivating ...please wait...");
    motorL.disable();
    motorR.disable();
    RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "Successfully deactivated!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::return_type DiffBotSystemHardware::read(
      const rclcpp::Time & /*time*/, const rclcpp::Duration &period)
  {
    if (motorL.read_motor() == 0)
    {
      hw_velocities_[0] = motorL.get_rpm() * 0.1047198;                // rpm2rps
      hw_positions_[0] = float(motorL.get_position()) / 4096 * 6.2832; // actual counts / counts per rev * rev2rad
    }
    else
    {
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "L motor crc check error!");
    }
    if (motorR.read_motor() == 0)
    {
      hw_velocities_[1] = motorR.get_rpm() * -0.1047198; // rpm2rps REVERSE right wheel
      hw_positions_[1] = float(motorR.get_position()) / 4096 * -6.2832;
    }
    else
    {
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "R motor crc check error!");
    }

    // BEGIN: This part here is for exemplary purposes - Please do not copy to your production code
    // for (std::size_t i = 0; i < hw_velocities_.size(); i++)
    // {
    //   // Simulate DiffBot wheels's movement as a first-order system
    //   // Update the joint status: this is a revolute joint without any limit.
    //   // Simply integrates
    //   hw_positions_[i] = hw_positions_[i] + period.seconds() * hw_velocities_[i];

    //   // RCLCPP_INFO(
    //   //     rclcpp::get_logger("DiffBotSystemHardware"),
    //   //     "Got position state %.5f and velocity state %.5f for '%s'!", hw_positions_[i],
    //   //     hw_velocities_[i], info_.joints[i].name.c_str());
    // }
    // END: This part here is for exemplary purposes - Please do not copy to your production code

    return hardware_interface::return_type::OK;
  }

  hardware_interface::return_type diffdrive_deliverybot ::DiffBotSystemHardware::write(
      const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
  {
    // check if motors receive commands -> enable motors
    if(motors_enabled && hw_commands_[0]==0.0 && hw_commands_[1]==0.0){
      motorL.disable();
      motorR.disable();
      motors_enabled = false;
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "disable motors: %d", motors_enabled);
    }
    // if one wheel receives a command turn on motors
    else if (!motors_enabled && (hw_commands_[0]!=0.0 || hw_commands_[1]!=0.0))
    {
      motorL.enable();
      motorR.enable();
      motors_enabled = true;
      RCLCPP_INFO(rclcpp::get_logger("DiffBotSystemHardware"), "enable motors: %d", motors_enabled);
    }
    if(motors_enabled){
      motorL.set_rpm(hw_commands_[0] * 9.5492968);  // rps2rpm Motor expect rpm command
      motorR.set_rpm(hw_commands_[1] * -9.5492968); // rps2rpm Motor expect rpm command REVERSE direction of right wheel
    }
    
    return hardware_interface::return_type::OK;
  }

} // namespace diffdrive_deliverybot

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
    diffdrive_deliverybot::DiffBotSystemHardware, hardware_interface::SystemInterface)
