#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "input_stream.h"
#include "track_info.h"
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

#define CHANNEL_LOG(level)    LOG(audio, level) << "[channel \"" << _name << "\"] "
#define TRACK_LOG(level)      LOG(audio, level) << "[track\"" << _info.track<< "\"] " 

#define PLAYER_DEF_VOL 1.0 //The volume % set by the user, between 0 and 1

TrackInfo::TrackInfo() :
   volume(DEF_MUSIC_VOL),
   loop(DEF_MUSIC_LOOP),
   fadeInDuration(DEF_MUSIC_FADE_IN)
{
}

class audio::Channel::Track
{
public:
   Track(TrackInfo const& info, float const& volumeScale);
   ~Track();

   void FadeOut(int duration);
   int Update(int dt);

private:
   void Play();
   void Stop();
   uint GetTimeRemaining();

private:
   TrackInfo                     _info;
   std::unique_ptr<audio::InputStream> _stream;
   sf::Music                     _music;
   float const&                  _volumeScale;
   int                           _fadeOutDuration;
   double                        _fadeOutProgress;
   claw::tween::single_tweener   _fadeOutTweener;
   claw::tween::single_tweener   _fadeInTweener;
};

static claw::tween::single_tweener nop_tweener(0.0, 1.0, 1.0, [](double){}, claw::tween::easing_linear::ease_out);

Channel::Track::Track(TrackInfo const& info, float const& volumeScale) :
   _info(info),
   _volumeScale(volumeScale),
   _fadeOutDuration(-1),
   _fadeOutProgress(1.0f),
   _fadeOutTweener(nop_tweener),
   _fadeInTweener(nop_tweener)
{
   Play();
}

Channel::Track::~Track()
{
   _music.stop();
}

void Channel::Track::FadeOut(int duration)
{
   bool isFaster = (_fadeOutDuration < 0) || (duration < (_fadeOutDuration * _fadeOutProgress));
   if (isFaster) {
      _fadeOutDuration = duration;
      _fadeOutTweener = claw::tween::single_tweener(_fadeOutProgress, 0.0, (double)duration, [this](double fadeScale) {
         TRACK_LOG(3) << " audio fade out: " << fadeScale;
         _fadeOutProgress = fadeScale;
         _music.setVolume((float)(fadeScale * _info.volume * _volumeScale));
      }, claw::tween::easing_linear::ease_out);
      _fadeOutTweener.update(0);
   }
}

void Channel::Track::Play()
{
   _music.stop();
   if (_info.track.empty()) {
      return;
   }

   _stream.reset(new audio::InputStream(_info.track));
   if (!_music.openFromStream(*_stream)) {
      TRACK_LOG(1) << "Can't find Music!";
      return;
   }
   if (_info.fadeInDuration > 0) {
      _fadeInTweener = claw::tween::single_tweener(0.0, 1.0, (double)_info.fadeInDuration, [this](double fadeScale) {
         TRACK_LOG(3) << " audio fade in: " << fadeScale;
         _music.setVolume((float)(fadeScale * _info.volume * _volumeScale));
      }, claw::tween::easing_linear::ease_in);
      _fadeInTweener.update(0);
   } else {
      _music.setVolume((float)_info.volume * _volumeScale);
   }
   _music.setLoop(_info.loop);
   _music.play();
}

uint Channel::Track::GetTimeRemaining()
{
   if (_music.getStatus() != sf::Music::Playing) {
      return 0;
   }
   if (_info.loop) {
      return (uint)-1;
   }
   return _music.getDuration().asMilliseconds() - _music.getPlayingOffset().asMilliseconds();
}

int Channel::Track::Update(int dt)
{
   // keep fading in, if necessary.
   if (_fadeOutDuration > 0) {
      _fadeOutTweener.update((float)dt);
      if (_fadeOutTweener.is_finished()) {
         _music.stop();
      }
   } else if (_info.fadeInDuration > 0 && !_fadeInTweener.is_finished()) {
      _fadeInTweener.update((float)dt);
   }

   // see how much time is left.
   if (_music.getStatus() == sf::Music::Playing) {
      if (_info.loop) {
         return INT_MAX;
      }
      return _music.getDuration().asMilliseconds() - _music.getPlayingOffset().asMilliseconds();
   }
   return 0;
}

void Channel::Track::Stop()
{
   _music.stop();
}

Channel::Channel(const char* name) :
   _name(name),
   player_volume_(PLAYER_DEF_VOL)
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

void Channel::Play(TrackInfo const& info)
{
   // Kill everything in the queue.  Fade out everything playing
   _queued.clear();
   StopPlaying(info.fadeInDuration);

   _playing.emplace_back(new Track(info, player_volume_));
}

void Channel::Queue(TrackInfo const& info)
{
   // Add an item to the queue
   _queued.emplace_back(info);
}

//Called by the game loop
void Channel::Update(int dt)
{
   uint i = 0, c = (uint)_playing.size();

   // poll all the tracks, rolling up how much time we have left
   int timeRemaining = -1;
   while (i < c) {
      Track& track = *_playing[i];
      int t = track.Update(dt);
      if (t == 0) {
         _playing[i].reset(_playing[--c].release());
      } else {
         i++;
         if (timeRemaining == -1) {
            timeRemaining = t;
         } else {
            timeRemaining = std::min(timeRemaining, t);
         }
      }
   }
   _playing.resize(c);

   // move onto the next track
   if (!_queued.empty()) {
      int crossfade = _queued.front().fadeInDuration;
      if (timeRemaining <= crossfade) {
         CHANNEL_LOG(3) << "crossfading to " << _queued.front().track << " " << timeRemaining << " <= " << crossfade;
         StopPlaying(crossfade);
         _playing.emplace_back(new Track(_queued.front(), player_volume_));
         _queued.pop_front();
      } else {
         CHANNEL_LOG(3) << "not time to crossfading to " << _queued.front().track << " yet " << timeRemaining << " > " << crossfade;
      }
   }
}

void Channel::StopPlaying(int fadeOut)
{
   for (auto &track : _playing) {
      track->FadeOut(fadeOut);
   }
}
