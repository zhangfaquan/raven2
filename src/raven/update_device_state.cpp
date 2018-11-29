/* Raven 2 Control - Control software for the Raven II robot
 * Copyright (C) 2005-2012  H. Hawkeye King, Blake Hannaford, and the University
 *of Washington BioRobotics Laboratory
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

#include "update_device_state.h"
#include "log.h"

extern DOF_type DOF_types[];
extern int NUM_MECH;
extern volatile int isUpdated;
extern unsigned long gTime;

unsigned int newDofTorqueSetting = 0;  // for setting torque from console
unsigned int newDofTorqueMech = 0;     // for setting torque from console
unsigned int newDofTorqueDof = 0;      //
unsigned int newDofPosSetting = 0;     // for setting torque from console
unsigned int newDofPosMech = 0;        // for setting torque from console
unsigned int newDofPosDof = 0;         //
int newDofTorqueTorque = 0;            // torque value in mNm
float newDofPosPos = 0;                // pos delta in rad or m
t_controlmode newRobotControlMode = homing_mode;

/**
 * updateDeviceState - Function that update the device state based on parameters
 *passed from
 *       the user interface
 *
 * \param params_current    the current set of parameters
 * \param arams_update      the new set of parameters
 * \param device0           pointer to device informaiton
 *
 */
int updateDeviceState(param_pass *currParams, param_pass *rcvdParams, device *device0) {
  currParams->last_sequence = rcvdParams->last_sequence;
  for (int i = 0; i < NUM_MECH; i++) {
    currParams->xd[i].x = rcvdParams->xd[i].x;
    currParams->xd[i].y = rcvdParams->xd[i].y;
    currParams->xd[i].z = rcvdParams->xd[i].z;
    currParams->rd[i].yaw = rcvdParams->rd[i].yaw;
    currParams->rd[i].pitch = rcvdParams->rd[i].pitch * WRIST_SCALE_FACTOR;
    currParams->rd[i].roll = rcvdParams->rd[i].roll;
    currParams->rd[i].grasp = rcvdParams->rd[i].grasp;
  }

  // set desired mech position in pedal_down runlevel
  if (currParams->runlevel == RL_PEDAL_DN) {
    for (int i = 0; i < NUM_MECH; i++) {
      device0->mech[i].pos_d.x = rcvdParams->xd[i].x;
      device0->mech[i].pos_d.y = rcvdParams->xd[i].y;
      device0->mech[i].pos_d.z = rcvdParams->xd[i].z;
      device0->mech[i].ori_d.grasp = rcvdParams->rd[i].grasp;



      for (int j = 0; j < 3; j++)
        for (int k = 0; k < 3; k++) device0->mech[i].ori_d.R[j][k] = rcvdParams->rd[i].R[j][k];
    }
  }

  //copy current joint positions to rcvd params so they can be passed to data1 for local_io kin calcs
  for (int i = 0; i < NUM_MECH; i++) {
    for(int j = 0; j < MAX_DOF_PER_MECH; j++)
      rcvdParams->jpos[i*MAX_DOF_PER_MECH + j] = device0->mech[i].joint[j].jpos;
  }


  //also pass teleop transform so they can be passed to data1 for local_io transforming
  for(int i = 0; i < NUM_MECH; i++){
    for(int j = 0; j < 4; j++){
       rcvdParams->teleop_tf_quat[i*4 + j] = device0->mech[i].teleop_transform[j];
    }
  }

  // Switch control modes only in pedal up or init.
  if ((currParams->runlevel == RL_E_STOP) &&
      (currParams->robotControlMode != (int)newRobotControlMode)) {
    currParams->robotControlMode = (int)newRobotControlMode;
    log_msg("Control mode updated");
  }

  // Set new torque command from console user input
  if (newDofTorqueSetting) {
    // reset all other joints to zero
    for (unsigned int idx = 0; idx < MAX_MECH_PER_DEV * MAX_DOF_PER_MECH; idx++) {
      if (idx == MAX_DOF_PER_MECH * newDofTorqueMech + newDofTorqueDof)
        currParams->torque_vals[idx] = newDofTorqueTorque;
      else
        currParams->torque_vals[idx] = 0;
    }
    newDofTorqueSetting = 0;
    log_msg("DOF Torque updated\n");
  }

  // Set new joint position command from console user input
  // robotControlMode for keyboard control of RAVEN is not yest set
  // TODO: reset to new keyboard mode when implemented
  if (newDofPosSetting && currParams->robotControlMode == -99) {
    // reset all other joints to zero
    for (unsigned int idx = 0; idx < MAX_MECH_PER_DEV * MAX_DOF_PER_MECH; idx++) {
      if (idx == MAX_DOF_PER_MECH * newDofPosMech + newDofPosDof)
        currParams->jpos_d[idx] += newDofPosPos;
      else
        currParams->jpos_d[idx] += 0;
    }
    newDofPosSetting = 0;
    log_msg("DOF Pos updated --- %f4\n",
            currParams->jpos_d[MAX_DOF_PER_MECH * newDofPosMech + newDofPosDof]);
  }

  // Set new surgeon mode
  // if (device0->crtk_state.get_pedal_trigger() == -1){
  //   device0->surgeon_mode = 1; // set pedal down
  //   device0->crtk_state.reset_pedal_trigger();
  // }
  // else if(device0->crtk_state.get_pedal_trigger() == 1){
  //   device0->surgeon_mode = 0; // set pedal up
  //   device0->crtk_state.reset_pedal_trigger();
  // }
  // else 
  if (device0->surgeon_mode != rcvdParams->surgeon_mode) {
    device0->surgeon_mode = rcvdParams->surgeon_mode;  // store the surgeon_mode to DS0
  }

  return 0;
}

