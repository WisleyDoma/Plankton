// Copyright (c) 2016 The UUV Simulator Authors.
// All rights reserved.
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

#include <uuv_world_ros_plugins/UnderwaterCurrentROSPlugin.h>

namespace uuv_simulator_ros
{
/////////////////////////////////////////////////
UnderwaterCurrentROSPlugin::UnderwaterCurrentROSPlugin()
{
  this->rosPublishPeriod = gazebo::common::Time(0.05);
  this->lastRosPublishTime = gazebo::common::Time(0.0);
}

/////////////////////////////////////////////////
UnderwaterCurrentROSPlugin::~UnderwaterCurrentROSPlugin()
{
#if GAZEBO_MAJOR_VERSION >= 8
  this->rosPublishConnection.reset();
#else
  gazebo::event::Events::DisconnectWorldUpdateBegin(
    this->rosPublishConnection);
#endif

  //this->rosNode->shutdown();
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::Load(gazebo::physics::WorldPtr _world,
    sdf::ElementPtr _sdf)
{
  using std::placeholders::_1;
  using std::placeholders::_2;

  try
  {
    UnderwaterCurrentPlugin::Load(_world, _sdf);
  } catch(gazebo::common::Exception &_e)
  {
    gzerr << "Error loading plugin."
          << "Please ensure that your model is correct."
          << '\n';
    return;
  }

  if (!rclcpp::is_initialized())
  {
    gzerr << "Not loading plugin since ROS has not been "
          << "properly initialized.  Try starting gazebo with ros plugin:\n"
          << "  gazebo -s libgazebo_ros_api_plugin.so\n";
    return;
  }

  this->ns = "";
  if (_sdf->HasElement("namespace"))
    this->ns = _sdf->Get<std::string>("namespace");

  gzmsg << "UnderwaterCurrentROSPlugin::namespace=" << this->ns << std::endl;

  //Node's namespace is set automatically from the sdf file
  myRosNode =  gazebo_ros::Node::Get(_sdf);//rclcpp::Node::make_unique(this->ns);
  //auto nsNode = myRosNode->create_sub_node(this->ns); //The 
  //this->rosNode.reset(new ros::NodeHandle(this->ns));

  // Advertise the flow velocity as a stamped twist message
  myFlowVelocityPub = myRosNode->create_publisher<geometry_msgs::msg::TwistStamped>(
    this->currentVelocityTopic, 10);

  // Advertise the service to update the current velocity model
  this->worldServices["set_current_velocity_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentModel>(
      "set_current_velocity_model",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateCurrentVelocityModel, this, _1, _2));

  // Advertise the service to update the current velocity model
  this->worldServices["get_current_velocity_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::GetCurrentModel>(
      "get_current_velocity_model",
      std::bind(&UnderwaterCurrentROSPlugin::GetCurrentVelocityModel, this, _1, _2));

  // Advertise the service to update the current velocity model
  this->worldServices["set_current_horz_angle_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentModel>(
      "set_current_horz_angle_model",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateCurrentHorzAngleModel, this, _1, _2));

  // Advertise the service to update the current velocity model
  this->worldServices["get_current_horz_angle_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::GetCurrentModel>(
      "get_current_horz_angle_model",
      std::bind(&UnderwaterCurrentROSPlugin::GetCurrentHorzAngleModel, this, _1, _2));

