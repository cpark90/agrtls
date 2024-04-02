#ifndef _ROS_mobileye_560_660_msgs_TsrVisionOnly_h
#define _ROS_mobileye_560_660_msgs_TsrVisionOnly_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class TsrVisionOnly : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint8_t _vision_only_sign_type_display1_type;
      _vision_only_sign_type_display1_type vision_only_sign_type_display1;
      typedef uint8_t _vision_only_supplementary_sign_type_display1_type;
      _vision_only_supplementary_sign_type_display1_type vision_only_supplementary_sign_type_display1;
      typedef uint8_t _vision_only_sign_type_display2_type;
      _vision_only_sign_type_display2_type vision_only_sign_type_display2;
      typedef uint8_t _vision_only_supplementary_sign_type_display2_type;
      _vision_only_supplementary_sign_type_display2_type vision_only_supplementary_sign_type_display2;
      typedef uint8_t _vision_only_sign_type_display3_type;
      _vision_only_sign_type_display3_type vision_only_sign_type_display3;
      typedef uint8_t _vision_only_supplementary_sign_type_display3_type;
      _vision_only_supplementary_sign_type_display3_type vision_only_supplementary_sign_type_display3;
      typedef uint8_t _vision_only_sign_type_display4_type;
      _vision_only_sign_type_display4_type vision_only_sign_type_display4;
      typedef uint8_t _vision_only_supplementary_sign_type_display4_type;
      _vision_only_supplementary_sign_type_display4_type vision_only_supplementary_sign_type_display4;

    TsrVisionOnly():
      header(),
      vision_only_sign_type_display1(0),
      vision_only_supplementary_sign_type_display1(0),
      vision_only_sign_type_display2(0),
      vision_only_supplementary_sign_type_display2(0),
      vision_only_sign_type_display3(0),
      vision_only_supplementary_sign_type_display3(0),
      vision_only_sign_type_display4(0),
      vision_only_supplementary_sign_type_display4(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->vision_only_sign_type_display1 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_sign_type_display1);
      *(outbuffer + offset + 0) = (this->vision_only_supplementary_sign_type_display1 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_supplementary_sign_type_display1);
      *(outbuffer + offset + 0) = (this->vision_only_sign_type_display2 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_sign_type_display2);
      *(outbuffer + offset + 0) = (this->vision_only_supplementary_sign_type_display2 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_supplementary_sign_type_display2);
      *(outbuffer + offset + 0) = (this->vision_only_sign_type_display3 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_sign_type_display3);
      *(outbuffer + offset + 0) = (this->vision_only_supplementary_sign_type_display3 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_supplementary_sign_type_display3);
      *(outbuffer + offset + 0) = (this->vision_only_sign_type_display4 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_sign_type_display4);
      *(outbuffer + offset + 0) = (this->vision_only_supplementary_sign_type_display4 >> (8 * 0)) & 0xFF;
      offset += sizeof(this->vision_only_supplementary_sign_type_display4);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->vision_only_sign_type_display1 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_sign_type_display1);
      this->vision_only_supplementary_sign_type_display1 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_supplementary_sign_type_display1);
      this->vision_only_sign_type_display2 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_sign_type_display2);
      this->vision_only_supplementary_sign_type_display2 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_supplementary_sign_type_display2);
      this->vision_only_sign_type_display3 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_sign_type_display3);
      this->vision_only_supplementary_sign_type_display3 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_supplementary_sign_type_display3);
      this->vision_only_sign_type_display4 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_sign_type_display4);
      this->vision_only_supplementary_sign_type_display4 =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->vision_only_supplementary_sign_type_display4);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/TsrVisionOnly"; };
    const char * getMD5(){ return "84f9582e1cda52683c53338cffe795f0"; };

  };

}
#endif