/**
*  setRobotControlMode()
*       Change controller mode, i.e. position control, velocity control, visual
* servoing, etc
*   \param t_controlmode    current control mode.
*/
void setRobotControlMode(t_controlmode in_controlMode) {
  log_msg("Robot control mode: %d", in_controlMode);
  newRobotControlMode = in_controlMode;
  isUpdated = TRUE;
}

/**
*  setDofTorque()
*    Set a torque to output on a joint.
*     Torque input is mNm
*   \param in_mech      Mechinism number of the joint
*   \param in_dof       DOF number
*   \param in_torque    Torque to set the DOF to (in mNm)
*/
void setDofTorque(unsigned int in_mech, unsigned int in_dof, int in_torque) {
  if (((int)in_mech < NUM_MECH) && ((int)in_dof < MAX_DOF_PER_MECH)) {
    newDofTorqueMech = in_mech;
    newDofTorqueDof = in_dof;
    newDofTorqueTorque = in_torque;
    newDofTorqueSetting = 1;
  }
  isUpdated = TRUE;
}

/**
*  addDofPos()
*    Set a pos to output on a joint.
*
*   \param in_mech      Mechinism number of the joint
*   \param in_dof       DOF number
*   \param in_torque    value to add to joint angle - radians or meters
*/
void addDofPos(unsigned int in_mech, unsigned int in_dof, float in_pos) {
  if (((int)in_mech < NUM_MECH) && ((int)in_dof < MAX_DOF_PER_MECH)) {
    newDofPosMech = in_mech;
    newDofPosDof = in_dof;
    newDofPosPos = in_pos;
    newDofPosSetting = 1;
  }
  isUpdated = TRUE;
}


