#ifndef _RADIANT_AUDIO_AUDIO_MANAGER_H
#define _RADIANT_AUDIO_AUDIO_MANAGER_H

#include "audio.h"
#include "channel.h"
#include "libjson.h"
#include "core/singleton.h"
#include <unordered_map>
#include <vector>

namespace sf { class SoundBuffer; class Sound; class Music;}
namespace claw { namespace tween {class single_tweener;} }

BEGIN_RADIANT_AUDIO_NAMESPACE

class AudioManager : public core::Singleton<AudioManager>
{
public:
   AudioManager();
   ~AudioManager();

   void PlaySound(const std::string& uri);
   std::shared_ptr<sf::Sound> CreateSound(const std::string& uri);

   //These set the vars from whcih the next call to PlayMusic will draw its parameters
   void SetNextMusicVolume(int volume, const std::string& channel);
   void SetNextMusicFade(int fade, const std::string& channel);
   void SetNextMusicLoop(bool loop, const std::string& channel);
   void SetNextMusicCrossfade(bool crossfade, const std::string& channel);

   void PlayMusic(const std::string& track, const std::string& channel);
   void UpdateAudio();


private:
   std::shared_ptr<sf::Sound> CreateSoundInternal(const std::string& uri);
   void CleanupSounds();

   std::unordered_map<std::string, sf::SoundBuffer*> sound_buffers_;
   std::vector<std::shared_ptr<sf::Sound>> sounds_;
   std::shared_ptr<sf::Sound>              empty_sound_;
   int num_sounds_;
   int efx_volume_;
   float master_efx_volume_;

   //TODO: if we ever have more than 2, use a vector of channels
   Channel  bgm_channel_;
   Channel  ambient_channel_;
   
};

END_RADIANT_AUDIO_NAMESPACE

#endif //_RADIANT_AUDIO_AUDIO_MANAGER_H