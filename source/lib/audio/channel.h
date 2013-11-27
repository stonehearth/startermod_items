//A channel can play 1 piece of music at a time, fading as necessary
//Use for background and ambient music
#ifndef _RADIANT_AUDIO_CHANNEL_H
#define _RADIANT_AUDIO_CHANNEL_H

#include "audio.h"
#include <unordered_map>
#include <vector>

namespace sf { class SoundBuffer; class Sound; class Music;}
namespace claw { namespace tween {class single_tweener;} }

BEGIN_RADIANT_AUDIO_NAMESPACE

class Channel
{
public: 
   Channel();
   ~Channel();

   //Music (BGM, Ambient)
   void SetNextMusicVolume(int volume);
   void SetNextMusicFade(int fade);
   void SetNextMusicLoop(bool loop);

   void PlayMusic(std::string track);
   void UpdateMusic(int currTime);

private:
   void SetAndPlayMusic(std::string track);

   std::unordered_map<std::string, std::string*>      music_buffers_;

   sf::Music*     music_;
   std::string    music_name_;
   bool           loop_;
   int            fade_;
   double         volume_;
   double         fading_volume_;
   float          master_volume_;
   std::string    nextTrack_;
   int            lastUpdated_;
   std::unique_ptr<claw::tween::single_tweener> tweener_;
};

END_RADIANT_AUDIO_NAMESPACE

#endif  //_RADIANT_AUDIO_CHANNEL_H