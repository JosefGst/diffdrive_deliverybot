# :robot: diffdrive_controller_zlac_ros2

   *DiffBot*, or ''Differential Mobile Robot'', is a simple mobile base with differential drive.
   The robot is basically a box moving according to differential drive kinematics.

Find the documentation in [doc/userdoc.rst](doc/userdoc.rst) or on [control.ros.org](https://control.ros.org/master/doc/ros2_control_demos/example_2/doc/userdoc.html).

## :hammer_and_wrench: Setup
This package builds on top of [zlac_ros2](https://github.com/JosefGst/zlac_ros2) motor driver library. 

```
cd diffdrive_deliverybot
sudo apt install python3-vcstool
vcs import .. < my.repos
sudo apt install python3-rosdep2 && rosdep update && rosdep install -i --from-path src --rosdistro $ROS_DISTRO -y
```
`colcon build` workspace

## :rocket: Usage

```
ros2 launch diffdrive_deliverybot diffbot.launch.py 
```

## :construction_worker: TODO:
- [ ] rename pkg to diffdrive_zlac_ros2
- [ ] make it work in simulation and with real hardware by simply changing a parameter
- [ ] add gif
- [ ] git workflow
- [ ] dockerize
- [ ] :exclamation:not tested