//TODO: maybe update something else on the device side
int update_motion_apis(device* dev){
    tf::Vector3 curr_pos;
    tf::Matrix3x3 curr_rot;
    tf::Transform curr_tf, new_tf;
    tf::Transform base_transform;

    // static int counter = 0;
    float r[9];
    // TODO: copy current to previous
    for(int i=0; i<2; i++){
      for(int j=0; j<9; j++){
        r[j] = dev->mech[i].ori.R[j/3][j%3];
      }
      curr_rot= tf::Matrix3x3(r[0],r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8]);
      curr_pos = tf::Vector3(dev->mech[i].pos.x/MICRON_PER_M,dev->mech[i].pos.y/MICRON_PER_M,dev->mech[i].pos.z/MICRON_PER_M);
      curr_tf = tf::Transform(curr_rot,curr_pos);
      // if(counter%1000 == 0){
      //   tf::Quaternion ttt = dev->crtk_motion_planner.crtk_motion_api[i].get_base_frame().getRotation();

      //   ROS_INFO("arm %i", i);
      //   ROS_INFO("base_frame: angle %f,\t axis %f\t%f\t%f.",ttt.getAngle(),ttt.getAxis().x(),ttt.getAxis().y(),ttt.getAxis().z());

      //   ROS_INFO("before: x %f,\t y %f,\t z %f",curr_tf.getOrigin().x(),curr_tf.getOrigin().y(),curr_tf.getOrigin().z());
      // }
      base_transform = dev->crtk_motion_planner.crtk_motion_api[i].get_base_frame();//.inverse();
      new_tf =  base_transform * curr_tf; 
      
      // if(counter%1000 == 0)
      //   ROS_INFO("after:  x %f,\t y %f,\t z %f\n",new_tf.getOrigin().x(),new_tf.getOrigin().y(),new_tf.getOrigin().z());
      dev->crtk_motion_planner.crtk_motion_api[i].set_pos(new_tf);
    }
    // counter ++;
    return 1;
}

/**
 * @brief      { function_description }
 *
 * @param      <unnamed>  { parameter_description }
 *
 * @return     { description_of_the_return_value }
 */
int update_device_crtk_motion(device* dev){
  CRTK_motion_type type;

  for(int i=0;i<2;i++){
    type = dev->crtk_motion_planner.crtk_motion_api[i].get_setpoint_out_type();    
    if(is_tf_type(type)){
      update_device_crtk_motion_tf(dev,i);
    }
    else if(is_js_type(type)){
      update_device_crtk_motion_js(dev,i);
    }
    else{
      continue;
    }
  }
  return 1;
 
}

