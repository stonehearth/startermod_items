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

using namespace ::radiant;
using namespace ::radiant::audio;

#define A_LOG(level)    LOG(audio, level)

DEFINE_SINGLETON(AudioManager);

#define MAX_SOUNDS 200
#define EFX_DEF_VOL 50
#define EFX_MASTER_DEF_VOL 1.0

//TODO: get the default volumes not from contstants, but from user-set saved files
AudioManager::AudioManager() :
   efx_volume_(EFX_DEF_VOL),
   master_efx_volume_(EFX_MASTER_DEF_VOL)
{
}

//Iterate through the map and list, cleaning memory
AudioManager::~AudioManager()
{
   //Cleanup the sound
   uint i, c = sounds_.size();
   for (i = 0; i < c; i++) {
      delete sounds_[i];
   }

   //Clean up all the sound buffers
   for (auto const& entry : sound_buffers_) {
      delete entry.second; // note: using delete, not delete[]. delete[] is for things allocated with new []
   }
}

//Use with sounds that play immediately and with consistant volume
//If there are less than 200 sounds currently playing, play this sound
//First, see if the soundbuffer for the uri already exists. If so, reuse.
//If not, make a new soundbuffer for the sound. Then, load it into a new sound
//and play that sound. 
//TODO: merge with general sound-playing (ie, PlaySoundEffect from RenderEffectList.cpp)
void AudioManager::PlaySound(std::string uri) 
{
   CleanupSounds();
   if (sounds_.size() > MAX_SOUNDS) {
      return;
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
         return;
      }
   }   

   //By now, *buffer should have the correct thing loaded inside of it
   sf::Sound *s = new sf::Sound(*buffer);
   sounds_.push_back(s);
   s->setVolume(efx_volume_ * master_efx_volume_);
   s->play();
}

//Every time a sound plays, cleans up any finished sounds.
void AudioManager::CleanupSounds()
{
   uint i, c = sounds_.size();
   for (i = 0; i < c;) {
      if (sounds_[i]->getStatus() == sf::SoundSource::Status::Stopped) {
         delete sounds_[i];
         sounds_[i] = sounds_[--c];
      }  else {
         i++;
      }
   }
   sounds_.resize(c);
}

//Set some properties on the next piece of music that plays
//TODO: use a vector and switch if there are ever more than 2 or 3 channels
void AudioManager::SetNextMusicVolume(int volume, std::string channel)
{
    if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicVolume(volume);
   } else {
      bgm_channel_.SetNextMusicVolume(volume);
   }
}

void AudioManager::SetNextMusicFade(int fade, std::string channel)
{
   if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicFade(fade);
   } else {
      bgm_channel_.SetNextMusicFade(fade);
   }
}

void AudioManager::SetNextMusicLoop(bool loop, std::string channel)
{
   if (channel.compare("ambient") == 0) {
       ambient_channel_.SetNextMusicLoop(loop);
   } else {
      bgm_channel_.SetNextMusicLoop(loop);
   }
}

//By default, play the music in the bgm channel. If something else
//is specified (ambient) play it there instead.
void AudioManager::PlayMusic(std::string track, std::string channel)
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
