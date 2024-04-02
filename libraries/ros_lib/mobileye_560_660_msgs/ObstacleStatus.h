#ifndef _ROS_mobileye_560_660_msgs_ObstacleStatus_h
#define _ROS_mobileye_560_660_msgs_ObstacleStatus_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class ObstacleStatus : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint16_t _num_obstacles_type;
      _num_obstacles_type num_obstacles;
      typedef uint16_t _timestamp_type;
      _timestamp_type timestamp;
      typedef uint16_t _application_version_type;
      _application_version_type application_version;
      typedef uint16_t _active_version_number_section_type;
      _active_version_number_section_type active_version_number_section;
      typedef bool _left_close_range_cut_in_type;
      _left_close_range_cut_in_type left_close_range_cut_in;
      typedef bool _right_close_range_cut_in_type;
      _right_close_range_cut_in_type right_close_range_cut_in;
      typedef uint8_t _stop_go_type;
      _stop_go_type stop_go;
      typedef uint8_t _protocol_version_type;
      _protocol_version_type protocol_version;
      typedef bool _close_car_type;
      _close_car_type close_car;
      typedef uint8_t _failsafe_type;
      _failsafe_type failsafe;
      enum { STOP_GO_STOP =  0 };
      enum { STOP_GO_GO =  1 };
      enum { STOP_GO_UNDECIDED =  2 };
      enum { STOP_GO_DRIVER_DECISION_REQUIRED =  3 };
      enum { STOP_GO_NOT_CALCULATED =  15 };
      enum { FAILSAFE_NONE =  0 };
      enum { FAILSAFE_LOW_SUN =  1 };
      enum { FAILSAFE_BLUR_IMAGE =  2 };

    ObstacleStatus():
      header(),
      num_obstacles(0),
      timestamp(0),
      application_version(0),
      active_version_number_section(0),
      left_close_range_cut_in(0),
      right_close_range_cut_in(0),
      stop_go(0),
      protocol_version(0),
      close_car(0),
      failsafe(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->num_obstacles >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->num_obstacles >> (8 * 1)) & 0xFF;
      offset += sizeof(this->num_obstacles);
      *(outbuffer + offset + 0) = (this->timestamp >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->timestamp >> (8 * 1)) & 0xFF;
      offset += sizeof(this->timestamp);
      *(outbuffer + offset + 0) = (this->application_version >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->application_version >> (8 * 1)) & 0xFF;
      offset += sizeof(this->application_version);
      *(outbuffer + offset + 0) = (this->active_version_number_section >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->active_version_number_section >> (8 * 1)) & 0xFF;
      offset += sizeof(this->active_version_number_section);
      union {
        bool real;
        uint8_t base;
      } u_left_close_range_cut_in;
      u_left_close_range_cut_in.real = this->left_close_range_cut_in;
      *(outbuffer + offset + 0) = (u_left_close_range_cut_in.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->left_close_range_cut_in);
      union {
        bool real;
        uint8_t base;
      } u_right_close_range_cut_in;
      u_right_close_range_cut_in.real = this->right_close_range_cut_in;
      *(outbuffer + offset + 0) = (u_right_close_range_cut_in.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->right_close_range_cut_in);
      *(outbuffer + offset + 0) = (this->stop_go >> (8 * 0)) & 0xFF;
      offset += sizeof(this->stop_go);
      *(outbuffer + offset + 0) = (this->protocol_version >> (8 * 0)) & 0xFF;
      offset += sizeof(this->protocol_version);
      union {
        bool real;
        uint8_t base;
      } u_close_car;
      u_close_car.real = this->close_car;
      *(outbuffer + offset + 0) = (u_close_car.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->close_car);
      *(outbuffer + offset + 0) = (this->failsafe >> (8 * 0)) & 0xFF;
      offset += sizeof(this->failsafe);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->num_obstacles =  ((uint16_t) (*(inbuffer + offset)));
      this->num_obstacles |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->num_obstacles);
      this->timestamp =  ((uint16_t) (*(inbuffer + offset)));
      this->timestamp |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->timestamp);
      this->application_version =  ((uint16_t) (*(inbuffer + offset)));
      this->application_version |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->application_version);
      this->active_version_number_section =  ((uint16_t) (*(inbuffer + offset)));
      this->active_version_number_section |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->active_version_number_section);
      union {
        bool real;
        uint8_t base;
      } u_left_close_range_cut_in;
      u_left_close_range_cut_in.base = 0;
      u_left_close_range_cut_in.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->left_close_range_cut_in = u_left_close_range_cut_in.real;
      offset += sizeof(this->left_close_range_cut_in);
      union {
        bool real;
        uint8_t base;
      } u_right_close_range_cut_in;
      u_right_close_range_cut_in.base = 0;
      u_right_close_range_cut_in.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->right_close_range_cut_in = u_right_close_range_cut_in.real;
      offset += sizeof(this->right_close_range_cut_in);
      this->stop_go =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->stop_go);
      this->protocol_version =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->protocol_version);
      union {
        bool real;
        uint8_t base;
      } u_close_car;
      u_close_car.base = 0;
      u_close_car.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->close_car = u_close_car.real;
      offset += sizeof(this->close_car);
      this->failsafe =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->failsafe);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/ObstacleStatus"; };
    const char * getMD5(){ return "b963ecf49d557c90935e49005018b9ff"; };

  };

}
#endif
