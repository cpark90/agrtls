#ifndef _ROS_radar_msgs_RadarErrorStatus_h
#define _ROS_radar_msgs_RadarErrorStatus_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace radar_msgs
{

  class RadarErrorStatus : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef bool _comm_error_type;
      _comm_error_type comm_error;
      typedef bool _overheat_error_type;
      _overheat_error_type overheat_error;
      typedef bool _range_perf_error_type;
      _range_perf_error_type range_perf_error;
      typedef bool _internal_error_type;
      _internal_error_type internal_error;
      typedef bool _xcvr_operational_type;
      _xcvr_operational_type xcvr_operational;

    RadarErrorStatus():
      header(),
      comm_error(0),
      overheat_error(0),
      range_perf_error(0),
      internal_error(0),
      xcvr_operational(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_comm_error;
      u_comm_error.real = this->comm_error;
      *(outbuffer + offset + 0) = (u_comm_error.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->comm_error);
      union {
        bool real;
        uint8_t base;
      } u_overheat_error;
      u_overheat_error.real = this->overheat_error;
      *(outbuffer + offset + 0) = (u_overheat_error.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->overheat_error);
      union {
        bool real;
        uint8_t base;
      } u_range_perf_error;
      u_range_perf_error.real = this->range_perf_error;
      *(outbuffer + offset + 0) = (u_range_perf_error.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->range_perf_error);
      union {
        bool real;
        uint8_t base;
      } u_internal_error;
      u_internal_error.real = this->internal_error;
      *(outbuffer + offset + 0) = (u_internal_error.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->internal_error);
      union {
        bool real;
        uint8_t base;
      } u_xcvr_operational;
      u_xcvr_operational.real = this->xcvr_operational;
      *(outbuffer + offset + 0) = (u_xcvr_operational.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->xcvr_operational);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        bool real;
        uint8_t base;
      } u_comm_error;
      u_comm_error.base = 0;
      u_comm_error.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->comm_error = u_comm_error.real;
      offset += sizeof(this->comm_error);
      union {
        bool real;
        uint8_t base;
      } u_overheat_error;
      u_overheat_error.base = 0;
      u_overheat_error.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->overheat_error = u_overheat_error.real;
      offset += sizeof(this->overheat_error);
      union {
        bool real;
        uint8_t base;
      } u_range_perf_error;
      u_range_perf_error.base = 0;
      u_range_perf_error.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->range_perf_error = u_range_perf_error.real;
      offset += sizeof(this->range_perf_error);
      union {
        bool real;
        uint8_t base;
      } u_internal_error;
      u_internal_error.base = 0;
      u_internal_error.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->internal_error = u_internal_error.real;
      offset += sizeof(this->internal_error);
      union {
        bool real;
        uint8_t base;
      } u_xcvr_operational;
      u_xcvr_operational.base = 0;
      u_xcvr_operational.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->xcvr_operational = u_xcvr_operational.real;
      offset += sizeof(this->xcvr_operational);
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarErrorStatus"; };
    const char * getMD5(){ return "fe56fae2fa79a98b7f6ce349debb1508"; };

  };

}
#endif