  // Advertise the service to update the current velocity model
  this->worldServices["set_current_vert_angle_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentModel>(
      "set_current_vert_angle_model",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateCurrentVertAngleModel, this, _1, _2));

  // Advertise the service to update the current velocity model
  //TODO It was refered as GetCurrentHorzAngleModel in the orig code. Probably a bug ?
  this->worldServices["get_current_vert_angle_model"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::GetCurrentModel>(
      "get_current_vert_angle_model",
      std::bind(&UnderwaterCurrentROSPlugin::GetCurrentVertAngleModel, this, _1, _2));

  // Advertise the service to update the current velocity mean value
  this->worldServices["set_current_velocity"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentVelocity>(
      "set_current_velocity",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateCurrentVelocity, this, _1, _2));

  // Advertise the service to update the current velocity mean value
  this->worldServices["set_current_horz_angle"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentDirection>(
      "set_current_horz_angle",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateHorzAngle, this, _1, _2));

  // Advertise the service to update the current velocity mean value
  this->worldServices["set_current_vert_angle"] =
    myRosNode->create_service<uuv_world_ros_plugins_msgs::srv::SetCurrentDirection>(
      "set_current_vert_angle",
      std::bind(&UnderwaterCurrentROSPlugin::UpdateVertAngle, this, _1, _2));

  this->rosPublishConnection = gazebo::event::Events::ConnectWorldUpdateBegin(
    std::bind(&UnderwaterCurrentROSPlugin::OnUpdateCurrentVel, this));
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::OnUpdateCurrentVel()
{
  if (this->lastUpdate - this->lastRosPublishTime >= this->rosPublishPeriod)
  {
    this->lastRosPublishTime = this->lastUpdate;
    geometry_msgs::msg::TwistStamped flowVelMsg;
    flowVelMsg.header.stamp = myRosNode->now();//::Time().now();
    flowVelMsg.header.frame_id = "/world";

    flowVelMsg.twist.linear.x = this->currentVelocity.X();
    flowVelMsg.twist.linear.y = this->currentVelocity.Y();
    flowVelMsg.twist.linear.z = this->currentVelocity.Z();

    myFlowVelocityPub->publish(flowVelMsg);
  }
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateHorzAngle(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentDirection::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentDirection::Response::SharedPtr _res)
{
  _res->success = this->currentHorzAngleModel.SetMean(_req->angle);
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateVertAngle(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentDirection::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentDirection::Response::SharedPtr _res)
{
  _res->success = this->currentVertAngleModel.SetMean(_req->angle);
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateCurrentVelocity(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentVelocity::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentVelocity::Response::SharedPtr _res)
{
  if (this->currentVelModel.SetMean(_req->velocity) &&
      this->currentHorzAngleModel.SetMean(_req->horizontal_angle) &&
      this->currentVertAngleModel.SetMean(_req->vertical_angle))
  {
    gzmsg << "Current velocity [m/s] = " << _req->velocity << std::endl
      << "Current horizontal angle [rad] = " << _req->horizontal_angle
      << std::endl
      << "Current vertical angle [rad] = " << _req->vertical_angle
      << std::endl
      << "\tWARNING: Current velocity calculated in the ENU frame"
      << std::endl;
    _res->success = true;
  }
  else
  {
    gzmsg << "Error while updating the current velocity" << std::endl;
    _res->success = false;
  }
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::GetCurrentVelocityModel(
    const uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Request::SharedPtr /*_req*/,
    uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Response::SharedPtr _res)
{
  _res->mean = this->currentVelModel.mean;
  _res->min = this->currentVelModel.min;
  _res->max = this->currentVelModel.max;
  _res->noise = this->currentVelModel.noiseAmp;
  _res->mu = this->currentVelModel.mu;
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::GetCurrentHorzAngleModel(
    const uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Request::SharedPtr /*_req*/,
    uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Response::SharedPtr _res)
{
  _res->mean = this->currentHorzAngleModel.mean;
  _res->min = this->currentHorzAngleModel.min;
  _res->max = this->currentHorzAngleModel.max;
  _res->noise = this->currentHorzAngleModel.noiseAmp;
  _res->mu = this->currentHorzAngleModel.mu;
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::GetCurrentVertAngleModel(
    const uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Request::SharedPtr /*_req*/,
    uuv_world_ros_plugins_msgs::srv::GetCurrentModel::Response::SharedPtr _res)
{
  _res->mean = this->currentVertAngleModel.mean;
  _res->min = this->currentVertAngleModel.min;
  _res->max = this->currentVertAngleModel.max;
  _res->noise = this->currentVertAngleModel.noiseAmp;
  _res->mu = this->currentVertAngleModel.mu;
}


/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateCurrentVelocityModel(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Response::SharedPtr _res)
{
  _res->success = this->currentVelModel.SetModel(
    std::max(0.0, _req->mean),
    std::max(0.0, _req->min),
    std::max(0.0, _req->max),
    _req->mu,
    _req->noise);
  gzmsg << "Current velocity model updated" << std::endl
    << "\tWARNING: Current velocity calculated in the ENU frame"
    << std::endl;
  this->currentVelModel.Print();
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateCurrentHorzAngleModel(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Response::SharedPtr _res)
{
  _res->success = this->currentHorzAngleModel.SetModel(_req->mean, _req->min,
    _req->max, _req->mu, _req->noise);
  gzmsg << "Horizontal angle model updated" << std::endl
    << "\tWARNING: Current velocity calculated in the ENU frame"
    << std::endl;
  this->currentHorzAngleModel.Print();
}

/////////////////////////////////////////////////
void UnderwaterCurrentROSPlugin::UpdateCurrentVertAngleModel(
    const uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Request::SharedPtr _req,
    uuv_world_ros_plugins_msgs::srv::SetCurrentModel::Response::SharedPtr _res)
{
  _res->success = this->currentVertAngleModel.SetModel(_req->mean, _req->min,
    _req->max, _req->mu, _req->noise);
  gzmsg << "Vertical angle model updated" << std::endl
    << "\tWARNING: Current velocity calculated in the ENU frame"
    << std::endl;
  this->currentVertAngleModel.Print();
}

/////////////////////////////////////////////////
GZ_REGISTER_WORLD_PLUGIN(UnderwaterCurrentROSPlugin)
}