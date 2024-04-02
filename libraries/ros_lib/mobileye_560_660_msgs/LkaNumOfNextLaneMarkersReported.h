#ifndef _ROS_mobileye_560_660_msgs_LkaNumOfNextLaneMarkersReported_h
#define _ROS_mobileye_560_660_msgs_LkaNumOfNextLaneMarkersReported_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class LkaNumOfNextLaneMarkersReported : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint16_t _num_of_next_lane_markers_reported_type;
      _num_of_next_lane_markers_reported_type num_of_next_lane_markers_reported;

    LkaNumOfNextLaneMarkersReported():
      header(),
      num_of_next_lane_markers_reported(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->num_of_next_lane_markers_reported >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->num_of_next_lane_markers_reported >> (8 * 1)) & 0xFF;
      offset += sizeof(this->num_of_next_lane_markers_reported);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->num_of_next_lane_markers_reported =  ((uint16_t) (*(inbuffer + offset)));
      this->num_of_next_lane_markers_reported |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->num_of_next_lane_markers_reported);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/LkaNumOfNextLaneMarkersReported"; };
    const char * getMD5(){ return "0313c1cecbae25d684c324d160d9925e"; };

  };

}
#endif
