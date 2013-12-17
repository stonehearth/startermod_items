#include "radiant.h"
#include "perfmon.h"
#include "counter.h"
#include "store.h"
#include "timeline.h"

using namespace radiant;
using namespace radiant::perfmon;

// performance counters are somewhat expensive when used in inner loops.  they're
// still useful, though, so work to minimize that overhead when we don't need time.
// If enabled__ is false, do nothing in any of the perfmon functions.  enabled__ is
// set/cleared on a per-frame basis (see perfmon::BeginFrame).
static bool enabled__;

__declspec(thread) Store* store__ = nullptr;

static Timeline& GetTimeline()
{
   if (!store__) {
      store__ = new Store();
   }
   return *store__->GetTimeline();
}

TimelineCounterGuard::TimelineCounterGuard(const char* name)
{
   if (enabled__) {
      last_counter_ = GetTimeline().GetCurrentCounter();
      SwitchToCounter(name);
   }
}

TimelineCounterGuard::~TimelineCounterGuard()
{
   if (enabled__) {
      GetTimeline().SetCounter(last_counter_);
   }
}

void perfmon::SwitchToCounter(char const* name)
{
   if (enabled__) {
      Timeline& timeline = GetTimeline();
      Counter* counter = timeline.GetCounter(name);
      timeline.SetCounter(counter);
   }
}

void perfmon::BeginFrame(bool enabled)
{
   enabled__ = enabled;
   if (enabled__) {
      GetTimeline().BeginFrame();
   }
}

core::Guard perfmon::OnFrameEnd(std::function<void(Frame*)> fn)
{
   return GetTimeline().OnFrameEnd(fn);
}

uint perfmon::CounterToMilliseconds(CounterValueType value)
{
   LARGE_INTEGER li;
   QueryPerformanceFrequency(&li);  // counts per second...
   return static_cast<uint>(ceil(1000 * value / li.QuadPart));
}

CounterValueType perfmon::MillisecondsToCounter(uint value)
{
   LARGE_INTEGER li;
   QueryPerformanceFrequency(&li);  // counts per second...
   return value * li.QuadPart / 1000;
}
