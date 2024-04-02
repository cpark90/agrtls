#ifndef _ROS_mobileye_560_660_msgs_ObstacleData_h
#define _ROS_mobileye_560_660_msgs_ObstacleData_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class ObstacleData : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint16_t _obstacle_id_type;
      _obstacle_id_type obstacle_id;
      typedef float _obstacle_pos_x_type;
      _obstacle_pos_x_type obstacle_pos_x;
      typedef float _obstacle_pos_y_type;
      _obstacle_pos_y_type obstacle_pos_y;
      typedef uint8_t _blinker_info_type;
      _blinker_info_type blinker_info;
      typedef uint8_t _cut_in_and_out_type;
      _cut_in_and_out_type cut_in_and_out;
      typedef float _obstacle_rel_vel_x_type;
      _obstacle_rel_vel_x_type obstacle_rel_vel_x;
      typedef uint8_t _obstacle_type_type;
      _obstacle_type_type obstacle_type;
      typedef uint8_t _obstacle_status_type;
      _obstacle_status_type obstacle_status;
      typedef bool _obstacle_brake_lights_type;
      _obstacle_brake_lights_type obstacle_brake_lights;
      typedef uint8_t _obstacle_valid_type;
      _obstacle_valid_type obstacle_valid;
      typedef float _obstacle_length_type;
      _obstacle_length_type obstacle_length;
      typedef float _obstacle_width_type;
      _obstacle_width_type obstacle_width;
      typedef uint16_t _obstacle_age_type;
      _obstacle_age_type obstacle_age;
      typedef uint8_t _obstacle_lane_type;
      _obstacle_lane_type obstacle_lane;
      typedef bool _cipv_flag_type;
      _cipv_flag_type cipv_flag;
      typedef float _radar_pos_x_type;
      _radar_pos_x_type radar_pos_x;
      typedef float _radar_vel_x_type;
      _radar_vel_x_type radar_vel_x;
      typedef uint8_t _radar_match_confidence_type;
      _radar_match_confidence_type radar_match_confidence;
      typedef uint16_t _matched_radar_id_type;
      _matched_radar_id_type matched_radar_id;
      typedef float _obstacle_angle_rate_type;
      _obstacle_angle_rate_type obstacle_angle_rate;
      typedef float _obstacle_scale_change_type;
      _obstacle_scale_change_type obstacle_scale_change;
      typedef float _object_accel_x_type;
      _object_accel_x_type object_accel_x;
      typedef bool _obstacle_replaced_type;
      _obstacle_replaced_type obstacle_replaced;
      typedef float _obstacle_angle_type;
      _obstacle_angle_type obstacle_angle;
      enum { BLINKER_INFO_UNAVAILABLE =  0 };
      enum { BLINKER_INFO_OFF =  1 };
      enum { BLINKER_INFO_LEFT =  2 };
      enum { BLINKER_INFO_RIGHT =  3 };
      enum { BLINKER_INFO_BOTH =  4 };
      enum { CUT_IN_AND_OUT_UNDEFINED =  0 };
      enum { CUT_IN_AND_OUT_IN_HOST_LANE =  1 };
      enum { CUT_IN_AND_OUT_OUT_HOST_LANE =  2 };
      enum { CUT_IN_AND_OUT_CUT_IN =  3 };
      enum { CUT_IN_AND_OUT_CUT_OUT =  4 };
      enum { OBSTACLE_TYPE_VEHICLE =  0 };
      enum { OBSTACLE_TYPE_TRUCK =  1 };
      enum { OBSTACLE_TYPE_BIKE =  2 };
      enum { OBSTACLE_TYPE_PED =  3 };
      enum { OBSTACLE_TYPE_BICYCLE =  4 };
      enum { OBSTACLE_STATUS_UNDEFINED =  0 };
      enum { OBSTACLE_STATUS_STANDING =  1 };
      enum { OBSTACLE_STATUS_STOPPED =  2 };
      enum { OBSTACLE_STATUS_MOVING =  3 };
      enum { OBSTACLE_STATUS_ONCOMING =  4 };
      enum { OBSTACLE_STATUS_PARKED =  5 };
      enum { OBSTACLE_VALID_INVALID =  0 };
      enum { OBSTACLE_VALID_NEW =  1 };
      enum { OBSTACLE_VALID_OLDER =  2 };
      enum { OBSTACLE_LANE_NOT_ASSIGNED =  0 };
      enum { OBSTACLE_LANE_EGO_LANE =  1 };
      enum { OBSTACLE_LANE_NEXT_LANE =  2 };
      enum { OBSTACLE_LANE_INVALID =  3 };
      enum { RADAR_MATCH_CONFIDENCE_NO_MATCH =  0 };
      enum { RADAR_MATCH_CONFIDENCE_MULTI_MATCH =  1 };
      enum { RADAR_MATCH_CONFIDENCE_BOUNDED_LOW =  2 };
      enum { RADAR_MATCH_CONFIDENCE_BOUNDED_MED =  3 };
      enum { RADAR_MATCH_CONFIDENCE_BOUNDED_HIGH =  4 };
      enum { RADAR_MATCH_CONFIDENCE_HIGH =  5 };

    ObstacleData():
      header(),
      obstacle_id(0),
      obstacle_pos_x(0),
      obstacle_pos_y(0),
      blinker_info(0),
      cut_in_and_out(0),
      obstacle_rel_vel_x(0),
      obstacle_type(0),
      obstacle_status(0),
      obstacle_brake_lights(0),
      obstacle_valid(0),
      obstacle_length(0),
      obstacle_width(0),
      obstacle_age(0),
      obstacle_lane(0),
      cipv_flag(0),
      radar_pos_x(0),
      radar_vel_x(0),
      radar_match_confidence(0),
      matched_radar_id(0),
      obstacle_angle_rate(0),
      obstacle_scale_change(0),
      object_accel_x(0),
      obstacle_replaced(0),
      obstacle_angle(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->obstacle_id >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->obstacle_id >> (8 * 1)) & 0xFF;
      offset += sizeof(this->obstacle_id);
      offset += serializeAvrFloat64(outbuffer + offset, this->obstacle_pos_x);
      offset += serializeAvrFloat64(outbuffer + offset, this->obstacle_pos_y);
      *(outbuffer + offset + 0) = (this->blinker_info >> (8 * 0)) & 0xFF;
      offset += sizeof(this->blinker_info);
      *(outbuffer + offset + 0) = (this->cut_in_and_out >> (8 * 0)) & 0xFF;
      offset += sizeof(this->cut_in_and_out);
      offset += serializeAvrFloat64(outbuffer + offset, this->obstacle_rel_vel_x);
      *(outbuffer + offset + 0) = (this->obstacle_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_type);
      *(outbuffer + offset + 0) = (this->obstacle_status >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_status);
      union {
        bool real;
        uint8_t base;
      } u_obstacle_brake_lights;
      u_obstacle_brake_lights.real = this->obstacle_brake_lights;
      *(outbuffer + offset + 0) = (u_obstacle_brake_lights.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_brake_lights);
      *(outbuffer + offset + 0) = (this->obstacle_valid >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_valid);
      union {
        float real;
        uint32_t base;
      } u_obstacle_length;
      u_obstacle_length.real = this->obstacle_length;
      *(outbuffer + offset + 0) = (u_obstacle_length.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_obstacle_length.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_obstacle_length.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_obstacle_length.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->obstacle_length);
      union {
        float real;
        uint32_t base;
      } u_obstacle_width;
      u_obstacle_width.real = this->obstacle_width;
      *(outbuffer + offset + 0) = (u_obstacle_width.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_obstacle_width.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_obstacle_width.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_obstacle_width.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->obstacle_width);
      *(outbuffer + offset + 0) = (this->obstacle_age >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->obstacle_age >> (8 * 1)) & 0xFF;
      offset += sizeof(this->obstacle_age);
      *(outbuffer + offset + 0) = (this->obstacle_lane >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_lane);
      union {
        bool real;
        uint8_t base;
      } u_cipv_flag;
      u_cipv_flag.real = this->cipv_flag;
      *(outbuffer + offset + 0) = (u_cipv_flag.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->cipv_flag);
      union {
        float real;
        uint32_t base;
      } u_radar_pos_x;
      u_radar_pos_x.real = this->radar_pos_x;
      *(outbuffer + offset + 0) = (u_radar_pos_x.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_radar_pos_x.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_radar_pos_x.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_radar_pos_x.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->radar_pos_x);
      union {
        float real;
        uint32_t base;
      } u_radar_vel_x;
      u_radar_vel_x.real = this->radar_vel_x;
      *(outbuffer + offset + 0) = (u_radar_vel_x.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_radar_vel_x.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_radar_vel_x.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_radar_vel_x.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->radar_vel_x);
      *(outbuffer + offset + 0) = (this->radar_match_confidence >> (8 * 0)) & 0xFF;
      offset += sizeof(this->radar_match_confidence);
      *(outbuffer + offset + 0) = (this->matched_radar_id >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->matched_radar_id >> (8 * 1)) & 0xFF;
      offset += sizeof(this->matched_radar_id);
      union {
        float real;
        uint32_t base;
      } u_obstacle_angle_rate;
      u_obstacle_angle_rate.real = this->obstacle_angle_rate;
      *(outbuffer + offset + 0) = (u_obstacle_angle_rate.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_obstacle_angle_rate.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_obstacle_angle_rate.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_obstacle_angle_rate.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->obstacle_angle_rate);
      offset += serializeAvrFloat64(outbuffer + offset, this->obstacle_scale_change);
      union {
        float real;
        uint32_t base;
      } u_object_accel_x;
      u_object_accel_x.real = this->object_accel_x;
      *(outbuffer + offset + 0) = (u_object_accel_x.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_object_accel_x.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_object_accel_x.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_object_accel_x.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->object_accel_x);
      union {
        bool real;
        uint8_t base;
      } u_obstacle_replaced;
      u_obstacle_replaced.real = this->obstacle_replaced;
      *(outbuffer + offset + 0) = (u_obstacle_replaced.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->obstacle_replaced);
      union {
        float real;
        uint32_t base;
      } u_obstacle_angle;
      u_obstacle_angle.real = this->obstacle_angle;
      *(outbuffer + offset + 0) = (u_obstacle_angle.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_obstacle_angle.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_obstacle_angle.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_obstacle_angle.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->obstacle_angle);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->obstacle_id =  ((uint16_t) (*(inbuffer + offset)));
      this->obstacle_id |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->obstacle_id);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->obstacle_pos_x));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->obstacle_pos_y));
      this->blinker_info =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->blinker_info);
      this->cut_in_and_out =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->cut_in_and_out);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->obstacle_rel_vel_x));
      this->obstacle_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->obstacle_type);
      this->obstacle_status =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->obstacle_status);
      union {
        bool real;
        uint8_t base;
      } u_obstacle_brake_lights;
      u_obstacle_brake_lights.base = 0;
      u_obstacle_brake_lights.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->obstacle_brake_lights = u_obstacle_brake_lights.real;
      offset += sizeof(this->obstacle_brake_lights);
      this->obstacle_valid =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->obstacle_valid);
      union {
        float real;
        uint32_t base;
      } u_obstacle_length;
      u_obstacle_length.base = 0;
      u_obstacle_length.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_obstacle_length.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_obstacle_length.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_obstacle_length.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->obstacle_length = u_obstacle_length.real;
      offset += sizeof(this->obstacle_length);
      union {
        float real;
        uint32_t base;
      } u_obstacle_width;
      u_obstacle_width.base = 0;
      u_obstacle_width.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_obstacle_width.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_obstacle_width.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_obstacle_width.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->obstacle_width = u_obstacle_width.real;
      offset += sizeof(this->obstacle_width);
      this->obstacle_age =  ((uint16_t) (*(inbuffer + offset)));
      this->obstacle_age |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->obstacle_age);
      this->obstacle_lane =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->obstacle_lane);
      union {
        bool real;
        uint8_t base;
      } u_cipv_flag;
      u_cipv_flag.base = 0;
      u_cipv_flag.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->cipv_flag = u_cipv_flag.real;
      offset += sizeof(this->cipv_flag);
      union {
        float real;
        uint32_t base;
      } u_radar_pos_x;
      u_radar_pos_x.base = 0;
      u_radar_pos_x.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_radar_pos_x.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_radar_pos_x.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_radar_pos_x.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->radar_pos_x = u_radar_pos_x.real;
      offset += sizeof(this->radar_pos_x);
      union {
        float real;
        uint32_t base;
      } u_radar_vel_x;
      u_radar_vel_x.base = 0;
      u_radar_vel_x.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_radar_vel_x.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_radar_vel_x.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_radar_vel_x.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->radar_vel_x = u_radar_vel_x.real;
      offset += sizeof(this->radar_vel_x);
      this->radar_match_confidence =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->radar_match_confidence);
      this->matched_radar_id =  ((uint16_t) (*(inbuffer + offset)));
      this->matched_radar_id |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->matched_radar_id);
      union {
        float real;
        uint32_t base;
      } u_obstacle_angle_rate;
      u_obstacle_angle_rate.base = 0;
      u_obstacle_angle_rate.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_obstacle_angle_rate.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_obstacle_angle_rate.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_obstacle_angle_rate.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->obstacle_angle_rate = u_obstacle_angle_rate.real;
      offset += sizeof(this->obstacle_angle_rate);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->obstacle_scale_change));
      union {
        float real;
        uint32_t base;
      } u_object_accel_x;
      u_object_accel_x.base = 0;
      u_object_accel_x.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_object_accel_x.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_object_accel_x.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_object_accel_x.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->object_accel_x = u_object_accel_x.real;
      offset += sizeof(this->object_accel_x);
      union {
        bool real;
        uint8_t base;
      } u_obstacle_replaced;
      u_obstacle_replaced.base = 0;
      u_obstacle_replaced.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->obstacle_replaced = u_obstacle_replaced.real;
      offset += sizeof(this->obstacle_replaced);
      union {
        float real;
        uint32_t base;
      } u_obstacle_angle;
      u_obstacle_angle.base = 0;
      u_obstacle_angle.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_obstacle_angle.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_obstacle_angle.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_obstacle_angle.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->obstacle_angle = u_obstacle_angle.real;
      offset += sizeof(this->obstacle_angle);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/ObstacleData"; };
    const char * getMD5(){ return "ff75c75f79e1f472d5b0086caa5c286f"; };

  };

}
#endif
