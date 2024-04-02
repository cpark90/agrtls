#ifndef _ROS_radar_msgs_RadarTrackArray_h
#define _ROS_radar_msgs_RadarTrackArray_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "radar_msgs/RadarTrack.h"

namespace radar_msgs
{

  class RadarTrackArray : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      uint32_t tracks_length;
      typedef radar_msgs::RadarTrack _tracks_type;
      _tracks_type st_tracks;
      _tracks_type * tracks;

    RadarTrackArray():
      header(),
      tracks_length(0), tracks(NULL)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->tracks_length >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->tracks_length >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (this->tracks_length >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (this->tracks_length >> (8 * 3)) & 0xFF;
      offset += sizeof(this->tracks_length);
      for( uint32_t i = 0; i < tracks_length; i++){
      offset += this->tracks[i].serialize(outbuffer + offset);
      }
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      uint32_t tracks_lengthT = ((uint32_t) (*(inbuffer + offset))); 
      tracks_lengthT |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1); 
      tracks_lengthT |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2); 
      tracks_lengthT |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3); 
      offset += sizeof(this->tracks_length);
      if(tracks_lengthT > tracks_length)
        this->tracks = (radar_msgs::RadarTrack*)realloc(this->tracks, tracks_lengthT * sizeof(radar_msgs::RadarTrack));
      tracks_length = tracks_lengthT;
      for( uint32_t i = 0; i < tracks_length; i++){
      offset += this->st_tracks.deserialize(inbuffer + offset);
        memcpy( &(this->tracks[i]), &(this->st_tracks), sizeof(radar_msgs::RadarTrack));
      }
     return offset;
    }

    const char * getType(){ return "radar_msgs/RadarTrackArray"; };
    const char * getMD5(){ return "49d8549010f0f446ea46ca97a1099cba"; };

  };

}
#endif
