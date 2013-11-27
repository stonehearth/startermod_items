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
   void PlayMusic(json::Node node);

   //Implement when we have a UI
   json::Node GetMusicSettings();
   void SetMusicSettings();

   void UpdateMusic(int currTime);

private:
   void SetAndPlayMusic(std::string track);

   std::unordered_map<std::string, std::string*>      music_buffers_;

   sf::Music*     bg_music_;
   std::string    bg_music_name_;
   bool           bgm_loop_;
   int            bgm_fade_;
   double         bgm_volume_;
   double         bgm_fading_volume_;
   float          bgm_master_volume_;
   std::string    nextTrack_;
   int            startTime_;
   int            lastUpdated_;
   std::unique_ptr<claw::tween::single_tweener> tweener_;
};

END_RADIANT_AUDIO_NAMESPACE

#endif  //_RADIANT_AUDIO_CHANNEL_H