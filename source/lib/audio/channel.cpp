#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "channel.h"
#include "resources/res_manager.h"

#include <unordered_map>
#include <SFML/Audio.hpp>
#include <claw/tween/single_tweener.hpp>
#include <claw/tween/easing/easing_back.hpp>
#include <claw/tween/easing/easing_bounce.hpp>
#include <claw/tween/easing/easing_circ.hpp>
#include <claw/tween/easing/easing_cubic.hpp>
#include <claw/tween/easing/easing_elastic.hpp>
#include <claw/tween/easing/easing_linear.hpp>
#include <claw/tween/easing/easing_none.hpp>
#include <claw/tween/easing/easing_quad.hpp>
#include <claw/tween/easing/easing_quart.hpp>
#include <claw/tween/easing/easing_quint.hpp>
#include <claw/tween/easing/easing_sine.hpp>

using namespace ::radiant;
using namespace ::radiant::audio;

#define BGM_DEF_VOL 50         //The volume for the playing music
#define BGM_MASTER_DEF_VOL 0.5 //The volume % set by the user, between 0 and 1
#define BGM_DEF_FADE 1000      //MS to have old music fade during crossfade
#define BGM_DEFAULT_LOOP true

//Delcare some tweening functions to gentle the fade in/out of sound effects
//Review Question:
//This is a copy of the fns from render_effect_list.cpp.
//Is there a better/central place to put them?
// see http://libclaw.sourceforge.net/tweeners.html
claw::tween::single_tweener::easing_function
get_audio_easing_function(std::string easing)
{
#define GET_AUDIO_EASING_FUNCTION(name) \
   if (eq == std::string(#name)) { \
      if (method == "in") { return claw::tween::easing_ ## name::ease_in; } \
      if (method == "out") { return claw::tween::easing_ ## name::ease_out; } \
      if (method == "in_out") { return claw::tween::easing_ ## name::ease_in_out; } \
   }

   int offset = easing.find('_');
   if (offset != std::string::npos) {
      std::string eq = easing.substr(0, offset);
      std::string method = easing.substr(offset + 1);

      GET_AUDIO_EASING_FUNCTION(back)
      GET_AUDIO_EASING_FUNCTION(bounce)
      GET_AUDIO_EASING_FUNCTION(circ)
      GET_AUDIO_EASING_FUNCTION(cubic)
      GET_AUDIO_EASING_FUNCTION(elastic)
      GET_AUDIO_EASING_FUNCTION(linear)
      GET_AUDIO_EASING_FUNCTION(none)
      GET_AUDIO_EASING_FUNCTION(quad)
      GET_AUDIO_EASING_FUNCTION(quart)
      GET_AUDIO_EASING_FUNCTION(quint)
      GET_AUDIO_EASING_FUNCTION(sine)
   }
   return claw::tween::easing_linear::ease_in;
#undef GET_EASING_FUNCTION
}

Channel::Channel() :
   bg_music_(NULL),
   bgm_fade_(BGM_DEF_FADE),
   bgm_volume_(BGM_DEF_VOL),
   bgm_master_volume_(BGM_MASTER_DEF_VOL),
   bgm_loop_(BGM_DEFAULT_LOOP),
   lastUpdated_(0)
{
}

Channel::~Channel()
{
   //Clean up all the music strings
   for ( std::unordered_map<std::string, std::string*>::iterator it = music_buffers_.begin(); it != music_buffers_.end(); it++ ) {
      delete[] it->second;
   }

   //Cleanup the music
   delete[] bg_music_;
}

//Recieves a JSON node full of music-related data:
//track name, and optionally whether to loop, and fade. 
void Channel::PlayMusic(json::Node node)
{
   //Get track and other optional data out of the node
   std::string uri = node.get<std::string>("track");
   if (node.has("loop")) {
      bgm_loop_ = node.get<bool>("loop");
   }
   if (node.has("fade")) {
      bgm_fade_ = node.get<int>("fade");
   }
   if (node.has("volume")) {
      bgm_volume_ = node.get<int>("volume");
   }

   //If this track is already playing, then just return
   if(uri.compare(bg_music_name_) == 0) {
      return;
   }

   //If there is already music playing, note next track for update
   if (bg_music_ != NULL && bg_music_->getStatus() == sf::Music::Playing) {
     nextTrack_ = uri;
   } else {
      //If there is no music, immediately start bg music
      if (bg_music_ != NULL) {
         delete[] bg_music_;
      }
      bg_music_ = new sf::Music();
      SetAndPlayMusic(uri);
   }
}

void Channel::SetAndPlayMusic(std::string track)
{
   static std::string* data;
   //REVIEW QUESTION:
   //Using an iterator here is terribly inefficient, but I if I try to index
   //an uninitialized std::string, I sometimes get back a piece of
   //existing string from another entry. Scary! At least there shouldn't
   //be too many music files, so it won't have to iterate long?
   auto i = music_buffers_.find(track);
   if (i == music_buffers_.end()) {
      auto stream = res::ResourceManager2::GetInstance().OpenResource(track);
      std::stringstream reader;
      reader << stream->rdbuf();
      std::string local_string = reader.str();
      data = new std::string(local_string);
      music_buffers_[track] = data;
   } else {
      data = music_buffers_[track];
   }

   if (bg_music_->openFromMemory(data->c_str(), data->size())) {
      bg_music_->setLoop(bgm_loop_);
      //Change to reflect that bgm's user-set and dynamically-set value can change
      bg_music_->setVolume((float)bgm_volume_ * bgm_master_volume_);
      bg_music_->play();
      bg_music_name_ = track;
   } else { 
      LOG(INFO) << "Can't find Music! " << track;
   }
}

//Called by the game loop
//TODO: crossfade
void Channel::UpdateMusic(int currTime)
{
   double dt = lastUpdated_ ? (currTime - lastUpdated_) : 0.0;
   lastUpdated_ = currTime;

   //If there is another track to play after this one
   if (nextTrack_.length() > 0) {
      if (!tweener_.get()) {
         //if we have another track to do and the tweener is null, then create a new tweener
         int fade = bgm_fade_;
         bgm_fading_volume_ = bgm_volume_;
         tweener_.reset(new claw::tween::single_tweener(bgm_fading_volume_, 0, fade, get_audio_easing_function("sine_out")));
      } else {
         //There is a next track and a tweener, we must be in the process of quieting the prev music
         if (tweener_->is_finished()) {
            //If the tweener is finished, play the next track
            tweener_ = NULL;
            bg_music_->stop();
            SetAndPlayMusic(nextTrack_);
            nextTrack_ = "";
         } else {
            //If the tweener is not finished, soften the volume
            tweener_->update(dt);
            bg_music_->setVolume((float)bgm_fading_volume_*bgm_master_volume_);
         }
      }
   }
}