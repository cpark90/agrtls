#ifndef _ROS_radar_msgs_RadarTrack_h
#define _ROS_radar_msgs_RadarTrack_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "geometry_msgs/Polygon.h"
#include "geometry_msgs/Vector3.h"

namespace radar_msgs
{

  class RadarTrack : public ros::Msg
  {
    public:
      typedef uint16_t _track_id_type;
      _track_id_type track_id;
      typedef geometry_msgs::Polygon _track_shape_type;
      _track_shape_type track_shape;
      typedef geometry_msgs::Vector3 _linear_velocity_type;
      _linear_velocity_type linear_velocity;
      typedef geometry_msgs::Vector3 _linear_acceleration_type;
      _linear_acceleration_type linear_acceleration;

    RadarTrack():
      track_id(0),
      track_shape(),
      linear_velocity(),
      linear_acceleration()
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      *(outbuffer + offset + 0) = (this->track_id >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->track_id >> (8 * 1)) & 0xFF;
      offset += sizeof(this->track_id);
      offset += this->track_shape.serialize(outbuffer + offset);
      offset += this->linear_velocity.serialize(outbuffer + offset);
      offset += this->linear_acceleration.serialize(outbuffer + offset);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      this->track_id =  ((uint16_t) (*(inbuffer + offset)));
      this->track_id |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->track_id);
      offset += this->track_shape.deserialize(inbuffer + offset);
      offset += this->linear_velocity.deserialize(inbuffer + offset);
      offset += this->linear_acceleration.deserialize(inbuffer + offset);
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarTrack"; };
    const char * getMD5(){ return "371627dd284ddde5489009ee4498d083"; };

  };

}
#endif
