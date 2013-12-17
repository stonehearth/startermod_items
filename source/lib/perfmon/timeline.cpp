#include "radiant.h"
#include "timeline.h"
#include "frame.h"
#include "counter.h"

using namespace radiant;
using namespace radiant::perfmon;

Timeline::Timeline() :
   current_frame_(nullptr),
   current_counter_(nullptr),
   on_frame_end_slot_("frame end")
{
   timer_.Start();
}

void
Timeline::BeginFrame()
{
   // Switch counters to accumulate the last bit...
   if (current_counter_) {
      CounterValueType elapsed = timer_.Restart();
      current_counter_->Increment(elapsed);
      current_counter_ = nullptr;
   }

   // Add the frame to the list
   Frame* last_frame = nullptr;
   if (current_frame_) {
      last_frame = current_frame_;
   }

   // switch frames and restart the counter...
   current_frame_ = new Frame();
   current_counter_ = current_frame_->GetCounter("unaccounted time");

   if (last_frame) {
      // Let everyone know about it.  must be called *after* we've created the
      // new frame so those counters get counted, too
      on_frame_end_slot_.Signal(last_frame);
      delete last_frame;
   }
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
