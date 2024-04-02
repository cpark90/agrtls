#ifndef _ROS_radar_msgs_RadarDetection_h
#define _ROS_radar_msgs_RadarDetection_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Vector3.h"

namespace radar_msgs
{

  class RadarDetection : public ros::Msg
  {
    public:
      typedef uint16_t _detection_id_type;
      _detection_id_type detection_id;
      typedef geometry_msgs::Point _position_type;
      _position_type position;
      typedef geometry_msgs::Vector3 _velocity_type;
      _velocity_type velocity;
      typedef float _amplitude_type;
      _amplitude_type amplitude;

    RadarDetection():
      detection_id(0),
      position(),
      velocity(),
      amplitude(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      *(outbuffer + offset + 0) = (this->detection_id >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->detection_id >> (8 * 1)) & 0xFF;
      offset += sizeof(this->detection_id);
      offset += this->position.serialize(outbuffer + offset);
      offset += this->velocity.serialize(outbuffer + offset);
      offset += serializeAvrFloat64(outbuffer + offset, this->amplitude);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      this->detection_id =  ((uint16_t) (*(inbuffer + offset)));
      this->detection_id |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->detection_id);
      offset += this->position.deserialize(inbuffer + offset);
      offset += this->velocity.deserialize(inbuffer + offset);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->amplitude));
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarDetection"; };
    const char * getMD5(){ return "5eb81a43632fa3ab61f658171a9b6c5b"; };

  };

}
#endif
