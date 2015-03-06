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

   void PlaySound(std::string const& uri, int vol);
   std::shared_ptr<sf::Sound> CreateSound(std::string const& uri);

   //Get and set the player volumes
   void SetPlayerVolume(float bgmVolume, float efxVolume);
   float GetPlayerBgmVolume();
   float GetPlayerEfxVolume();

   void PlayMusic(std::string const& channel, TrackInfo const& trackInfo);
   void QueueMusic(std::string const& channel, TrackInfo const& trackInfo);
   void UpdateAudio();

private:
   std::shared_ptr<sf::Sound> CreateSoundInternal(std::string const& uri);
   void CleanupSounds();

   std::unordered_map<std::string, sf::SoundBuffer*> sound_buffers_;
   std::vector<std::shared_ptr<sf::Sound>> sounds_;
   std::shared_ptr<sf::Sound>              empty_sound_;
   int num_sounds_;
   int _lastUpdateTime;
   bool useOldFalloff_;

   float player_efx_volume_;
   float player_bgm_volume_;

   float master_efx_volume_;

   std::unordered_map<std::string, Channel*> _channels;
   Channel _music;
   Channel _ambient;
};

END_RADIANT_AUDIO_NAMESPACE

#endif //_RADIANT_AUDIO_AUDIO_MANAGER_H