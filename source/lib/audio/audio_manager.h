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

   void PlaySound(std::string uri);

   void PlayMusic(json::Node node);
   void UpdateAudio(int currTime);

   //TODO: implement getter/setters for sound/music by channel

private:
   void CleanupSounds();

   std::unordered_map<std::string, sf::SoundBuffer*> sound_buffers_;
   std::vector<sf::Sound*> sounds_;
   float ui_efx_volume_;   
   int num_sounds_;

   //TODO: if we ever have more than 2, use a vector of channels
   Channel  bgm_channel_;
   Channel  ambient_channel;
   
};

END_RADIANT_AUDIO_NAMESPACE

#endif //_RADIANT_AUDIO_AUDIO_MANAGER_H