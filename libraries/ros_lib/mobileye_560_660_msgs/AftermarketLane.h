#ifndef _ROS_mobileye_560_660_msgs_AftermarketLane_h
#define _ROS_mobileye_560_660_msgs_AftermarketLane_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class AftermarketLane : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint8_t _lane_confidence_left_type;
      _lane_confidence_left_type lane_confidence_left;
      typedef bool _ldw_available_left_type;
      _ldw_available_left_type ldw_available_left;
      typedef uint8_t _lane_type_left_type;
      _lane_type_left_type lane_type_left;
      typedef float _distance_to_left_lane_type;
      _distance_to_left_lane_type distance_to_left_lane;
      typedef uint8_t _lane_confidence_right_type;
      _lane_confidence_right_type lane_confidence_right;
      typedef bool _ldw_available_right_type;
      _ldw_available_right_type ldw_available_right;
      typedef uint8_t _lane_type_right_type;
      _lane_type_right_type lane_type_right;
      typedef float _distance_to_right_lane_type;
      _distance_to_right_lane_type distance_to_right_lane;
      enum { LANE_CONFIDENCE_NONE =  0 };
      enum { LANE_CONFIDENCE_LOW =  1 };
      enum { LANE_CONFIDENCE_MED =  2 };
      enum { LANE_CONFIDENCE_HIGH =  3 };
      enum { LANE_TYPE_DASHED =  0 };
      enum { LANE_TYPE_SOLID =  1 };
      enum { LANE_TYPE_NONE =  2 };
      enum { LANE_TYPE_ROAD_EDGE =  3 };
      enum { LANE_TYPE_DOUBLE_LANE_MARK =  4 };
      enum { LANE_TYPE_BOTTS_DOTS =  5 };
      enum { LANE_TYPE_INVALID =  6 };

    AftermarketLane():
      header(),
      lane_confidence_left(0),
      ldw_available_left(0),
      lane_type_left(0),
      distance_to_left_lane(0),
      lane_confidence_right(0),
      ldw_available_right(0),
      lane_type_right(0),
      distance_to_right_lane(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->lane_confidence_left >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lane_confidence_left);
      union {
        bool real;
        uint8_t base;
      } u_ldw_available_left;
      u_ldw_available_left.real = this->ldw_available_left;
      *(outbuffer + offset + 0) = (u_ldw_available_left.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ldw_available_left);
      *(outbuffer + offset + 0) = (this->lane_type_left >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lane_type_left);
      union {
        float real;
        uint32_t base;
      } u_distance_to_left_lane;
      u_distance_to_left_lane.real = this->distance_to_left_lane;
      *(outbuffer + offset + 0) = (u_distance_to_left_lane.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_distance_to_left_lane.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_distance_to_left_lane.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_distance_to_left_lane.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->distance_to_left_lane);
      *(outbuffer + offset + 0) = (this->lane_confidence_right >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lane_confidence_right);
      union {
        bool real;
        uint8_t base;
      } u_ldw_available_right;
      u_ldw_available_right.real = this->ldw_available_right;
      *(outbuffer + offset + 0) = (u_ldw_available_right.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->ldw_available_right);
      *(outbuffer + offset + 0) = (this->lane_type_right >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lane_type_right);
      union {
        float real;
        uint32_t base;
      } u_distance_to_right_lane;
      u_distance_to_right_lane.real = this->distance_to_right_lane;
      *(outbuffer + offset + 0) = (u_distance_to_right_lane.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_distance_to_right_lane.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_distance_to_right_lane.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_distance_to_right_lane.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->distance_to_right_lane);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->lane_confidence_left =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->lane_confidence_left);
      union {
        bool real;
        uint8_t base;
      } u_ldw_available_left;
      u_ldw_available_left.base = 0;
      u_ldw_available_left.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ldw_available_left = u_ldw_available_left.real;
      offset += sizeof(this->ldw_available_left);
      this->lane_type_left =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->lane_type_left);
      union {
        float real;
        uint32_t base;
      } u_distance_to_left_lane;
      u_distance_to_left_lane.base = 0;
      u_distance_to_left_lane.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_distance_to_left_lane.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_distance_to_left_lane.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_distance_to_left_lane.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->distance_to_left_lane = u_distance_to_left_lane.real;
      offset += sizeof(this->distance_to_left_lane);
      this->lane_confidence_right =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->lane_confidence_right);
      union {
        bool real;
        uint8_t base;
      } u_ldw_available_right;
      u_ldw_available_right.base = 0;
      u_ldw_available_right.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->ldw_available_right = u_ldw_available_right.real;
      offset += sizeof(this->ldw_available_right);
      this->lane_type_right =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->lane_type_right);
      union {
        float real;
        uint32_t base;
      } u_distance_to_right_lane;
      u_distance_to_right_lane.base = 0;
      u_distance_to_right_lane.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_distance_to_right_lane.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_distance_to_right_lane.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_distance_to_right_lane.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->distance_to_right_lane = u_distance_to_right_lane.real;
      offset += sizeof(this->distance_to_right_lane);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/AftermarketLane"; };
    const char * getMD5(){ return "8a56b7a5f0247252831a59dfc0910af7"; };

  };

}
#endif
