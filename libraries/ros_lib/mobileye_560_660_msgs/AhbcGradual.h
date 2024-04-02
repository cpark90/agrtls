#ifndef _ROS_mobileye_560_660_msgs_AhbcGradual_h
#define _ROS_mobileye_560_660_msgs_AhbcGradual_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"

namespace mobileye_560_660_msgs
{

  class AhbcGradual : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef float _boundary_domain_bottom_non_glare_hlb_type;
      _boundary_domain_bottom_non_glare_hlb_type boundary_domain_bottom_non_glare_hlb;
      typedef float _boundary_domain_non_glare_left_hand_hlb_type;
      _boundary_domain_non_glare_left_hand_hlb_type boundary_domain_non_glare_left_hand_hlb;
      typedef float _boundary_domain_non_glare_right_hand_hlb_type;
      _boundary_domain_non_glare_right_hand_hlb_type boundary_domain_non_glare_right_hand_hlb;
      typedef uint16_t _object_distance_hlb_type;
      _object_distance_hlb_type object_distance_hlb;
      typedef uint8_t _status_boundary_domain_bottom_non_glare_hlb_type;
      _status_boundary_domain_bottom_non_glare_hlb_type status_boundary_domain_bottom_non_glare_hlb;
      typedef uint8_t _status_boundary_domain_non_glare_left_hand_hlb_type;
      _status_boundary_domain_non_glare_left_hand_hlb_type status_boundary_domain_non_glare_left_hand_hlb;
      typedef uint8_t _status_boundary_domain_non_glare_right_hand_hlb_type;
      _status_boundary_domain_non_glare_right_hand_hlb_type status_boundary_domain_non_glare_right_hand_hlb;
      typedef uint8_t _status_object_distance_hlb_type;
      _status_object_distance_hlb_type status_object_distance_hlb;
      typedef bool _left_target_change_type;
      _left_target_change_type left_target_change;
      typedef bool _right_target_change_type;
      _right_target_change_type right_target_change;
      typedef bool _too_many_cars_type;
      _too_many_cars_type too_many_cars;
      typedef bool _busy_scene_type;
      _busy_scene_type busy_scene;

