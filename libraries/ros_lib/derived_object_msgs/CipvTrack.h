#ifndef _ROS_derived_object_msgs_CipvTrack_h
#define _ROS_derived_object_msgs_CipvTrack_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "radar_msgs/RadarTrack.h"

namespace derived_object_msgs
{

  class CipvTrack : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef bool _is_valid_type;
      _is_valid_type is_valid;
      typedef radar_msgs::RadarTrack _track_type;
      _track_type track;

    CipvTrack():
      header(),
      is_valid(0),
      track()
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_is_valid;
      u_is_valid.real = this->is_valid;
      *(outbuffer + offset + 0) = (u_is_valid.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->is_valid);
      offset += this->track.serialize(outbuffer + offset);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_is_valid;
      u_is_valid.base = 0;
      u_is_valid.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->is_valid = u_is_valid.real;
      offset += sizeof(this->is_valid);
      offset += this->track.deserialize(inbuffer + offset);
     return offset;
    }

    const char * getType(){ return "derived_object_msgs/CipvTrack"; };
    const char * getMD5(){ return "db9a76b43a89517bb97bfea8c670557e"; };

  };

}
#endif
