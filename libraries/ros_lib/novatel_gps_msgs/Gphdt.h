#ifndef _ROS_novatel_gps_msgs_Gphdt_h
#define _ROS_novatel_gps_msgs_Gphdt_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace novatel_gps_msgs
{

  class Gphdt : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef const char* _message_id_type;
      _message_id_type message_id;
      typedef float _heading_type;
      _heading_type heading;
      typedef const char* _t_type;
      _t_type t;

    Gphdt():
      header(),
      message_id(""),
      heading(0),
      t("")
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      uint32_t length_message_id = strlen(this->message_id);
      varToArr(outbuffer + offset, length_message_id);
      offset += 4;
      memcpy(outbuffer + offset, this->message_id, length_message_id);
      offset += length_message_id;
      offset += serializeAvrFloat64(outbuffer + offset, this->heading);
      uint32_t length_t = strlen(this->t);
      varToArr(outbuffer + offset, length_t);
      offset += 4;
      memcpy(outbuffer + offset, this->t, length_t);
      offset += length_t;
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      uint32_t length_message_id;
      arrToVar(length_message_id, (inbuffer + offset));
      offset += 4;
      for(unsigned int k= offset; k< offset+length_message_id; ++k){
          inbuffer[k-1]=inbuffer[k];
      }
      inbuffer[offset+length_message_id-1]=0;
      this->message_id = (char *)(inbuffer + offset-1);
      offset += length_message_id;
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->heading));
      uint32_t length_t;
      arrToVar(length_t, (inbuffer + offset));
      offset += 4;
      for(unsigned int k= offset; k< offset+length_t; ++k){
          inbuffer[k-1]=inbuffer[k];
      }
      inbuffer[offset+length_t-1]=0;
      this->t = (char *)(inbuffer + offset-1);
      offset += length_t;
     return offset;
    }

    const char * getType(){ return "novatel_gps_msgs/Gphdt"; };
    const char * getMD5(){ return "ddf912a83c312999924f468d3d13a183"; };

  };

}
#endif