    AhbcGradual():
      header(),
      boundary_domain_bottom_non_glare_hlb(0),
      boundary_domain_non_glare_left_hand_hlb(0),
      boundary_domain_non_glare_right_hand_hlb(0),
      object_distance_hlb(0),
      status_boundary_domain_bottom_non_glare_hlb(0),
      status_boundary_domain_non_glare_left_hand_hlb(0),
      status_boundary_domain_non_glare_right_hand_hlb(0),
      status_object_distance_hlb(0),
      left_target_change(0),
      right_target_change(0),
      too_many_cars(0),
      busy_scene(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_bottom_non_glare_hlb;
      u_boundary_domain_bottom_non_glare_hlb.real = this->boundary_domain_bottom_non_glare_hlb;
      *(outbuffer + offset + 0) = (u_boundary_domain_bottom_non_glare_hlb.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_boundary_domain_bottom_non_glare_hlb.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_boundary_domain_bottom_non_glare_hlb.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_boundary_domain_bottom_non_glare_hlb.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->boundary_domain_bottom_non_glare_hlb);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_non_glare_left_hand_hlb;
      u_boundary_domain_non_glare_left_hand_hlb.real = this->boundary_domain_non_glare_left_hand_hlb;
      *(outbuffer + offset + 0) = (u_boundary_domain_non_glare_left_hand_hlb.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_boundary_domain_non_glare_left_hand_hlb.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_boundary_domain_non_glare_left_hand_hlb.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_boundary_domain_non_glare_left_hand_hlb.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->boundary_domain_non_glare_left_hand_hlb);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_non_glare_right_hand_hlb;
      u_boundary_domain_non_glare_right_hand_hlb.real = this->boundary_domain_non_glare_right_hand_hlb;
      *(outbuffer + offset + 0) = (u_boundary_domain_non_glare_right_hand_hlb.base >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (u_boundary_domain_non_glare_right_hand_hlb.base >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (u_boundary_domain_non_glare_right_hand_hlb.base >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (u_boundary_domain_non_glare_right_hand_hlb.base >> (8 * 3)) & 0xFF;
      offset += sizeof(this->boundary_domain_non_glare_right_hand_hlb);
      *(outbuffer + offset + 0) = (this->object_distance_hlb >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->object_distance_hlb >> (8 * 1)) & 0xFF;
      offset += sizeof(this->object_distance_hlb);
      *(outbuffer + offset + 0) = (this->status_boundary_domain_bottom_non_glare_hlb >> (8 * 0)) & 0xFF;
      offset += sizeof(this->status_boundary_domain_bottom_non_glare_hlb);
      *(outbuffer + offset + 0) = (this->status_boundary_domain_non_glare_left_hand_hlb >> (8 * 0)) & 0xFF;
      offset += sizeof(this->status_boundary_domain_non_glare_left_hand_hlb);
      *(outbuffer + offset + 0) = (this->status_boundary_domain_non_glare_right_hand_hlb >> (8 * 0)) & 0xFF;
      offset += sizeof(this->status_boundary_domain_non_glare_right_hand_hlb);
      *(outbuffer + offset + 0) = (this->status_object_distance_hlb >> (8 * 0)) & 0xFF;
      offset += sizeof(this->status_object_distance_hlb);
      union {
        bool real;
        uint8_t base;
      } u_left_target_change;
      u_left_target_change.real = this->left_target_change;
      *(outbuffer + offset + 0) = (u_left_target_change.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->left_target_change);
      union {
        bool real;
        uint8_t base;
      } u_right_target_change;
      u_right_target_change.real = this->right_target_change;
      *(outbuffer + offset + 0) = (u_right_target_change.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->right_target_change);
      union {
        bool real;
        uint8_t base;
      } u_too_many_cars;
      u_too_many_cars.real = this->too_many_cars;
      *(outbuffer + offset + 0) = (u_too_many_cars.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->too_many_cars);
      union {
        bool real;
        uint8_t base;
      } u_busy_scene;
      u_busy_scene.real = this->busy_scene;
      *(outbuffer + offset + 0) = (u_busy_scene.base >> (8 * 0)) & 0xFF;
      offset += sizeof(this->busy_scene);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_bottom_non_glare_hlb;
      u_boundary_domain_bottom_non_glare_hlb.base = 0;
      u_boundary_domain_bottom_non_glare_hlb.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_boundary_domain_bottom_non_glare_hlb.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_boundary_domain_bottom_non_glare_hlb.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_boundary_domain_bottom_non_glare_hlb.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->boundary_domain_bottom_non_glare_hlb = u_boundary_domain_bottom_non_glare_hlb.real;
      offset += sizeof(this->boundary_domain_bottom_non_glare_hlb);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_non_glare_left_hand_hlb;
      u_boundary_domain_non_glare_left_hand_hlb.base = 0;
      u_boundary_domain_non_glare_left_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_boundary_domain_non_glare_left_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_boundary_domain_non_glare_left_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_boundary_domain_non_glare_left_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->boundary_domain_non_glare_left_hand_hlb = u_boundary_domain_non_glare_left_hand_hlb.real;
      offset += sizeof(this->boundary_domain_non_glare_left_hand_hlb);
      union {
        float real;
        uint32_t base;
      } u_boundary_domain_non_glare_right_hand_hlb;
      u_boundary_domain_non_glare_right_hand_hlb.base = 0;
      u_boundary_domain_non_glare_right_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 0))) << (8 * 0);
      u_boundary_domain_non_glare_right_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      u_boundary_domain_non_glare_right_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      u_boundary_domain_non_glare_right_hand_hlb.base |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      this->boundary_domain_non_glare_right_hand_hlb = u_boundary_domain_non_glare_right_hand_hlb.real;
      offset += sizeof(this->boundary_domain_non_glare_right_hand_hlb);
      this->object_distance_hlb =  ((uint16_t) (*(inbuffer + offset)));
      this->object_distance_hlb |= ((uint16_t) (*(inbuffer + offset + 1))) << (8 * 1);
      offset += sizeof(this->object_distance_hlb);
      this->status_boundary_domain_bottom_non_glare_hlb =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->status_boundary_domain_bottom_non_glare_hlb);
      this->status_boundary_domain_non_glare_left_hand_hlb =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->status_boundary_domain_non_glare_left_hand_hlb);
      this->status_boundary_domain_non_glare_right_hand_hlb =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->status_boundary_domain_non_glare_right_hand_hlb);
      this->status_object_distance_hlb =  ((uint8_t) (*(inbuffer + offset)));
      offset += sizeof(this->status_object_distance_hlb);
      union {
        bool real;
        uint8_t base;
      } u_left_target_change;
      u_left_target_change.base = 0;
      u_left_target_change.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->left_target_change = u_left_target_change.real;
      offset += sizeof(this->left_target_change);
      union {
        bool real;
        uint8_t base;
      } u_right_target_change;
      u_right_target_change.base = 0;
      u_right_target_change.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->right_target_change = u_right_target_change.real;
      offset += sizeof(this->right_target_change);
      union {
        bool real;
        uint8_t base;
      } u_too_many_cars;
      u_too_many_cars.base = 0;
      u_too_many_cars.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->too_many_cars = u_too_many_cars.real;
      offset += sizeof(this->too_many_cars);
      union {
        bool real;
        uint8_t base;
      } u_busy_scene;
      u_busy_scene.base = 0;
      u_busy_scene.base |= ((uint8_t) (*(inbuffer + offset + 0))) << (8 * 0);
      this->busy_scene = u_busy_scene.real;
      offset += sizeof(this->busy_scene);
     return offset;
    }

    const char * getType(){ return "mobileye_560_660_msgs/AhbcGradual"; };
    const char * getMD5(){ return "06801ea66cd7dc52de9867c12dbfa5bf"; };

  };

}
#endif
