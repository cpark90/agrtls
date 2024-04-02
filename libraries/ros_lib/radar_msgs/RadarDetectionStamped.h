#ifndef _ROS_radar_msgs_RadarDetectionStamped_h
#define _ROS_radar_msgs_RadarDetectionStamped_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "radar_msgs/RadarDetection.h"

namespace radar_msgs
{

  class RadarDetectionStamped : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef radar_msgs::RadarDetection _detection_type;
      _detection_type detection;

    RadarDetectionStamped():
      header(),
      detection()
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += this->detection.serialize(outbuffer + offset);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += this->detection.deserialize(inbuffer + offset);
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarDetectionStamped"; };
    const char * getMD5(){ return "9475ae8df95353e8dc069034d39b71ba"; };

  };

}
#endif
