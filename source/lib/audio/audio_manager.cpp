#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "audio_manager.h"
#include <vector>
#include <unordered_map>
#include <SFML/Audio.hpp>
#include "resources/res_manager.h"



using namespace ::radiant;
using namespace ::radiant::audio;

DEFINE_SINGLETON(AudioManager);

#define MAX_SOUNDS 200
#define EFX_DEF_VOL 50

//TODO: get the default volumes not from contstants, but from user-set saved files
AudioManager::AudioManager() 
{
}

//Iterate through the map and list, cleaning memory
AudioManager::~AudioManager()
{
   //Cleanup the sound
   uint i, c = sounds_.size();
   for (i = 0; i < c; i++) {
      if (sounds_[i]->getStatus() == sf::SoundSource::Status::Stopped) {
         delete sounds_[i];
      }
   }

   //Clean up all the sound buffers
   for ( std::unordered_map<std::string, sf::SoundBuffer*>::iterator it = sound_buffers_.begin(); it != sound_buffers_.end(); it++ ) {
      delete[] it->second;
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
   if (buffer == NULL) {
      buffer = new sf::SoundBuffer();
      if (buffer->loadFromStream(audio::InputStream(uri))) {
         //We loaded the stream
         sound_buffers_[uri] = buffer;
      } else {
         //If this fails, log and return
         LOG(INFO) << "Can't find Sound Effect! " << uri;
         return;
      }
   }   

   //By now, *buffer should have the correct thing loaded inside of it
   sf::Sound *s = new sf::Sound(*buffer);
   sounds_.push_back(s);
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

//By default, play the music in the bgm channel. If something else
//is specified (ambient) play it there instead.
//TODO: use a vector and switch if there are ever more than 2 or 3 channels
void AudioManager::PlayMusic(json::Node node)
{
   //Get the channel out of the node, if specified.
   if (node.has("channel")) {
      std::string channel_name = node.get<std::string>("channel");
      if (channel_name.compare("ambient") == 0) {
         ambient_channel.PlayMusic(node);
         return;
      }
   }
   bgm_channel_.PlayMusic(node);
}

//Go through the music channels and call their update functions
void AudioManager::UpdateAudio(int currTime)
{
   ambient_channel.UpdateMusic(currTime);
   bgm_channel_.UpdateMusic(currTime);
}