/* Raven 2 Control - Control software for the Raven II robot
 * Copyright (C) 2005-2018  Andrew Lewis, Yun-Hsuan Su, Blake Hannaford, 
 * and the University of Washington BioRobotics Laboratory
 *
 * This file is part of Raven 2 Control.
 *
 * Raven 2 Control is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Raven 2 Control is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Raven 2 Control.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * crtk_motion_api.h
 *
 * \brief Class file for CRTK state object, which holds all of the state
 *  flags for the "Robot Operating State" aspect of the CRTK API
 *
 *  \date Oct 25, 2018
 *  \author Andrew Lewis, Yun-Hsuan Su

 */

#include <ros/ros.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <sensor_msgs/JointState.h>
#include <tf/tf.h>

#ifndef CRTK_MOTION_API_H_
#define CRTK_MOTION_API_H_

enum CRTK_motion_level {CRTK_interp, CRTK_move, CRTK_servo, CRTK_NULL_level};
enum CRTK_motion_type {CRTK_jr, CRTK_cr, CRTK_jp, CRTK_cp, CRTK_jv, CRTK_cv, CRTK_jf, CRTK_cf, CRTK_NULL_type};
    
struct CRTK_goal
{
    char updated; 
    float jr[7];
    float jp[7];
    float jv[7];
    float jf[7];

    tf::Transform cr;
    tf::Transform cp;
    tf::Transform cv;
    tf::Transform cf;
};

class CRTK_motion_api 
{
  public:
    CRTK_motion_api();
    CRTK_motion_api(char);
    ~CRTK_motion_api(){};
    void crtk_servo_cr_cb(geometry_msgs::TransformStamped);

    char check_updates();       // START HERE TODAY!!!!!!                       

    tf::Transform get_pos();
    char set_pos(tf::Transform);
    //tf::Transform get_vel();
    void set_jpos(float*);
    //void set_jvel(float*);

    void set_base_frame(tf::Transform);
    void set_default_base_frame(char);
    tf::Transform get_base_frame();

    void transfer_data(CRTK_motion_api*);
    char preempt_to_network(CRTK_motion_api*);
    char network_to_preempt(CRTK_motion_api*);

    void set_goal_in(CRTK_motion_level, CRTK_goal);
    void set_goal_in(CRTK_motion_level, CRTK_motion_type, tf::Transform);
    void set_goal_in(CRTK_motion_level, CRTK_motion_type, float*);
    void set_setpoint_in(CRTK_motion_level, CRTK_goal);
    void set_setpoint_in(CRTK_motion_level, CRTK_motion_type, tf::Transform);
    void set_setpoint_in(CRTK_motion_level, CRTK_motion_type, float*);

    CRTK_goal get_goal_in(CRTK_motion_level);
    tf::Transform get_goal_in_tf(CRTK_motion_level, CRTK_motion_type);
    tf::Transform get_setpoint_in_tf(CRTK_motion_level, CRTK_motion_type);
    float* get_goal_in_js(CRTK_motion_level, CRTK_motion_type);
    float* get_setpoint_in_js(CRTK_motion_level, CRTK_motion_type);

    char set_setpoint_out_tf(CRTK_motion_level, CRTK_motion_type, tf::Transform);                // TODO: call this from motion planner
    void set_setpoint_out_js(sensor_msgs::JointState*);
    char set_setpoint_out_js(CRTK_motion_level, CRTK_motion_type, float*);     // TODO: call this from motion planner
    char set_goal_out_tf(CRTK_motion_level, CRTK_motion_type, tf::Transform);                    // TODO: call this from motion planner
    void set_goal_out_js(sensor_msgs::JointState*);
    char set_goal_out_js(CRTK_motion_level, CRTK_motion_type, float*);         // TODO: call this from motion planner

    
    CRTK_goal get_setpoint_in(CRTK_motion_level);
    
    tf::Transform get_setpoint_out_tf();            // TODO: call this from motion planner
    sensor_msgs::JointState get_setpoint_out_js();  // TODO: call this from motion planner
    tf::Transform get_goal_out_tf();                // TODO: call this from motion planner
    sensor_msgs::JointState get_goal_out_js();      // TODO: call this from motion planner

  private:
    // current robot pose
    tf::Transform r0_transform;
    tf::Transform pos; // measured_cp
    //tf::Transform vel;
    float jpos[7];
    //float jvel[7];

    CRTK_goal goal_in[2];       // for interpolation, move
    CRTK_goal setpoint_in[3];   // for interpolation, move, servo

    sensor_msgs::JointState goal_out_js;
    sensor_msgs::JointState setpoint_out_js;
    tf::Transform goal_out_tf;
    tf::Transform setpoint_out_tf;

    CRTK_motion_level   goal_out_level;
    CRTK_motion_type    goal_out_type;
    CRTK_motion_level   setpoint_out_level;
    CRTK_motion_type    setpoint_out_type;

    void reset_goal_in();      // for interpolation, move
    void reset_setpoint_in();  // for interpolation, move, servo
    void reset_goal_out();      
    void reset_setpoint_out();  

};



char is_tf_type(CRTK_motion_type);
char is_js_type(CRTK_motion_type);
char is_type_valid(CRTK_motion_level, CRTK_motion_type);

#endif