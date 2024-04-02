#ifndef _ROS_novatel_msgs_INSCOV_h
#define _ROS_novatel_msgs_INSCOV_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "novatel_msgs/CommonHeader.h"

namespace novatel_msgs
{

  class INSCOV : public ros::Msg
  {
    public:
      typedef novatel_msgs::CommonHeader _header_type;
      _header_type header;
      typedef int32_t _week_type;
      _week_type week;
      typedef float _gpssec_type;
      _gpssec_type gpssec;
      typedef float _pos11_type;
      _pos11_type pos11;
      typedef float _pos12_type;
      _pos12_type pos12;
      typedef float _pos13_type;
      _pos13_type pos13;
      typedef float _pos21_type;
      _pos21_type pos21;
      typedef float _pos22_type;
      _pos22_type pos22;
      typedef float _pos23_type;
      _pos23_type pos23;
      typedef float _pos31_type;
      _pos31_type pos31;
      typedef float _pos32_type;
      _pos32_type pos32;
      typedef float _pos33_type;
      _pos33_type pos33;
      typedef float _att11_type;
      _att11_type att11;
      typedef float _att12_type;
      _att12_type att12;
      typedef float _att13_type;
      _att13_type att13;
      typedef float _att21_type;
      _att21_type att21;
      typedef float _att22_type;
      _att22_type att22;
      typedef float _att23_type;
      _att23_type att23;
      typedef float _att31_type;
      _att31_type att31;
      typedef float _att32_type;
      _att32_type att32;
      typedef float _att33_type;
      _att33_type att33;
      typedef float _vel11_type;
      _vel11_type vel11;
      typedef float _vel12_type;
      _vel12_type vel12;
      typedef float _vel13_type;
      _vel13_type vel13;
      typedef float _vel21_type;
      _vel21_type vel21;
      typedef float _vel22_type;
      _vel22_type vel22;
      typedef float _vel23_type;
      _vel23_type vel23;
      typedef float _vel31_type;
      _vel31_type vel31;
      typedef float _vel32_type;
      _vel32_type vel32;
      typedef float _vel33_type;
      _vel33_type vel33;

    INSCOV():
      header(),
      week(0),
      gpssec(0),
      pos11(0),
      pos12(0),
      pos13(0),
      pos21(0),
      pos22(0),
      pos23(0),
      pos31(0),
      pos32(0),
      pos33(0),
      att11(0),
      att12(0),
      att13(0),
      att21(0),
      att22(0),
      att23(0),
      att31(0),
      att32(0),
      att33(0),
      vel11(0),
      vel12(0),
      vel13(0),
      vel21(0),
      vel22(0),
      vel23(0),
      vel31(0),
      vel32(0),
      vel33(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        int32_t real;
        uint32_t base;
      } u_week;
      u_week.real = this->week;
      *(outbuffer + offset + 0) = (u_week.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_week.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_week.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_week.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->week);
      offset += serializeAvrFloat64(outbuffer + offset, this->gpssec);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos11);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos12);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos13);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos21);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos22);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos23);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos31);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos32);
      offset += serializeAvrFloat64(outbuffer + offset, this->pos33);
      offset += serializeAvrFloat64(outbuffer + offset, this->att11);
      offset += serializeAvrFloat64(outbuffer + offset, this->att12);
      offset += serializeAvrFloat64(outbuffer + offset, this->att13);
      offset += serializeAvrFloat64(outbuffer + offset, this->att21);
      offset += serializeAvrFloat64(outbuffer + offset, this->att22);
      offset += serializeAvrFloat64(outbuffer + offset, this->att23);
      offset += serializeAvrFloat64(outbuffer + offset, this->att31);
      offset += serializeAvrFloat64(outbuffer + offset, this->att32);
      offset += serializeAvrFloat64(outbuffer + offset, this->att33);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel11);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel12);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel13);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel21);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel22);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel23);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel31);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel32);
      offset += serializeAvrFloat64(outbuffer + offset, this->vel33);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        int32_t real;
        uint32_t base;
      } u_week;
      u_week.base = 0;
      u_week.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_week.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_week.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_week.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->week = u_week.real;
      offset += sizeof(this->week);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->gpssec));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos11));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos12));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos13));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos21));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos22));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos23));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos31));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos32));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->pos33));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att11));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att12));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att13));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att21));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att22));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att23));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att31));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att32));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->att33));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel11));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel12));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel13));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel21));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel22));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel23));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel31));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel32));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->vel33));
     return offset;
    }

    const char * getType(){ return "novatel_msgs/INSCOV"; };
    const char * getMD5(){ return "75d77cf9321af3888caeeab3a756d0ac"; };

  };

}
#endif
