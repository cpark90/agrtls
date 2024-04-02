#ifndef _ROS_mobileye_560_660_msgs_FixedFoe_h
#define _ROS_mobileye_560_660_msgs_FixedFoe_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class FixedFoe : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef float _fixed_yaw_type;
      _fixed_yaw_type fixed_yaw;
      typedef float _fixed_horizon_type;
      _fixed_horizon_type fixed_horizon;

    FixedFoe():
      header(),
      fixed_yaw(0),
      fixed_horizon(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += serializeAvrFloat64(outbuffer + offset, this->fixed_yaw);
      offset += serializeAvrFloat64(outbuffer + offset, this->fixed_horizon);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->fixed_yaw));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->fixed_horizon));
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/FixedFoe"; };
    const char * getMD5(){ return "b4f93d021949d9d8c671473a5bedf703"; };

  };

}
#endif
