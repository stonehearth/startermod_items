//A channel can play 1 piece of music at a time, fading as necessary
//Use for background and ambient music
#ifndef _RADIANT_AUDIO_CHANNEL_H
#define _RADIANT_AUDIO_CHANNEL_H

#include "audio.h"
#include "track_info.h"
#include <unordered_map>
#include <deque>
#include <vector>

namespace sf { class SoundBuffer; class Sound; class Music;}
namespace claw { namespace tween {class single_tweener;} }

BEGIN_RADIANT_AUDIO_NAMESPACE

class Channel
{
public: 
   Channel(const char* name);
   ~Channel();
   
   void SetPlayerVolume(float volume);

   void Play(TrackInfo const& info);
   void Queue(TrackInfo const& info);

   void Update(int dt);

private:
   class Track;

   void StopPlaying(int fadeOut);

private:
   const char* _name;
   std::vector<std::unique_ptr<Track>> _playing;
   std::deque<TrackInfo>               _queued;

   float          player_volume_;
};

END_RADIANT_AUDIO_NAMESPACE

#endif  //_RADIANT_AUDIO_CHANNEL_H