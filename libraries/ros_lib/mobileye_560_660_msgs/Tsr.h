#ifndef _ROS_mobileye_560_660_msgs_Tsr_h
#define _ROS_mobileye_560_660_msgs_Tsr_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class Tsr : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint8_t _vision_only_sign_type_type;
      _vision_only_sign_type_type vision_only_sign_type;
      typedef uint8_t _vision_only_supplementary_sign_type_type;
      _vision_only_supplementary_sign_type_type vision_only_supplementary_sign_type;
      typedef float _sign_position_x_type;
      _sign_position_x_type sign_position_x;
      typedef float _sign_position_y_type;
      _sign_position_y_type sign_position_y;
      typedef float _sign_position_z_type;
      _sign_position_z_type sign_position_z;
      typedef uint8_t _filter_type_type;
      _filter_type_type filter_type;
      enum { SIGN_TYPE_REGULAR_10 =  0 };
      enum { SIGN_TYPE_REGULAR_20 =  1 };
      enum { SIGN_TYPE_REGULAR_30 =  2 };
      enum { SIGN_TYPE_REGULAR_40 =  3 };
      enum { SIGN_TYPE_REGULAR_50 =  4 };
      enum { SIGN_TYPE_REGULAR_60 =  5 };
      enum { SIGN_TYPE_REGULAR_70 =  6 };
      enum { SIGN_TYPE_REGULAR_80 =  7 };
      enum { SIGN_TYPE_REGULAR_90 =  8 };
      enum { SIGN_TYPE_REGULAR_100 =  9 };
      enum { SIGN_TYPE_REGULAR_110 =  10 };
      enum { SIGN_TYPE_REGULAR_120 =  11 };
      enum { SIGN_TYPE_REGULAR_130 =  12 };
      enum { SIGN_TYPE_REGULAR_140 =  13 };
      enum { SIGN_TYPE_REGULAR_END_RESTRICTION_OF_NUMBER =  20 };
      enum { SIGN_TYPE_ELECTRONIC_10 =  28 };
      enum { SIGN_TYPE_ELECTRONIC_20 =  29 };
      enum { SIGN_TYPE_ELECTRONIC_30 =  30 };
      enum { SIGN_TYPE_ELECTRONIC_40 =  31 };
      enum { SIGN_TYPE_ELECTRONIC_50 =  32 };
      enum { SIGN_TYPE_ELECTRONIC_60 =  33 };
      enum { SIGN_TYPE_ELECTRONIC_70 =  34 };
      enum { SIGN_TYPE_ELECTRONIC_80 =  35 };
      enum { SIGN_TYPE_ELECTRONIC_90 =  36 };
      enum { SIGN_TYPE_ELECTRONIC_100 =  37 };
      enum { SIGN_TYPE_ELECTRONIC_110 =  38 };
      enum { SIGN_TYPE_ELECTRONIC_120 =  39 };
      enum { SIGN_TYPE_ELECTRONIC_130 =  40 };
      enum { SIGN_TYPE_ELECTRONIC_140 =  41 };
      enum { SIGN_TYPE_ELECTRONIC_END_RESTRICTION_OF_NUMBER =  50 };
      enum { SIGN_TYPE_REGULAR_GENERAL_END_ALL_RESTRICTION =  64 };
      enum { SIGN_TYPE_ELECTRONIC_GENERAL_END_ALL_RESTRICTION =  65 };
      enum { SIGN_TYPE_REGULAR_5 =  100 };
      enum { SIGN_TYPE_REGULAR_15 =  101 };
      enum { SIGN_TYPE_REGULAR_25 =  102 };
      enum { SIGN_TYPE_REGULAR_35 =  103 };
      enum { SIGN_TYPE_REGULAR_45 =  104 };
      enum { SIGN_TYPE_REGULAR_55 =  105 };
      enum { SIGN_TYPE_REGULAR_65 =  106 };
      enum { SIGN_TYPE_REGULAR_75 =  107 };
      enum { SIGN_TYPE_REGULAR_85 =  108 };
      enum { SIGN_TYPE_REGULAR_95 =  109 };
      enum { SIGN_TYPE_REGULAR_105 =  110 };
      enum { SIGN_TYPE_REGULAR_115 =  111 };
      enum { SIGN_TYPE_REGULAR_125 =  112 };
      enum { SIGN_TYPE_REGULAR_135 =  113 };
      enum { SIGN_TYPE_REGULAR_145 =  114 };
      enum { SIGN_TYPE_ELECTRONIC_5 =  115 };
      enum { SIGN_TYPE_ELECTRONIC_15 =  116 };
      enum { SIGN_TYPE_ELECTRONIC_25 =  117 };
      enum { SIGN_TYPE_ELECTRONIC_35 =  118 };
      enum { SIGN_TYPE_ELECTRONIC_45 =  119 };
      enum { SIGN_TYPE_ELECTRONIC_55 =  120 };
      enum { SIGN_TYPE_ELECTRONIC_65 =  121 };
      enum { SIGN_TYPE_ELECTRONIC_75 =  122 };
      enum { SIGN_TYPE_ELECTRONIC_85 =  123 };
      enum { SIGN_TYPE_ELECTRONIC_95 =  124 };
      enum { SIGN_TYPE_ELECTRONIC_105 =  125 };
      enum { SIGN_TYPE_ELECTRONIC_115 =  126 };
      enum { SIGN_TYPE_ELECTRONIC_125 =  127 };
      enum { SIGN_TYPE_ELECTRONIC_135 =  128 };
      enum { SIGN_TYPE_ELECTRONIC_145 =  129 };
      enum { SIGN_TYPE_REGULAR_MOTORWAY_BEGIN =  171 };
      enum { SIGN_TYPE_REGULAR_END_OF_MOTORWAY =  172 };
      enum { SIGN_TYPE_REGULAR_EXPRESSWAY_BEGIN =  173 };
      enum { SIGN_TYPE_REGULAR_END_OF_EXPRESSWAY =  174 };
      enum { SIGN_TYPE_REGULAR_PLAYGROUND_AREA_BEGIN =  175 };
      enum { SIGN_TYPE_REGULAR_END_OF_PLAYGROUND_AREA =  176 };
      enum { SIGN_TYPE_REGULAR_NO_PASSING_START =  200 };
      enum { SIGN_TYPE_REGULAR_END_OF_NO_PASSING =  201 };
      enum { SIGN_TYPE_ELECTRONIC_NO_PASSING_START =  220 };
      enum { SIGN_TYPE_ELECTRONIC_END_OF_NO_PASSING =  221 };
      enum { SIGN_TYPE_NONE_DETECTED =  254 };
      enum { SIGN_TYPE_INVALID =  255 };
      enum { SUPP_SIGN_TYPE_NONE =  0 };
      enum { SUPP_SIGN_TYPE_RAIN =  1 };
      enum { SUPP_SIGN_TYPE_SNOW =  2 };
      enum { SUPP_SIGN_TYPE_TRAILER =  3 };
      enum { SUPP_SIGN_TYPE_TIME =  4 };
      enum { SUPP_SIGN_TYPE_ARROW_LEFT =  5 };
      enum { SUPP_SIGN_TYPE_ARROW_RIGHT =  6 };
      enum { SUPP_SIGN_TYPE_BEND_ARROW_LEFT =  7 };
      enum { SUPP_SIGN_TYPE_BEND_ARROW_RIGHT =  8 };
      enum { SUPP_SIGN_TYPE_TRUCK =  9 };
      enum { SUPP_SIGN_TYPE_DISTANCE_ARROW =  10 };
      enum { SUPP_SIGN_TYPE_WEIGHT =  11 };
      enum { SUPP_SIGN_TYPE_DISTANCE_IN =  12 };
      enum { SUPP_SIGN_TYPE_TRACTOR =  13 };
      enum { SUPP_SIGN_TYPE_SNOW_RAIN =  14 };
      enum { SUPP_SIGN_TYPE_SCHOOL =  15 };
      enum { SUPP_SIGN_TYPE_RAIN_CLOUD =  16 };
      enum { SUPP_SIGN_TYPE_FOG =  17 };
      enum { SUPP_SIGN_TYPE_HAZARDOUS_MATERIALS =  18 };
      enum { SUPP_SIGN_TYPE_NIGHT =  19 };
      enum { SUPP_SIGN_TYPE_GENERIC =  20 };
      enum { SUPP_SIGN_TYPE_RAPPEL =  21 };
      enum { SUPP_SIGN_TYPE_ZONE =  22 };
      enum { SUPP_SIGN_TYPE_INVALID =  255 };
      enum { FILTER_TYPE_NOT_FILTERED =  0 };
      enum { FILTER_TYPE_IRRELEVANT_TO_DRIVER =  1 };
      enum { FILTER_TYPE_ON_VEHICLE =  2 };
      enum { FILTER_TYPE_EMBEDDED =  3 };

    Tsr():
      header(),
      vision_only_sign_type(0),
      vision_only_supplementary_sign_type(0),
      sign_position_x(0),
      sign_position_y(0),
      sign_position_z(0),
      filter_type(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->vision_only_sign_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_sign_type);
      *(outbuffer + offset + 0) = (this->vision_only_supplementary_sign_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_supplementary_sign_type);
      union {
        float real;
        uint32_t base;
      } u_sign_position_x;
      u_sign_position_x.real = this->sign_position_x;
      *(outbuffer + offset + 0) = (u_sign_position_x.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_sign_position_x.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_sign_position_x.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_sign_position_x.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->sign_position_x);
      union {
        float real;
        uint32_t base;
      } u_sign_position_y;
      u_sign_position_y.real = this->sign_position_y;
      *(outbuffer + offset + 0) = (u_sign_position_y.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_sign_position_y.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_sign_position_y.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_sign_position_y.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->sign_position_y);
      union {
        float real;
        uint32_t base;
      } u_sign_position_z;
      u_sign_position_z.real = this->sign_position_z;
      *(outbuffer + offset + 0) = (u_sign_position_z.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_sign_position_z.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_sign_position_z.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_sign_position_z.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->sign_position_z);
      *(outbuffer + offset + 0) = (this->filter_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->filter_type);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->vision_only_sign_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_sign_type);
      this->vision_only_supplementary_sign_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_supplementary_sign_type);
      union {
        float real;
        uint32_t base;
      } u_sign_position_x;
      u_sign_position_x.base = 0;
      u_sign_position_x.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_sign_position_x.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_sign_position_x.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_sign_position_x.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->sign_position_x = u_sign_position_x.real;
      offset += sizeof(this->sign_position_x);
      union {
        float real;
        uint32_t base;
      } u_sign_position_y;
      u_sign_position_y.base = 0;
      u_sign_position_y.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_sign_position_y.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_sign_position_y.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_sign_position_y.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->sign_position_y = u_sign_position_y.real;
      offset += sizeof(this->sign_position_y);
      union {
        float real;
        uint32_t base;
      } u_sign_position_z;
      u_sign_position_z.base = 0;
      u_sign_position_z.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_sign_position_z.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_sign_position_z.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_sign_position_z.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->sign_position_z = u_sign_position_z.real;
      offset += sizeof(this->sign_position_z);
      this->filter_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->filter_type);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/Tsr"; };
    const char * getMD5(){ return "6181cda0894c479426a7c686589123b7"; };

  };

}
#endif
