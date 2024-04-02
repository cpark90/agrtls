#ifndef _ROS_mobileye_560_660_msgs_Ahbc_h
#define _ROS_mobileye_560_660_msgs_Ahbc_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class Ahbc : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint8_t _high_low_beam_decision_type;
      _high_low_beam_decision_type high_low_beam_decision;
      typedef uint16_t _reasons_for_switch_to_low_beam_type;
      _reasons_for_switch_to_low_beam_type reasons_for_switch_to_low_beam;
      enum { HIGH_LOW_BEAM_DECISION_NO_RECOMMENDATION =  0 };
      enum { HIGH_LOW_BEAM_DECISION_RECOMMENDATION_OFF =  1 };
      enum { HIGH_LOW_BEAM_DECISION_RECOMMENDATION_ON =  2 };
      enum { HIGH_LOW_BEAM_DECISION_INVALID =  3 };

    Ahbc():
      header(),
      high_low_beam_decision(0),
      reasons_for_switch_to_low_beam(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->high_low_beam_decision >> (8 * 0)) & 0xFF;
      offset += sizeof(this->high_low_beam_decision);
      *(outbuffer + offset + 0) = (this->reasons_for_switch_to_low_beam >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->reasons_for_switch_to_low_beam >> (8 * 1)) & 0xFF;
      offset += sizeof(this->reasons_for_switch_to_low_beam);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->high_low_beam_decision =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->high_low_beam_decision);
      this->reasons_for_switch_to_low_beam =  ((uint16_t) (*(inbuffer + offset)));
      this->reasons_for_switch_to_low_beam |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->reasons_for_switch_to_low_beam);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/Ahbc"; };
    const char * getMD5(){ return "475e214fc14bee0ccbbfc2ae7aaea6ec"; };

  };

}
#endif
