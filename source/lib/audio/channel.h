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
   void SetNextMusicCrossfade(bool crossfade);

   void SetPlayerVolume(float volume);

   void PlayMusic(std::string const& track);
   void UpdateMusic(int currTime);

private:
   struct Track;

private:
   void SetAndPlayMusic(std::string const& track, double target_volume);

   std::unique_ptr<Track>     currentTrack_;
   std::unique_ptr<Track>     outgoingTrack_;

   std::string    currentTrackName_;
   bool           loop_;
   bool           crossfade_;
   int            fade_;
   double         volume_;
   double         fading_volume_;
   double         rising_volume_;
   float          player_volume_;
   std::string    nextTrack_;
   int            lastUpdated_;
   std::unique_ptr<claw::tween::single_tweener> fading_tweener_;
   std::unique_ptr<claw::tween::single_tweener> rising_tweener_;

};

END_RADIANT_AUDIO_NAMESPACE

#endif  //_RADIANT_AUDIO_CHANNEL_H