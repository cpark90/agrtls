#ifndef _ROS_novatel_gps_msgs_NovatelCorrectedImuData_h
#define _ROS_novatel_gps_msgs_NovatelCorrectedImuData_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "novatel_gps_msgs/NovatelMessageHeader.h"

namespace novatel_gps_msgs
{

  class NovatelCorrectedImuData : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef novatel_gps_msgs::NovatelMessageHeader _novatel_msg_header_type;
      _novatel_msg_header_type novatel_msg_header;
      typedef uint32_t _gps_week_num_type;
      _gps_week_num_type gps_week_num;
      typedef float _gps_seconds_type;
      _gps_seconds_type gps_seconds;
      typedef float _pitch_rate_type;
      _pitch_rate_type pitch_rate;
      typedef float _roll_rate_type;
      _roll_rate_type roll_rate;
      typedef float _yaw_rate_type;
      _yaw_rate_type yaw_rate;
      typedef float _lateral_acceleration_type;
      _lateral_acceleration_type lateral_acceleration;
      typedef float _longitudinal_acceleration_type;
      _longitudinal_acceleration_type longitudinal_acceleration;
      typedef float _vertical_acceleration_type;
      _vertical_acceleration_type vertical_acceleration;

    NovatelCorrectedImuData():
      header(),
      novatel_msg_header(),
      gps_week_num(0),
      gps_seconds(0),
      pitch_rate(0),
      roll_rate(0),
      yaw_rate(0),
      lateral_acceleration(0),
      longitudinal_acceleration(0),
      vertical_acceleration(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += this->novatel_msg_header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->gps_week_num >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->gps_week_num >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (this->gps_week_num >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (this->gps_week_num >> (8 * 3)) & 0xFF;
      offset += sizeof(this->gps_week_num);
      offset += serializeAvrFloat64(outbuffer + offset, this->gps_seconds);
      offset += serializeAvrFloat64(outbuffer + offset, this->pitch_rate);
      offset += serializeAvrFloat64(outbuffer + offset, this->roll_rate);
      offset += serializeAvrFloat64(outbuffer + offset, this->yaw_rate);
      offset += serializeAvrFloat64(outbuffer + offset, this->lateral_acceleration);
      offset += serializeAvrFloat64(outbuffer + offset, this->longitudinal_acceleration);
      offset += serializeAvrFloat64(outbuffer + offset, this->vertical_acceleration);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += this->novatel_msg_header.deserialize(inbuffer + offset);
      this->gps_week_num =  ((uint32_t) (*(inbuffer + offset)));
      this->gps_week_num |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      this->gps_week_num |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      this->gps_week_num |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      offset += sizeof(this->gps_week_num);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->gps_seconds));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pitch_rate));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->roll_rate));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->yaw_rate));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->lateral_acceleration));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->longitudinal_acceleration));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vertical_acceleration));
     return offset;
    }

    const char * getType(){ return "novatel_gps_msgs/NovatelCorrectedImuData"; };
    const char * getMD5(){ return "81ec3aad90f65315c03ad2199cdd99cf"; };

  };

}
#endif
