#ifndef _RADIANT_AUDIO_AUDIO_MANAGER_H
#define _RADIANT_AUDIO_AUDIO_MANAGER_H

#include "audio.h"
#include "core/singleton.h"
#include <SFML/Audio.hpp>
#include <list>
#include <unordered_map>

BEGIN_RADIANT_AUDIO_NAMESPACE

class AudioManager : public core::Singleton<AudioManager>
{
public:
   AudioManager();
   ~AudioManager();

   void PlayUISound(std::string uri);
   void PlayBGM(std::string uri);

   json::Node GetUIEFXSettings();
   void SetUIEFXSettings();

   json::Node GetBGMSettings();
   void SetBGMSettings();

private:
   void CleanupSounds();

   std::unordered_map<std::string, sf::SoundBuffer*> sound_buffers_;
   std::list<sf::Sound*> ui_sounds_;

   sf::Music bgm_music_;

   float ui_efx_volume_;
   float bgm_volume_;

   int num_sounds_;
};

END_RADIANT_AUDIO_NAMESPACE

#endif //_RADIANT_AUDIO_AUDIO_MANAGER_H