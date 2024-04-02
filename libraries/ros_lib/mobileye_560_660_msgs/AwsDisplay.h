#ifndef _ROS_mobileye_560_660_msgs_AwsDisplay_h
#define _ROS_mobileye_560_660_msgs_AwsDisplay_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class AwsDisplay : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef bool _suppress_sound_type;
      _suppress_sound_type suppress_sound;
      typedef bool _night_time_type;
      _night_time_type night_time;
      typedef bool _dusk_time_type;
      _dusk_time_type dusk_time;
      typedef uint8_t _sound_type_type;
      _sound_type_type sound_type;
      typedef bool _headway_valid_type;
      _headway_valid_type headway_valid;
      typedef float _headway_measurement_type;
      _headway_measurement_type headway_measurement;
      typedef bool _lanes_on_type;
      _lanes_on_type lanes_on;
      typedef bool _left_ldw_on_type;
      _left_ldw_on_type left_ldw_on;
      typedef bool _right_ldw_on_type;
      _right_ldw_on_type right_ldw_on;
      typedef bool _fcw_on_type;
      _fcw_on_type fcw_on;
      typedef bool _left_crossing_type;
      _left_crossing_type left_crossing;
      typedef bool _right_crossing_type;
      _right_crossing_type right_crossing;
      typedef bool _maintenance_type;
      _maintenance_type maintenance;
      typedef bool _failsafe_type;
      _failsafe_type failsafe;
      typedef bool _ped_fcw_type;
      _ped_fcw_type ped_fcw;
      typedef bool _ped_in_dz_type;
      _ped_in_dz_type ped_in_dz;
      typedef uint8_t _headway_warning_level_type;
      _headway_warning_level_type headway_warning_level;
      enum { SOUND_SILENT =  0 };
      enum { SOUND_LDWL =  1 };
      enum { SOUND_LDWR =  2 };
      enum { SOUND_FAR_HW =  3 };
      enum { SOUND_NEAR_HW =  4 };
      enum { SOUND_SOFT_FCW =  5 };
      enum { SOUND_HARD_FCW =  6 };
      enum { SOUND_RESERVED =  7 };
      enum { HEADWAY_LEVEL_OFF =  0 };
      enum { HEADWAY_LEVEL_GREEN =  1 };
      enum { HEADWAY_LEVEL_ORANGE =  2 };
      enum { HEADWAY_LEVEL_RED =  3 };

    AwsDisplay():
      header(),
      suppress_sound(0),
      night_time(0),
      dusk_time(0),
      sound_type(0),
      headway_valid(0),
      headway_measurement(0),
      lanes_on(0),
      left_ldw_on(0),
      right_ldw_on(0),
      fcw_on(0),
      left_crossing(0),
      right_crossing(0),
      maintenance(0),
      failsafe(0),
      ped_fcw(0),
      ped_in_dz(0),
      headway_warning_level(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_suppress_sound;
      u_suppress_sound.real = this->suppress_sound;
      *(outbuffer + offset + 0) = (u_suppress_sound.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->suppress_sound);
      union {
        bool real;
        uint8_t base;
      } u_night_time;
      u_night_time.real = this->night_time;
      *(outbuffer + offset + 0) = (u_night_time.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->night_time);
      union {
        bool real;
        uint8_t base;
      } u_dusk_time;
      u_dusk_time.real = this->dusk_time;
      *(outbuffer + offset + 0) = (u_dusk_time.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->dusk_time);
      *(outbuffer + offset + 0) = (this->sound_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->sound_type);
      union {
        bool real;
        uint8_t base;
      } u_headway_valid;
      u_headway_valid.real = this->headway_valid;
      *(outbuffer + offset + 0) = (u_headway_valid.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->headway_valid);
      union {
        float real;
        uint32_t base;
      } u_headway_measurement;
      u_headway_measurement.real = this->headway_measurement;
      *(outbuffer + offset + 0) = (u_headway_measurement.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_headway_measurement.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_headway_measurement.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_headway_measurement.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->headway_measurement);
      union {
        bool real;
        uint8_t base;
      } u_lanes_on;
      u_lanes_on.real = this->lanes_on;
      *(outbuffer + offset + 0) = (u_lanes_on.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lanes_on);
      union {
        bool real;
        uint8_t base;
      } u_left_ldw_on;
      u_left_ldw_on.real = this->left_ldw_on;
      *(outbuffer + offset + 0) = (u_left_ldw_on.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->left_ldw_on);
      union {
        bool real;
        uint8_t base;
      } u_right_ldw_on;
      u_right_ldw_on.real = this->right_ldw_on;
      *(outbuffer + offset + 0) = (u_right_ldw_on.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->right_ldw_on);
      union {
        bool real;
        uint8_t base;
      } u_fcw_on;
      u_fcw_on.real = this->fcw_on;
      *(outbuffer + offset + 0) = (u_fcw_on.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->fcw_on);
      union {
        bool real;
        uint8_t base;
      } u_left_crossing;
      u_left_crossing.real = this->left_crossing;
      *(outbuffer + offset + 0) = (u_left_crossing.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->left_crossing);
      union {
        bool real;
        uint8_t base;
      } u_right_crossing;
      u_right_crossing.real = this->right_crossing;
      *(outbuffer + offset + 0) = (u_right_crossing.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->right_crossing);
      union {
        bool real;
        uint8_t base;
      } u_maintenance;
      u_maintenance.real = this->maintenance;
      *(outbuffer + offset + 0) = (u_maintenance.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->maintenance);
      union {
        bool real;
        uint8_t base;
      } u_failsafe;
      u_failsafe.real = this->failsafe;
      *(outbuffer + offset + 0) = (u_failsafe.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->failsafe);
      union {
        bool real;
        uint8_t base;
      } u_ped_fcw;
      u_ped_fcw.real = this->ped_fcw;
      *(outbuffer + offset + 0) = (u_ped_fcw.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ped_fcw);
      union {
        bool real;
        uint8_t base;
      } u_ped_in_dz;
      u_ped_in_dz.real = this->ped_in_dz;
      *(outbuffer + offset + 0) = (u_ped_in_dz.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ped_in_dz);
      *(outbuffer + offset + 0) = (this->headway_warning_level >> (8 * 0)) & 0xFF;
      offset += sizeof(this->headway_warning_level);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_suppress_sound;
      u_suppress_sound.base = 0;
      u_suppress_sound.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->suppress_sound = u_suppress_sound.real;
      offset += sizeof(this->suppress_sound);
      union {
        bool real;
        uint8_t base;
      } u_night_time;
      u_night_time.base = 0;
      u_night_time.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->night_time = u_night_time.real;
      offset += sizeof(this->night_time);
      union {
        bool real;
        uint8_t base;
      } u_dusk_time;
      u_dusk_time.base = 0;
      u_dusk_time.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->dusk_time = u_dusk_time.real;
      offset += sizeof(this->dusk_time);
      this->sound_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->sound_type);
      union {
        bool real;
        uint8_t base;
      } u_headway_valid;
      u_headway_valid.base = 0;
      u_headway_valid.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->headway_valid = u_headway_valid.real;
      offset += sizeof(this->headway_valid);
      union {
        float real;
        uint32_t base;
      } u_headway_measurement;
      u_headway_measurement.base = 0;
      u_headway_measurement.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_headway_measurement.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_headway_measurement.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_headway_measurement.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->headway_measurement = u_headway_measurement.real;
      offset += sizeof(this->headway_measurement);
      union {
        bool real;
        uint8_t base;
      } u_lanes_on;
      u_lanes_on.base = 0;
      u_lanes_on.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->lanes_on = u_lanes_on.real;
      offset += sizeof(this->lanes_on);
      union {
        bool real;
        uint8_t base;
      } u_left_ldw_on;
      u_left_ldw_on.base = 0;
      u_left_ldw_on.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->left_ldw_on = u_left_ldw_on.real;
      offset += sizeof(this->left_ldw_on);
      union {
        bool real;
        uint8_t base;
      } u_right_ldw_on;
      u_right_ldw_on.base = 0;
      u_right_ldw_on.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->right_ldw_on = u_right_ldw_on.real;
      offset += sizeof(this->right_ldw_on);
      union {
        bool real;
        uint8_t base;
      } u_fcw_on;
      u_fcw_on.base = 0;
      u_fcw_on.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->fcw_on = u_fcw_on.real;
      offset += sizeof(this->fcw_on);
      union {
        bool real;
        uint8_t base;
      } u_left_crossing;
      u_left_crossing.base = 0;
      u_left_crossing.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->left_crossing = u_left_crossing.real;
      offset += sizeof(this->left_crossing);
      union {
        bool real;
        uint8_t base;
      } u_right_crossing;
      u_right_crossing.base = 0;
      u_right_crossing.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->right_crossing = u_right_crossing.real;
      offset += sizeof(this->right_crossing);
      union {
        bool real;
        uint8_t base;
      } u_maintenance;
      u_maintenance.base = 0;
      u_maintenance.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->maintenance = u_maintenance.real;
      offset += sizeof(this->maintenance);
      union {
        bool real;
        uint8_t base;
      } u_failsafe;
      u_failsafe.base = 0;
      u_failsafe.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->failsafe = u_failsafe.real;
      offset += sizeof(this->failsafe);
      union {
        bool real;
        uint8_t base;
      } u_ped_fcw;
      u_ped_fcw.base = 0;
      u_ped_fcw.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ped_fcw = u_ped_fcw.real;
      offset += sizeof(this->ped_fcw);
      union {
        bool real;
        uint8_t base;
      } u_ped_in_dz;
      u_ped_in_dz.base = 0;
      u_ped_in_dz.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ped_in_dz = u_ped_in_dz.real;
      offset += sizeof(this->ped_in_dz);
      this->headway_warning_level =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->headway_warning_level);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/AwsDisplay"; };
    const char * getMD5(){ return "7aa82a0063aa4c0e719bef3d14c24bf7"; };

  };

}
#endif
