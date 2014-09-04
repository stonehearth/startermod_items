#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "audio_manager.h"
#include <vector>
#include <unordered_map>
#include <SFML/Audio.hpp>
#include "resources/res_manager.h"
#include "platform/utils.h"
#include "core/config.h"


using namespace ::radiant;
using namespace ::radiant::audio;

#define A_LOG(level)    LOG(audio, level)

DEFINE_SINGLETON(AudioManager);

#define MAX_SOUNDS 200

const float DEFAULT_PLAYER_BGM_VOL = 1.0f;
const float DEFAULT_PLAYER_EFX_VOL = 1.0f;

//#pragma optimize("", off)

//TODO: get the default volumes not from contstants, but from user-set saved files
AudioManager::AudioManager()
{
   empty_sound_ = std::make_shared<sf::Sound>(sf::SoundBuffer());

   const core::Config& config = core::Config::GetInstance();
   player_bgm_volume_ = config.Get("audio.bgm_volume", DEFAULT_PLAYER_BGM_VOL);
   player_efx_volume_ = config.Get("audio.efx_volume", DEFAULT_PLAYER_EFX_VOL);

   bgm_channel_.SetPlayerVolume(player_bgm_volume_);
   ambient_channel_.SetPlayerVolume(player_efx_volume_);
}
//#pragma optimize("", on)

//Iterate through the map and list, cleaning memory
AudioManager::~AudioManager()
{
   //Clean up all the sound buffers
   for (auto const& entry : sound_buffers_) {
      delete entry.second; // note: using delete, not delete[]. delete[] is for things allocated with new []
   }
}

void AudioManager::SetPlayerVolume(float bgmVolume, float efxVolume)
{
   player_bgm_volume_ = bgmVolume;
   bgm_channel_.SetPlayerVolume(bgmVolume);

   player_efx_volume_ = efxVolume;
   ambient_channel_.SetPlayerVolume(efxVolume);

   //TODO: the effects that play in the game have to get the volume from this class.

   //Save to config now? Save later/less often?
   core::Config& config = core::Config::GetInstance();
   config.Set("audio.bgm_volume", player_bgm_volume_);
   config.Set("audio.efx_volume", player_efx_volume_);
}

float AudioManager::GetPlayerBgmVolume() 
{
   return player_bgm_volume_;
}

float AudioManager::GetPlayerEfxVolume()
{
   return player_efx_volume_;
}



//Use with sounds that play immediately and with consistant volume
//If there are less than 200 sounds currently playing, play this sound
//First, see if the soundbuffer for the uri already exists. If so, reuse.
//If not, make a new soundbuffer for the sound. Then, load it into a new sound
//and play that sound. 
void AudioManager::PlaySound(std::string const& uri, int vol) 
{
   std::shared_ptr<sf::Sound> s = CreateSoundInternal(uri);
   s->setVolume((float)vol * player_efx_volume_);
   s->play();
}

std::shared_ptr<sf::Sound> AudioManager::CreateSound(std::string const& uri)
{
   return CreateSoundInternal(uri);
}

std::shared_ptr<sf::Sound> AudioManager::CreateSoundInternal(std::string const& uri) 
{
   CleanupSounds();
   if (sounds_.size() >= MAX_SOUNDS) {
      return empty_sound_;
   }

   sf::SoundBuffer *buffer = sound_buffers_[uri];
   if (buffer == nullptr) {
      buffer = new sf::SoundBuffer();
      if (buffer->loadFromStream(audio::InputStream(uri))) {
         //We loaded the stream
         sound_buffers_[uri] = buffer;
      } else {
         //If this fails, log and return
         A_LOG(1) << "Can't find Sound Effect! " << uri;
         delete buffer;
         return empty_sound_;
      }
   }   

   //By now, *buffer should have the correct thing loaded inside of it
   sounds_.push_back(std::make_shared<sf::Sound>(*buffer));
   return sounds_.back();
}

void AudioManager::CleanupSounds()
{
   // No need to do any work until we're at MAX_SOUNDS.
   if (sounds_.size() < MAX_SOUNDS) {
      return;
   }
   uint i, c = sounds_.size();
   for (i = 0; i < c;) {
      if (sounds_[i]->getStatus() == sf::SoundSource::Status::Stopped) {
         sounds_.erase(sounds_.begin() + i);
         --c;
      }  else {
         i++;
      }
   }
   sounds_.resize(c);
}

//Set some properties on the next piece of music that plays
//TODO: use a vector and switch if there are ever more than 2 or 3 channels
void AudioManager::SetNextMusicVolume(int volume, std::string const& channel)
{
    if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicVolume(volume);
   } else {
      bgm_channel_.SetNextMusicVolume(volume);
   }
}

void AudioManager::SetNextMusicFade(int fade, std::string const& channel)
{
   if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicFade(fade);
   } else {
      bgm_channel_.SetNextMusicFade(fade);
   }
}

void AudioManager::SetNextMusicLoop(bool loop, std::string const& channel)
{
   if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicLoop(loop);
   } else {
      bgm_channel_.SetNextMusicLoop(loop);
   }
}

void AudioManager::SetNextMusicCrossfade(bool crossfade, std::string const& channel)
{
   if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicCrossfade(crossfade);
   } else {
      bgm_channel_.SetNextMusicCrossfade(crossfade);
   }
}

//By default, play the music in the bgm channel. If something else
//is specified (ambient) play it there instead.
void AudioManager::PlayMusic(std::string const& track, std::string const& channel)
{
   if (channel.compare("ambient") == 0) {
      ambient_channel_.PlayMusic(track);
   } else {
      bgm_channel_.PlayMusic(track);
   }
}

//Go through the music channels and call their update functions
void AudioManager::UpdateAudio()
{
   int current_time = platform::get_current_time_in_ms();
   ambient_channel_.UpdateMusic(current_time);
   bgm_channel_.UpdateMusic(current_time);
}