int update_device_crtk_motion_tf(device* dev, int arm){

  float max_dist_per_ms = 0.0001;   // m/ms = 10 cm/s 
  float max_radian_per_ms = 0.001;  // rad/ms = 1 rad/s 
  // static int count = 0;
  int out = 1;
  CRTK_motion_type  type = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_type(); 
  tf::Quaternion curr_rot = tf::Transform(tf::Matrix3x3(dev->mech[arm].ori.R[0][0], dev->mech[arm].ori.R[0][1], dev->mech[arm].ori.R[0][2], 
                                                        dev->mech[arm].ori.R[1][0], dev->mech[arm].ori.R[1][1], dev->mech[arm].ori.R[1][2], 
                                                        dev->mech[arm].ori.R[2][0], dev->mech[arm].ori.R[2][1], dev->mech[arm].ori.R[2][2])).getRotation();
  switch(type){
    case CRTK_cr:{
      tf::Transform incr_tf = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_tf();

      // (1) for translation
      tf::Vector3 incr = incr_tf.getOrigin();
    
      // rotate to RAVEN frame and scale from meters to microns
      incr = dev->crtk_motion_planner.crtk_motion_api[arm].get_base_frame().inverse() * incr;
      incr = incr * MICRON_PER_M;
      

      if(incr.length() <= max_dist_per_ms * MICRON_PER_M){
        dev->mech[arm].pos_d.x += (int)(incr.x());
        dev->mech[arm].pos_d.y += (int)(incr.y());
        dev->mech[arm].pos_d.z += (int)(incr.z());
      }
      else{
        ROS_ERROR("Relative Cartesian translation too large. (length= %f m per ms)",incr.length()/MICRON_PER_M);
        out = 0;
      }
      

      // (2) for rotation!
      static int count = 0;
      count ++;

      static float incr_rot_angle_prev = 0;
      tf::Quaternion incr_rot = incr_tf.getRotation();
      // if(count % 250 == 0){
      //   ROS_INFO("incr quaternion before1: %f,%f,%f,%f", incr_rot.x(),incr_rot.y(),incr_rot.z(), incr_rot.w());
      // }
      tf::Vector3 incr_rot_axis = incr_rot.getAxis().normalize();
      
      float incr_rot_angle = incr_rot.getAngle();

      // if(incr_rot_angle_prev != incr_rot_angle){
      //   ROS_INFO("switching axis: %f", incr_rot_angle);
      //   incr_rot_angle_prev = incr_rot_angle;
      // }

      // if(count % 250 == 0){
      //   ROS_INFO("incr quaternion before2: %f,%f,%f,%f", incr_rot.x(),incr_rot.y(),incr_rot.z(), incr_rot.w());
      //   ROS_INFO("incr axis before: %f,%f,%f angle %f", incr_rot_axis.x(),incr_rot_axis.y(),incr_rot_axis.z(), incr_rot_angle);
      // }
      // incr_rot_axis = dev->crtk_motion_planner.crtk_motion_api[arm].get_base_frame().inverse().getBasis() * incr_rot_axis;
      // incr_rot = tf::Quaternion(incr_rot_axis,incr_rot_angle);
      // tf::Transform incr_rot_mx = tf::Transform(curr_rot)*tf::Transform(incr_rot);  
      // if(count % 250 == 0){   
      //   ROS_INFO("incr axis after: \t %f,%f,%f angle %f", incr_rot_axis.x(),incr_rot_axis.y(),incr_rot_axis.z(), incr_rot_angle);
      //   ROS_INFO("angle diff = %f\n",curr_rot.angle(incr_rot_mx.getRotation()));
      // }
      
      // if(count % 500 == 0){
      //   // ROS_INFO("incr quaternion after: %f,%f,%f,%f", incr_rot.x(),incr_rot.y(),incr_rot.z(), incr_rot.w());
      //   ROS_INFO("\n incr_rot_length = %f \t !isnan = %i ", incr_rot.length(), !isnan(incr_rot.length()));
      //   ROS_INFO("incr_rot.getAngle = %f \t max_rad_per_ms = %f \n", incr_rot.getAngle(), max_radian_per_ms);
      // }


      tf::Matrix3x3 t1 = dev->crtk_motion_planner.crtk_motion_api[arm].get_base_frame().inverse().getBasis();
      tf::Matrix3x3 t2 = incr_tf.getBasis();
      tf::Matrix3x3 q_temp = t1*t2*t1.inverse();
      tf::Matrix3x3 mx_temp = tf::Transform(tf::Matrix3x3(dev->mech[arm].ori_d.R[0][0], dev->mech[arm].ori_d.R[0][1], dev->mech[arm].ori_d.R[0][2], 
                                                        dev->mech[arm].ori_d.R[1][0], dev->mech[arm].ori_d.R[1][1], dev->mech[arm].ori_d.R[1][2], 
                                                        dev->mech[arm].ori_d.R[2][0], dev->mech[arm].ori_d.R[2][1], dev->mech[arm].ori_d.R[2][2])).getBasis();
      // double R,P,Y;
      // if(count % 500 == 0){
      //   incr_tf.getBasis().getRPY(R,P,Y);
      //   ROS_INFO("incr_tf (before transform)   : roll %f, pitch %f, yaw %f", R,P,Y);
      //   // ROS_INFO("incr_tf: %f,%f,%f,%f", incr_tf.getRotation().x(),incr_tf.getRotation().y(),incr_tf.getRotation().z(), incr_tf.getRotation().w());
        
      //   q_temp.getRPY(R,P,Y);
      //   ROS_INFO("q_temp (after transform)     : roll %f, pitch %f, yaw %f", R,P,Y);
      //   // ROS_INFO("q_temp: %f,%f,%f,%f", q_temp.x(),q_temp.y(),q_temp.z(), q_temp.w());
      // }

      if (tf::Transform(t2).getRotation() != tf::Quaternion::getIdentity()) {
        q_temp = q_temp * mx_temp;
        tf::Matrix3x3 rot_mx_temp(q_temp);


        for (int j = 0; j < 3; j++)
          for (int k = 0; k < 3; k++) 
            dev->mech[arm].ori_d.R[j][k] = rot_mx_temp[j][k];

        tf::Quaternion test = tf::Transform(rot_mx_temp).getRotation();

        // if(count % 500 == 0){
        //   ROS_INFO("matrix quaternion = %f,%f,%f,%f ,angle = %f\n", test.x(),test.y(),test.z(),test.w(),test.getAngle());
        // }
      }
      // if(incr_rot.length() > 0 && !isnan(incr_rot.length())){
      //   if(incr_rot.getAngle() <= max_radian_per_ms){
      //     tf::Matrix3x3 incr_rot_mx_mx = incr_rot_mx.getBasis();
      //     tf::Quaternion test = tf::Transform(incr_rot_mx_mx).getRotation();

      //     if(count % 500 == 0){
      //       ROS_INFO("matrix quaternion = %f,%f,%f,%f ,angle = %f\n", test.x(),test.y(),test.z(),test.w(),test.getAngle());
      //     }

      //     for(int i=0; i<3 ; i++){
      //       for(int j=0;j<3;j++){
      //         dev->mech[arm].ori_d.R[i][j] = incr_rot_mx_mx[i][j];
      //       }
      //     }
      //   }
      //   else{
      //     ROS_ERROR("Cartesian Relative rotation too large. (angle= %f rad per ms)",incr_rot.getAngle());
      //     out = 0;
      //   }
      // }
      else{
        out = 0;
      }

      dev->crtk_motion_planner.crtk_motion_api[arm].reset_setpoint_out(); 
      return out;
      break;
    }
    case CRTK_cp:{
      tf::Vector3 new_pos = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_tf().getOrigin();
      tf::Vector3 curr_pos = tf::Vector3(dev->mech[arm].pos.x,dev->mech[arm].pos.y,dev->mech[arm].pos.z);

      tf::Quaternion new_rot = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_tf().getRotation();
      
      // rotate to RAVEN frame and scale from meters to microns
      new_pos = dev->crtk_motion_planner.crtk_motion_api[arm].get_base_frame().inverse() * new_pos;
      new_pos = new_pos * MICRON_PER_M;
;
      if(fabs((new_pos-curr_pos).length()) <= max_dist_per_ms * MICRON_PER_M){
        dev->mech[arm].pos_d.x = new_pos.x();
        dev->mech[arm].pos_d.y = new_pos.y();
        dev->mech[arm].pos_d.z = new_pos.z();
        dev->crtk_motion_planner.crtk_motion_api[arm].reset_setpoint_out(); 
      }
      else{
        ROS_ERROR("Absolute Cartesian step size too large. (step length=%f micron per ms)",fabs((new_pos-curr_pos).length()));
        out = 0;
      }
      if(new_rot.length() > 0 && !isnan(new_rot.length())){
        if(new_rot.angle(curr_rot) <= 0.5*max_radian_per_ms){
          tf::Matrix3x3 incr_rot_mx_mx = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_tf().getBasis();
          for(int i=0; i<3 ; i++)
            for(int j=0;j<3;j++)
              dev->mech[arm].ori_d.R[i][j] = incr_rot_mx_mx[i][j];
        }
        else{
          ROS_ERROR("Absolute Cartesian rotation too large. (angle= %f rad per ms)",new_rot.angle(curr_rot));
          out = 0;
        }
      }
      else{
        out = 0;
      }
      
      return out;
      break;
    }
    case CRTK_cv:{
      // TODO later:D
      break;
    }
    default:{
      ROS_ERROR("Unsupported motion tf type.");
      return 0;
    }
  }

  return 0;
}

int update_device_crtk_motion_js(device* dev, int arm){

  // CRTK_motion_type  type = dev->crtk_motion_planner.crtk_motion_api[arm].get_setpoint_out_type(); 
  // TODO later:D
  return 0;
}