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

#define PLAYER_DEF_VOL 1.0 //The volume % set by the user, between 0 and 1


struct audio::Channel::Track {
   audio::InputStream   stream;
   sf::Music            music;

   Track(std::string const& trackName);
   ~Track();
};

Channel::Track::Track(std::string const& trackName)
   : stream(trackName)
{
}

Channel::Track::~Track()
{
   music.stop();
}


Channel::Channel() :
   rising_tweener_(nullptr),
   fading_tweener_(nullptr),
   fade_(DEF_MUSIC_FADE),
   volume_(DEF_MUSIC_VOL),
   player_volume_(PLAYER_DEF_VOL),
   loop_(DEF_MUSIC_LOOP),
   crossfade_(DEF_MUSIC_CROSSFADE),
   lastUpdated_(0)
{
}

Channel::~Channel()
{
}

//This is the volume that the player sets; a number between 0.0 and 1.0
//The actual volume derived with player_volume_ * game volume, or just volume
void Channel::SetPlayerVolume(float vol)
{
   player_volume_ = vol;
}

//This is the volume the music on this track will next play at
//The actual volume derived with this * player_volume_
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


void Channel::PlayMusic(std::string const& track)
{
   // If this track is already playing, then just return
   if (track == currentTrackName_) {
      return;
   }

   if (currentTrack_ && currentTrack_->music.getStatus() == sf::Music::Playing) {
      // If there is already music playing, note next track for update
     nextTrack_ = track;
   } else {
      // If there is no music, immediately start music
      SetAndPlayMusic(track, volume_);
   }
}

//Pass in the name of the track to play and the game's desired volume for the music
//Note: the player's desired volume is added right before the music is played
void Channel::SetAndPlayMusic(std::string const& trackName, double target_volume)
{
   currentTrack_.reset(new Track(trackName));

   sf::Music* music = &currentTrack_->music;
   if (music->openFromStream(currentTrack_->stream)) {
      music->setLoop(loop_);
      //Change to reflect that music's user-set and dynamically-set value can change
      music->setVolume((float)target_volume * player_volume_);
      music->play();
      currentTrackName_ = trackName;
   } else { 
      A_LOG(1) << "Can't find Music! " << trackName;
   }
}

//Called by the game loop
void Channel::UpdateMusic(int currTime)
{
   double dt = lastUpdated_ ? (currTime - lastUpdated_) : 0.0;
   lastUpdated_ = currTime;

   // If there is another track to play after this one
   if (!nextTrack_.empty() && !fading_tweener_.get()) {
      int fade = fade_;
      fading_volume_ = volume_;
      outgoingTrack_.reset(currentTrack_.release());
      fading_tweener_.reset(new claw::tween::single_tweener(fading_volume_, 0, fade, claw::tween::easing_linear::ease_out));

      // If crossfade option is true, then also create a rising tweener and set the new music to playing
      if (crossfade_ && !rising_tweener_.get()) {
         rising_volume_ = 0;
         rising_tweener_.reset(new claw::tween::single_tweener(rising_volume_, volume_, fade, claw::tween::easing_linear::ease_in));

         SetAndPlayMusic(nextTrack_, rising_volume_);
         nextTrack_ = "";
      }
   } 

   // If we are in the middle of fading out (and potentially fading in)
   if (outgoingTrack_ != nullptr) {
      if (fading_tweener_->is_finished()) {
         // and if the fade tweener is finished, stop the outgoing music and reset the tweener. 
         fading_tweener_.reset(nullptr);
         outgoingTrack_.reset(nullptr);
            
         // If we are not crossfading, then start the next track. 
         if (!crossfade_) {
            SetAndPlayMusic(nextTrack_, volume_);
            nextTrack_ = "";
         }
      } else {
         // If the tweener is not finished, soften the volume
         fading_tweener_->update(dt);
         outgoingTrack_->music.setVolume((float)fading_volume_ * player_volume_);

         // If crossfade is on, raise that volume
         if (crossfade_) {
            rising_tweener_->update(dt);
            currentTrack_->music.setVolume((float)rising_volume_ * player_volume_);
         }
      }
   } else {
      // If nobody is fading out (or fading in) then just one piece of music is playing
      // adjust that music by player volume
      // TODO: this might cause audio weirdness if the player is adjusting the volume during a fade
      if (currentTrack_ && currentTrack_->music.getVolume() != volume_ * player_volume_) {
         currentTrack_->music.setVolume(static_cast<float>(volume_ * player_volume_));
      }
   }
   
   // If we have a rising tweener that is finished, then set it to null.
   if (rising_tweener_.get() && rising_tweener_->is_finished()) {
      rising_tweener_.reset(nullptr);
   }
}
