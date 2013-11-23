#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "audio_manager.h"
#include <list>
#include <unordered_map>


using namespace ::radiant;
using namespace ::radiant::audio;

DEFINE_SINGLETON(AudioManager);

#define MAX_SOUNDS 200

AudioManager::AudioManager()
{
   num_sounds_ = 0;
}

//Iterate through the map and list, cleaning memory
AudioManager::~AudioManager()
{
   //Clean up all the sounds
   for ( std::list<sf::Sound*>::iterator i = ui_sounds_.begin(); i != ui_sounds_.end(); i++ ) {
      delete[] (*i);
   }

   //Clean up all the buffers
   for ( std::unordered_map<std::string, sf::SoundBuffer*>::iterator it = sound_buffers_.begin(); it != sound_buffers_.end(); it++ ) {

      delete[] it->second;
   }
}

//Use with sounds that play immediately and with consistant volume
//If there are less than 200 sounds currently playing, play this sound
//Experiment: [] automatically creates a default soundbuffer if there isn't one. 
//How can we tell it's not a valid one? Check sound duration. 
//TODO: do we need to set individual volume/loop/one-of from the UI? No for starters
void AudioManager::PlayUISound(std::string uri) 
{
   CleanupSounds();
   if (ui_sounds_.size() > MAX_SOUNDS) {
      return;
   }

   sf::SoundBuffer *buffer = sound_buffers_[uri];
   if (buffer == NULL) {
      //InputStream stream = InputStream(uri);
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
   sf::Time time = buffer->getDuration();
   float sec = buffer->getDuration().asSeconds();
   if (sec <= 0) {
      //There's no input stream in the buffer yet
      
   }
   
   //By now, *buffer should have the correct thing loaded inside of it
   sf::Sound *s = new sf::Sound(*buffer);
   ui_sounds_.push_back(s);
   s->play();
}

//Cleans up any finished sounds.
void AudioManager::CleanupSounds()
{
   std::list<sf::Sound*>::iterator i = ui_sounds_.begin();
   while (i != ui_sounds_.end()) {
      if ((*i)->getStatus() == sf::SoundSource::Status::Stopped) {
         //Increment the iterator and THEN erase the item previous, which is returned by i++
         delete[] (*i);
         ui_sounds_.erase(i++); 
      }
   }
}