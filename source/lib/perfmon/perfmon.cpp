#include "radiant.h"
#include "perfmon.h"
#include "counter.h"
#include "store.h"
#include "timeline.h"

using namespace radiant;
using namespace radiant::perfmon;

__declspec(thread) Store* store__ = nullptr;

static Timeline& GetTimeline()
{
   if (!store__) {
      store__ = new Store();
   }
   return *store__->GetTimeline();
}

FrameGuard::FrameGuard()
{
   BeginFrame();
}

FrameGuard::~FrameGuard()
{
   EndFrame();
}

TimelineCounterGuard::TimelineCounterGuard(const char* name)
{
   last_counter_ = GetTimeline().GetCurrentCounter();
   SwitchToCounter(name);
}

TimelineCounterGuard::~TimelineCounterGuard()
{
   GetTimeline().SetCounter(last_counter_);
}

void perfmon::SwitchToCounter(char const* name)
{
   Timeline& timeline = GetTimeline();
   Counter* counter = timeline.GetCounter(name);
   timeline.SetCounter(counter);
}

void perfmon::BeginFrame()
{
   GetTimeline().BeginFrame();
}

void perfmon::EndFrame()
{
   GetTimeline().EndFrame();
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
