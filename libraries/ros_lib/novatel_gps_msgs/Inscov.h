#ifndef _ROS_novatel_gps_msgs_Inscov_h
#define _ROS_novatel_gps_msgs_Inscov_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "novatel_gps_msgs/NovatelMessageHeader.h"

namespace novatel_gps_msgs
{

  class Inscov : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef novatel_gps_msgs::NovatelMessageHeader _novatel_msg_header_type;
      _novatel_msg_header_type novatel_msg_header;
      typedef uint32_t _week_type;
      _week_type week;
      typedef float _seconds_type;
      _seconds_type seconds;
      float position_covariance[9];
      float attitude_covariance[9];
      float velocity_covariance[9];

    Inscov():
      header(),
      novatel_msg_header(),
      week(0),
      seconds(0),
      position_covariance(),
      attitude_covariance(),
      velocity_covariance()
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += this->novatel_msg_header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->week >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->week >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (this->week >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (this->week >> (8 * 3)) & 0xFF;
      offset += sizeof(this->week);
      offset += serializeAvrFloat64(outbuffer + offset, this->seconds);
      for( uint32_t i = 0; i < 9; i++){
      offset += serializeAvrFloat64(outbuffer + offset, this->position_covariance[i]);
      }
      for( uint32_t i = 0; i < 9; i++){
      offset += serializeAvrFloat64(outbuffer + offset, this->attitude_covariance[i]);
      }
      for( uint32_t i = 0; i < 9; i++){
      offset += serializeAvrFloat64(outbuffer + offset, this->velocity_covariance[i]);
      }
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += this->novatel_msg_header.deserialize(inbuffer + offset);
      this->week =  ((uint32_t) (*(inbuffer + offset)));
      this->week |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      this->week |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      this->week |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      offset += sizeof(this->week);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->seconds));
      for( uint32_t i = 0; i < 9; i++){
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->position_covariance[i]));
      }
      for( uint32_t i = 0; i < 9; i++){
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->attitude_covariance[i]));
      }
      for( uint32_t i = 0; i < 9; i++){
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->velocity_covariance[i]));
      }
     return offset;
    }

    const char * getType(){ return "novatel_gps_msgs/Inscov"; };
    const char * getMD5(){ return "a4ae1586410fc24e8ab4019825bb8bdd"; };

  };

}
#endif
