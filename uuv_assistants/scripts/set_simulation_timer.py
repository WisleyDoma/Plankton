#!/usr/bin/env python3
# Copyright (c) 2016 The UUV Simulator Authors.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from __future__ import print_function
import rclpy


def main():
    rclpy.init().init_node()

    node = rclpy.create_node('set_simulation_timer')

    # if not rclpy.ok():
    #     rospy.ROSException('Something went wrong')

    timeout = 0.0
    if node.has_parameter('~timeout'):
        timeout = node.get_parameter('~timeout').get_parameter_value().double_value
        if timeout <= 0:
            raise RuntimeError('Termination time must be a positive floating point value')

    print('Starting simulation timer - Timeout = {} s'.format(timeout))
    rate = node.create_rate(100)
    #while rospy.get_time() < timeout:
    timeTuple = node.get_clock().now().seconds_nanoseconds
    currentTime = float(timeTuple[0]) + float(timeTuple[1]) / 1e9
    while currentTime < timeout
        rate.sleep()
        timeTuple = node.get_clock().now().seconds_nanoseconds
        currentTime = float(timeTuple[0]) + float(timeTuple[1]) / 1e9

    print('Simulation timeout - Killing simulation...')

if __name__ == '__main__':
    main()
