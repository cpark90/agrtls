#ifndef _ROS_mobileye_560_660_msgs_LkaLane_h
#define _ROS_mobileye_560_660_msgs_LkaLane_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class LkaLane : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint8_t _lane_type_type;
      _lane_type_type lane_type;
      typedef uint8_t _quality_type;
      _quality_type quality;
      typedef uint8_t _model_degree_type;
      _model_degree_type model_degree;
      typedef float _position_parameter_c0_type;
      _position_parameter_c0_type position_parameter_c0;
      typedef float _curvature_parameter_c2_type;
      _curvature_parameter_c2_type curvature_parameter_c2;
      typedef float _curvature_derivative_parameter_c3_type;
      _curvature_derivative_parameter_c3_type curvature_derivative_parameter_c3;
      typedef float _marking_width_type;
      _marking_width_type marking_width;
      typedef float _heading_angle_parameter_c1_type;
      _heading_angle_parameter_c1_type heading_angle_parameter_c1;
      typedef float _view_range_type;
      _view_range_type view_range;
      typedef bool _view_range_availability_type;
      _view_range_availability_type view_range_availability;
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

    LkaLane():
      header(),
      lane_type(0),
      quality(0),
      model_degree(0),
      position_parameter_c0(0),
      curvature_parameter_c2(0),
      curvature_derivative_parameter_c3(0),
      marking_width(0),
      heading_angle_parameter_c1(0),
      view_range(0),
      view_range_availability(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->lane_type >> (8 * 0)) & 0xFF;
      offset += sizeof(this->lane_type);
      *(outbuffer + offset + 0) = (this->quality >> (8 * 0)) & 0xFF;
      offset += sizeof(this->quality);
      *(outbuffer + offset + 0) = (this->model_degree >> (8 * 0)) & 0xFF;
      offset += sizeof(this->model_degree);
      offset += serializeAvrFloat64(outbuffer + offset, this->position_parameter_c0);
      offset += serializeAvrFloat64(outbuffer + offset, this->curvature_parameter_c2);
      offset += serializeAvrFloat64(outbuffer + offset, this->curvature_derivative_parameter_c3);
      union {
        float real;
        uint32_t base;
      } u_marking_width;
      u_marking_width.real = this->marking_width;
      *(outbuffer + offset + 0) = (u_marking_width.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_marking_width.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_marking_width.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_marking_width.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->marking_width);
      offset += serializeAvrFloat64(outbuffer + offset, this->heading_angle_parameter_c1);
      union {
        float real;
        uint32_t base;
      } u_view_range;
      u_view_range.real = this->view_range;
      *(outbuffer + offset + 0) = (u_view_range.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_view_range.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_view_range.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_view_range.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->view_range);
      union {
        bool real;
        uint8_t base;
      } u_view_range_availability;
      u_view_range_availability.real = this->view_range_availability;
      *(outbuffer + offset + 0) = (u_view_range_availability.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->view_range_availability);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->lane_type =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->lane_type);
      this->quality =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->quality);
      this->model_degree =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->model_degree);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->position_parameter_c0));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->curvature_parameter_c2));
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->curvature_derivative_parameter_c3));
      union {
        float real;
        uint32_t base;
      } u_marking_width;
      u_marking_width.base = 0;
      u_marking_width.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_marking_width.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_marking_width.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_marking_width.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->marking_width = u_marking_width.real;
      offset += sizeof(this->marking_width);
      offset += deserializeAvrFloat64(inbuffer + offset, &(this->heading_angle_parameter_c1));
      union {
        float real;
        uint32_t base;
      } u_view_range;
      u_view_range.base = 0;
      u_view_range.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_view_range.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_view_range.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_view_range.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->view_range = u_view_range.real;
      offset += sizeof(this->view_range);
      union {
        bool real;
        uint8_t base;
      } u_view_range_availability;
      u_view_range_availability.base = 0;
      u_view_range_availability.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->view_range_availability = u_view_range_availability.real;
      offset += sizeof(this->view_range_availability);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/LkaLane"; };
    const char * getMD5(){ return "13c7b357c14488be92473cab7e5461ca"; };

  };

}
#endif
