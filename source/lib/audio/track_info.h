#ifndef _RADIANT_AUDIO_TRACK_INFO_H
#define _RADIANT_AUDIO_TRACK_INFO_H

#include "audio.h"

BEGIN_RADIANT_AUDIO_NAMESPACE

class TrackInfo
{
public:
   TrackInfo();

   int volume;
   bool loop;
   int fadeInDuration;
   std::string track;
};

END_RADIANT_AUDIO_NAMESPACE

#endif //_RADIANT_AUDIO_TRACK_INFO_H
