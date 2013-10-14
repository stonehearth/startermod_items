#include "radiant.h"
#include "timeline.h"
#include "frame.h"
#include "counter.h"

using namespace radiant;
using namespace radiant::perfmon;

Timeline::Timeline() :
   current_frame_(nullptr),
   current_counter_(nullptr)
{
   timer_.Start();
}

void
Timeline::BeginFrame()
{
   VERIFY(!current_frame_, "call to BeginFrame while in-frame");

   current_frame_ = new Frame();
   current_counter_ = current_frame_->GetCounter("unaccounted time");

   // don't restart the timer.  we intentionally bill the slop time of the previous
   // frame to the current frame.  by "slop time", I mean the time between the last
   // call to EndFrame and this call to BeginFrame
}

void Timeline::EndFrame()
{
   VERIFY(current_frame_, "call to EndFrame while out-of-frame");

   Frame* last_frame = current_frame_;
   frames_.push_front(current_frame_);
   current_frame_ = nullptr;
   current_counter_ = nullptr;

   on_frame_end_slot_.Signal(last_frame);
}

Counter* Timeline::GetCurrentCounter()
{
   return current_counter_;
}

Counter* Timeline::GetCounter(char const* name)
{
   VERIFY(current_frame_, "call to GetCounter while out-of-frame");

   return current_frame_->GetCounter(name);
}

Counter* Timeline::SetCounter(Counter* counter)
{
   VERIFY(current_frame_, "call to SetCounter while out-of-frame");
   VERIFY(counter, "nullptr passed to SetCurrentFrameCounter");

   CounterValueType elapsed = timer_.Restart();
   current_counter_->Increment(elapsed);

   Counter* last_counter = current_counter_;
   current_counter_ = counter;

   return last_counter;
}

core::Guard Timeline::OnFrameEnd(std::function<void(Frame*)> const& fn)
{
   return on_frame_end_slot_.Register(fn);
}
