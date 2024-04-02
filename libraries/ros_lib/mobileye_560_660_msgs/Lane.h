#ifndef _ROS_mobileye_560_660_msgs_Lane_h
#define _ROS_mobileye_560_660_msgs_Lane_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class Lane : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef float _lane_curvature_type;
      _lane_curvature_type lane_curvature;
      typedef float _lane_heading_type;
      _lane_heading_type lane_heading;
      typedef bool _construction_area_type;
      _construction_area_type construction_area;
      typedef float _pitch_angle_type;
      _pitch_angle_type pitch_angle;
      typedef float _yaw_angle_type;
      _yaw_angle_type yaw_angle;
      typedef bool _right_ldw_availability_type;
      _right_ldw_availability_type right_ldw_availability;
      typedef bool _left_ldw_availability_type;
      _left_ldw_availability_type left_ldw_availability;

    Lane():
      header(),
      lane_curvature(0),
      lane_heading(0),
      construction_area(0),
      pitch_angle(0),
      yaw_angle(0),
      right_ldw_availability(0),
      left_ldw_availability(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += serializeAvrFloat64(outbuffer + offset, this->lane_curvature);
      offset += serializeAvrFloat64(outbuffer + offset, this->lane_heading);
      union {
        bool real;
        uint8_t base;
      } u_construction_area;
      u_construction_area.real = this->construction_area;
      *(outbuffer + offset + 0) = (u_construction_area.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->construction_area);
      offset += serializeAvrFloat64(outbuffer + offset, this->pitch_angle);
      offset += serializeAvrFloat64(outbuffer + offset, this->yaw_angle);
      union {
        bool real;
        uint8_t base;
      } u_right_ldw_availability;
      u_right_ldw_availability.real = this->right_ldw_availability;
      *(outbuffer + offset + 0) = (u_right_ldw_availability.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->right_ldw_availability);
      union {
        bool real;
        uint8_t base;
      } u_left_ldw_availability;
      u_left_ldw_availability.real = this->left_ldw_availability;
      *(outbuffer + offset + 0) = (u_left_ldw_availability.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->left_ldw_availability);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->lane_curvature));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->lane_heading));
      union {
        bool real;
        uint8_t base;
      } u_construction_area;
      u_construction_area.base = 0;
      u_construction_area.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->construction_area = u_construction_area.real;
      offset += sizeof(this->construction_area);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pitch_angle));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->yaw_angle));
      union {
        bool real;
        uint8_t base;
      } u_right_ldw_availability;
      u_right_ldw_availability.base = 0;
      u_right_ldw_availability.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->right_ldw_availability = u_right_ldw_availability.real;
      offset += sizeof(this->right_ldw_availability);
      union {
        bool real;
        uint8_t base;
      } u_left_ldw_availability;
      u_left_ldw_availability.base = 0;
      u_left_ldw_availability.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->left_ldw_availability = u_left_ldw_availability.real;
      offset += sizeof(this->left_ldw_availability);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/Lane"; };
    const char * getMD5(){ return "86e77b353e33571bb5143fbccbe07372"; };

  };

}
#endif
