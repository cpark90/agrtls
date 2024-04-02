#ifndef _ROS_novatel_gps_msgs_Inspva_h
#define _ROS_novatel_gps_msgs_Inspva_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "novatel_gps_msgs/NovatelMessageHeader.h"

namespace novatel_gps_msgs
{

  class Inspva : public ros::Msg
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
      typedef float _latitude_type;
      _latitude_type latitude;
      typedef float _longitude_type;
      _longitude_type longitude;
      typedef float _height_type;
      _height_type height;
      typedef float _north_velocity_type;
      _north_velocity_type north_velocity;
      typedef float _east_velocity_type;
      _east_velocity_type east_velocity;
      typedef float _up_velocity_type;
      _up_velocity_type up_velocity;
      typedef float _roll_type;
      _roll_type roll;
      typedef float _pitch_type;
      _pitch_type pitch;
      typedef float _azimuth_type;
      _azimuth_type azimuth;
      typedef const char* _status_type;
      _status_type status;

    Inspva():
      header(),
      novatel_msg_header(),
      week(0),
      seconds(0),
      latitude(0),
      longitude(0),
      height(0),
      north_velocity(0),
      east_velocity(0),
      up_velocity(0),
      roll(0),
      pitch(0),
      azimuth(0),
      status("")
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
      offset += serializeAvrFloat64(outbuffer + offset, this->latitude);
      offset += serializeAvrFloat64(outbuffer + offset, this->longitude);
      offset += serializeAvrFloat64(outbuffer + offset, this->height);
      offset += serializeAvrFloat64(outbuffer + offset, this->north_velocity);
      offset += serializeAvrFloat64(outbuffer + offset, this->east_velocity);
      offset += serializeAvrFloat64(outbuffer + offset, this->up_velocity);
      offset += serializeAvrFloat64(outbuffer + offset, this->roll);
      offset += serializeAvrFloat64(outbuffer + offset, this->pitch);
      offset += serializeAvrFloat64(outbuffer + offset, this->azimuth);
      uint32_t length_status = strlen(this->status);
      varToArr(outbuffer + offset, length_status);
      offset += 4;
      memcpy(outbuffer + offset, this->status, length_status);
      offset += length_status;
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
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->latitude));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->longitude));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->height));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->north_velocity));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->east_velocity));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->up_velocity));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->roll));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pitch));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->azimuth));
      uint32_t length_status;
      arrToVar(length_status, (inbuffer + offset));
      offset += 4;
      for(unsigned int k= offset; k< offset+length_status; ++k){
          inbuffer[k-1]=inbuffer[k];
      }
      inbuffer[offset+length_status-1]=0;
      this->status = (char *)(inbuffer + offset-1);
      offset += length_status;
     return offset;
    }

    const char * getType(){ return "novatel_gps_msgs/Inspva"; };
    const char * getMD5(){ return "f6fbcfee08847158b28edeb7f76b942f"; };

  };

}
#endif
