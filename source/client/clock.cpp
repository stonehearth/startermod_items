#include "pch.h"
#include "platform/utils.h"
#include "clock.h"

using namespace radiant;
using namespace radiant::client;

Clock::Clock() :
   realtime_start_(0),
   game_time_start_(0),
   game_time_interval_(0),
   game_time_next_delivery_(0),
   last_reported_time_(0)
{
}

void Clock::Update(int game_time, int game_time_interval, int estimated_next_delivery)
{
   game_time_interval_ = std::max(game_time - game_time_start_, 0);
   game_time_start_ = game_time;
   game_time_next_delivery_ = estimated_next_delivery;
   realtime_start_ = platform::get_current_time_in_ms();
}

void Clock::EstimateCurrentGameTime(int &game_time, float& alpha) const
{
   // Compute the ratio of how long it's been since we got a tick from
   // the server and when we expect to get the next tick.  That's the
   // alpha!
   int delta = platform::get_current_time_in_ms() - realtime_start_;
   alpha = static_cast<float>(delta) / game_time_next_delivery_;

   // Clamp alpha...
   alpha = std::min(1.0f, std::max(0.0f, alpha));

   // The next game time send to the client is just the beginning of the
   // interval interpolated up to the next expected interval.  One catch, though:
   // make absolutely sure time never, ever goes backward.
   game_time = static_cast<int>(game_time_start_ + (alpha * game_time_interval_));
   game_time = std::max(game_time, last_reported_time_);

   int changed = game_time - last_reported_time_;
   last_reported_time_ = game_time;

   CLIENT_LOG(5) << "estimate time: " << game_time << "(alpha: " << alpha << " change: " << changed << ")" << "["
                  << " gt start: " << game_time_start_
                  << " gt interval: " << game_time_interval_
                  << " gt next: " << game_time_next_delivery_
                  << " delta t: " << delta << "]";
}
