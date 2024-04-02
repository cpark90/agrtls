#ifndef _ROS_mobileye_560_660_msgs_LkaReferencePoints_h
#define _ROS_mobileye_560_660_msgs_LkaReferencePoints_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class LkaReferencePoints : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef float _ref_point_1_position_type;
      _ref_point_1_position_type ref_point_1_position;
      typedef float _ref_point_1_distance_type;
      _ref_point_1_distance_type ref_point_1_distance;
      typedef bool _ref_point_1_validity_type;
      _ref_point_1_validity_type ref_point_1_validity;
      typedef float _ref_point_2_position_type;
      _ref_point_2_position_type ref_point_2_position;
      typedef float _ref_point_2_distance_type;
      _ref_point_2_distance_type ref_point_2_distance;
      typedef bool _ref_point_2_validity_type;
      _ref_point_2_validity_type ref_point_2_validity;

    LkaReferencePoints():
      header(),
      ref_point_1_position(0),
      ref_point_1_distance(0),
      ref_point_1_validity(0),
      ref_point_2_position(0),
      ref_point_2_distance(0),
      ref_point_2_validity(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      offset += serializeAvrFloat64(outbuffer + offset, this->ref_point_1_position);
      offset += serializeAvrFloat64(outbuffer + offset, this->ref_point_1_distance);
      union {
        bool real;
        uint8_t base;
      } u_ref_point_1_validity;
      u_ref_point_1_validity.real = this->ref_point_1_validity;
      *(outbuffer + offset + 0) = (u_ref_point_1_validity.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ref_point_1_validity);
      offset += serializeAvrFloat64(outbuffer + offset, this->ref_point_2_position);
      offset += serializeAvrFloat64(outbuffer + offset, this->ref_point_2_distance);
      union {
        bool real;
        uint8_t base;
      } u_ref_point_2_validity;
      u_ref_point_2_validity.real = this->ref_point_2_validity;
      *(outbuffer + offset + 0) = (u_ref_point_2_validity.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ref_point_2_validity);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->ref_point_1_position));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->ref_point_1_distance));
      union {
        bool real;
        uint8_t base;
      } u_ref_point_1_validity;
      u_ref_point_1_validity.base = 0;
      u_ref_point_1_validity.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ref_point_1_validity = u_ref_point_1_validity.real;
      offset += sizeof(this->ref_point_1_validity);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->ref_point_2_position));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->ref_point_2_distance));
      union {
        bool real;
        uint8_t base;
      } u_ref_point_2_validity;
      u_ref_point_2_validity.base = 0;
      u_ref_point_2_validity.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ref_point_2_validity = u_ref_point_2_validity.real;
      offset += sizeof(this->ref_point_2_validity);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/LkaReferencePoints"; };
    const char * getMD5(){ return "0da833fa4330bb296afc10246a88cb60"; };

  };

}
#endif
