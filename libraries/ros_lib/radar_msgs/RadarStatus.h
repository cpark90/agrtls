#ifndef _ROS_radar_msgs_RadarStatus_h
#define _ROS_radar_msgs_RadarStatus_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace radar_msgs
{

  class RadarStatus : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef int16_t _curvature_type;
      _curvature_type curvature;
      typedef float _yaw_rate_type;
      _yaw_rate_type yaw_rate;
      typedef float _vehicle_speed_type;
      _vehicle_speed_type vehicle_speed;
      typedef uint8_t _max_track_targets_type;
      _max_track_targets_type max_track_targets;
      typedef bool _raw_data_mode_type;
      _raw_data_mode_type raw_data_mode;
      typedef int8_t _temperature_type;
      _temperature_type temperature;
      typedef bool _patial_blockage_type;
      _patial_blockage_type patial_blockage;
      typedef bool _side_lobe_blockage_type;
      _side_lobe_blockage_type side_lobe_blockage;

    RadarStatus():
      header(),
      curvature(0),
      yaw_rate(0),
      vehicle_speed(0),
      max_track_targets(0),
      raw_data_mode(0),
      temperature(0),
      patial_blockage(0),
      side_lobe_blockage(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        int16_t real;
        uint16_t base;
      } u_curvature;
      u_curvature.real = this->curvature;
      *(outbuffer + offset + 0) = (u_curvature.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_curvature.base >> (8 * 1)) & 0xFF;
      offset += sizeof(this->curvature);
      union {
        float real;
        uint32_t base;
      } u_yaw_rate;
      u_yaw_rate.real = this->yaw_rate;
      *(outbuffer + offset + 0) = (u_yaw_rate.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_yaw_rate.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_yaw_rate.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_yaw_rate.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->yaw_rate);
      union {
        float real;
        uint32_t base;
      } u_vehicle_speed;
      u_vehicle_speed.real = this->vehicle_speed;
      *(outbuffer + offset + 0) = (u_vehicle_speed.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_vehicle_speed.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_vehicle_speed.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_vehicle_speed.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->vehicle_speed);
      *(outbuffer + offset + 0) = (this->max_track_targets >> (8 * 0)) & 0xFF;
      offset += sizeof(this->max_track_targets);
      union {
        bool real;
        uint8_t base;
      } u_raw_data_mode;
      u_raw_data_mode.real = this->raw_data_mode;
      *(outbuffer + offset + 0) = (u_raw_data_mode.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->raw_data_mode);
      union {
        int8_t real;
        uint8_t base;
      } u_temperature;
      u_temperature.real = this->temperature;
      *(outbuffer + offset + 0) = (u_temperature.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->temperature);
      union {
        bool real;
        uint8_t base;
      } u_patial_blockage;
      u_patial_blockage.real = this->patial_blockage;
      *(outbuffer + offset + 0) = (u_patial_blockage.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->patial_blockage);
      union {
        bool real;
        uint8_t base;
      } u_side_lobe_blockage;
      u_side_lobe_blockage.real = this->side_lobe_blockage;
      *(outbuffer + offset + 0) = (u_side_lobe_blockage.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->side_lobe_blockage);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        int16_t real;
        uint16_t base;
      } u_curvature;
      u_curvature.base = 0;
      u_curvature.base |= ((uint16_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_curvature.base |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      this->curvature = u_curvature.real;
      offset += sizeof(this->curvature);
      union {
        float real;
        uint32_t base;
      } u_yaw_rate;
      u_yaw_rate.base = 0;
      u_yaw_rate.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_yaw_rate.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_yaw_rate.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_yaw_rate.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->yaw_rate = u_yaw_rate.real;
      offset += sizeof(this->yaw_rate);
      union {
        float real;
        uint32_t base;
      } u_vehicle_speed;
      u_vehicle_speed.base = 0;
      u_vehicle_speed.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_vehicle_speed.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_vehicle_speed.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_vehicle_speed.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->vehicle_speed = u_vehicle_speed.real;
      offset += sizeof(this->vehicle_speed);
      this->max_track_targets =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->max_track_targets);
      union {
        bool real;
        uint8_t base;
      } u_raw_data_mode;
      u_raw_data_mode.base = 0;
      u_raw_data_mode.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->raw_data_mode = u_raw_data_mode.real;
      offset += sizeof(this->raw_data_mode);
      union {
        int8_t real;
        uint8_t base;
      } u_temperature;
      u_temperature.base = 0;
      u_temperature.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->temperature = u_temperature.real;
      offset += sizeof(this->temperature);
      union {
        bool real;
        uint8_t base;
      } u_patial_blockage;
      u_patial_blockage.base = 0;
      u_patial_blockage.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->patial_blockage = u_patial_blockage.real;
      offset += sizeof(this->patial_blockage);
      union {
        bool real;
        uint8_t base;
      } u_side_lobe_blockage;
      u_side_lobe_blockage.base = 0;
      u_side_lobe_blockage.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->side_lobe_blockage = u_side_lobe_blockage.real;
      offset += sizeof(this->side_lobe_blockage);
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarStatus"; };
    const char * getMD5(){ return "5b982da6e1f20a73f7c8a15070ee5a36"; };

  };

}
#endif
