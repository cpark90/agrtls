#ifndef _ROS_novatel_gps_msgs_NovatelVelocity_h
#define _ROS_novatel_gps_msgs_NovatelVelocity_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "novatel_gps_msgs/NovatelMessageHeader.h"

namespace novatel_gps_msgs
{

  class NovatelVelocity : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef novatel_gps_msgs::NovatelMessageHeader _novatel_msg_header_type;
      _novatel_msg_header_type novatel_msg_header;
      typedef const char* _solution_status_type;
      _solution_status_type solution_status;
      typedef const char* _velocity_type_type;
      _velocity_type_type velocity_type;
      typedef float _latency_type;
      _latency_type latency;
      typedef float _age_type;
      _age_type age;
      typedef float _horizontal_speed_type;
      _horizontal_speed_type horizontal_speed;
      typedef float _track_ground_type;
      _track_ground_type track_ground;
      typedef float _vertical_speed_type;
      _vertical_speed_type vertical_speed;

    NovatelVelocity():
      header(),
      novatel_msg_header(),
      solution_status(""),
      velocity_type(""),
      latency(0),
      age(0),
      horizontal_speed(0),
      track_ground(0),
      vertical_speed(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += this->novatel_msg_header.serialize(outbuffer + offset);
      uint32_t length_solution_status = strlen(this->solution_status);
      varToArr(outbuffer + offset, length_solution_status);
      offset += 4;
      memcpy(outbuffer + offset, this->solution_status, length_solution_status);
      offset += length_solution_status;
      uint32_t length_velocity_type = strlen(this->velocity_type);
      varToArr(outbuffer + offset, length_velocity_type);
      offset += 4;
      memcpy(outbuffer + offset, this->velocity_type, length_velocity_type);
      offset += length_velocity_type;
      union {
        float real;
        uint32_t base;
      } u_latency;
      u_latency.real = this->latency;
      *(outbuffer + offset + 0) = (u_latency.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_latency.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_latency.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_latency.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->latency);
      union {
        float real;
        uint32_t base;
      } u_age;
      u_age.real = this->age;
      *(outbuffer + offset + 0) = (u_age.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_age.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_age.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_age.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->age);
      offset += serializeAvrFloat64(outbuffer + offset, this->horizontal_speed);
      offset += serializeAvrFloat64(outbuffer + offset, this->track_ground);
      offset += serializeAvrFloat64(outbuffer + offset, this->vertical_speed);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += this->novatel_msg_header.deserialize(inbuffer + offset);
      uint32_t length_solution_status;
      arrToVar(length_solution_status, (inbuffer + offset));
      offset += 4;
      for(unsigned int k= offset; k< offset+length_solution_status; ++k){
          inbuffer[k-1]=inbuffer[k];
      }
      inbuffer[offset+length_solution_status-1]=0;
      this->solution_status = (char *)(inbuffer + offset-1);
      offset += length_solution_status;
      uint32_t length_velocity_type;
      arrToVar(length_velocity_type, (inbuffer + offset));
      offset += 4;
      for(unsigned int k= offset; k< offset+length_velocity_type; ++k){
          inbuffer[k-1]=inbuffer[k];
      }
      inbuffer[offset+length_velocity_type-1]=0;
      this->velocity_type = (char *)(inbuffer + offset-1);
      offset += length_velocity_type;
      union {
        float real;
        uint32_t base;
      } u_latency;
      u_latency.base = 0;
      u_latency.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_latency.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_latency.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_latency.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->latency = u_latency.real;
      offset += sizeof(this->latency);
      union {
        float real;
        uint32_t base;
      } u_age;
      u_age.base = 0;
      u_age.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_age.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_age.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_age.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->age = u_age.real;
      offset += sizeof(this->age);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->horizontal_speed));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->track_ground));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vertical_speed));
     return offset;
    }

    const char * getType(){ return "novatel_gps_msgs/NovatelVelocity"; };
    const char * getMD5(){ return "a8fb7d9232aaf68f98caa2b4cda2597b"; };

  };

}
#endif
