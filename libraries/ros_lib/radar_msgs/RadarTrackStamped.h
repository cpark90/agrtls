#ifndef _ROS_radar_msgs_RadarTrackStamped_h
#define _ROS_radar_msgs_RadarTrackStamped_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "radar_msgs/RadarTrack.h"

namespace radar_msgs
{

  class RadarTrackStamped : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef radar_msgs::RadarTrack _track_type;
      _track_type track;

    RadarTrackStamped():
      header(),
      track()
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += this->track.serialize(outbuffer + offset);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += this->track.deserialize(inbuffer + offset);
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarTrackStamped"; };
    const char * getMD5(){ return "399e617447bde0b05cc7e3e4f30fb1c7"; };

  };

}
#endif
