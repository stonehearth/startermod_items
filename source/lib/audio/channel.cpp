#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "channel.h"
#include "audio.h"
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

#define A_LOG(level)    LOG(audio, level)

//#define DEF_VOL 50         //The volume for the playing music
#define MASTER_DEF_VOL 1.0 //The volume % set by the user, between 0 and 1
//#define DEF_FADE 1000      //MS to have old music fade during crossfade
//#define DEFAULT_LOOP true

Channel::Channel() :
   music_(nullptr),
   outgoing_music_(nullptr),
   rising_tweener_(nullptr),
   fading_tweener_(nullptr),
   fade_(DEF_MUSIC_FADE),
   volume_(DEF_MUSIC_VOL),
   master_volume_(MASTER_DEF_VOL),
   loop_(DEF_MUSIC_LOOP),
   crossfade_(DEF_MUSIC_CROSSFADE),
   lastUpdated_(0)
{
}

Channel::~Channel()
{
}

void Channel::SetNextMusicVolume(int volume)
{
   volume_ = volume;
}

void Channel::SetNextMusicFade(int fade)
{
   fade_ = fade;
}

void Channel::SetNextMusicLoop(bool loop)
{
   loop_ = loop;
}

void Channel::SetNextMusicCrossfade(bool crossfade)
{
   crossfade_ = crossfade;
}


void Channel::PlayMusic(const std::string& track)
{
   //If this track is already playing, then just return
   if(track.compare(music_name_) == 0) {
      return;
   }

   //If there is already music playing, note next track for update
   if (music_ != nullptr && music_->getStatus() == sf::Music::Playing) {
     nextTrack_ = track;
   } else {
      //If there is no music, immediately start music
      if (music_ == nullptr) {
         music_.reset(new sf::Music());
      }
      music_->stop();
      SetAndPlayMusic(track, volume_);
   }
}

void Channel::SetAndPlayMusic(const std::string& track, double target_volume)
{
   auto i = music_buffers_.find(track);
   if (i == music_buffers_.end()) {
      auto stream = res::ResourceManager2::GetInstance().OpenResource(track);
      std::stringstream reader;
      reader << stream->rdbuf();
      music_buffers_[track] = reader.str();
   } 

   if (music_->openFromMemory(music_buffers_[track].c_str(), music_buffers_[track].size())) {
      music_->setLoop(loop_);
      //Change to reflect that music's user-set and dynamically-set value can change
      music_->setVolume((float)target_volume * master_volume_);
      music_->play();
      music_name_ = track;
   } else { 
      A_LOG(1) << "Can't find Music! " << track;
   }
}

//Called by the game loop
void Channel::UpdateMusic(int currTime)
{
   double dt = lastUpdated_ ? (currTime - lastUpdated_) : 0.0;
   lastUpdated_ = currTime;

   //If there is another track to play after this one
   if (nextTrack_ != "" && !fading_tweener_.get()) {
      int fade = fade_;
      fading_volume_ = volume_;
      outgoing_music_.reset(music_.release());
      fading_tweener_.reset(new claw::tween::single_tweener(fading_volume_, 0, fade, claw::tween::easing_linear::ease_out));

      //If crossfade option is true, then also create a rising tweener and set the new music to playing
      if (crossfade_ && !rising_tweener_.get()) {
         rising_volume_ = 0;
         rising_tweener_.reset(new claw::tween::single_tweener(rising_volume_, volume_, fade, claw::tween::easing_linear::ease_in));

         music_.reset(new sf::Music());
         SetAndPlayMusic(nextTrack_, rising_volume_);
         nextTrack_ = "";
      }
   } 

   //If we are in the middle of fading out (and potentially fading in)
   if (outgoing_music_ != nullptr) {
      if (fading_tweener_->is_finished()) {
         //and if the fade tweener is finished, stop the outgoing music and reset the tweener. 
         fading_tweener_.reset(nullptr);
         outgoing_music_->stop();
         outgoing_music_.reset(nullptr);
            
         //If we are not crossfading, then start the next track. 
         if (!crossfade_) {
            SetAndPlayMusic(nextTrack_, volume_);
            nextTrack_ = "";
         }
      } else {
         //If the tweener is not finished, soften the volume
         fading_tweener_->update(dt);
         outgoing_music_->setVolume((float)fading_volume_ * master_volume_);

         //If crossfade is on, raise that volume
         if (crossfade_) {
            rising_tweener_->update(dt);
            music_->setVolume((float)rising_volume_ * master_volume_);
         }
      }
   }
   
   //If we have a rising tweener that is finsihed, then set it to null.
   if (rising_tweener_.get() && rising_tweener_->is_finished()) {
      rising_tweener_.reset(nullptr);
   }
}