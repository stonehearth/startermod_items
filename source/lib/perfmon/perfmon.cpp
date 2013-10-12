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
   return *store__->GetTimeline();
}

PerfmonThreadGuard::PerfmonThreadGuard()
{
   store__ = new Store();
}

PerfmonThreadGuard::~PerfmonThreadGuard()
{
   delete store__;
   store__ = nullptr;
}

FrameGuard::FrameGuard()
{
   GetTimeline().BeginFrame();
}

FrameGuard::~FrameGuard()
{
   GetTimeline().EndFrame();
}

TimelineCounterGuard::TimelineCounterGuard(const char* name)
{
   Timeline& timeline = GetTimeline();

   last_counter_ = timeline.GetCurrentCounter();
   Counter* counter = timeline.GetCounter(name);
   timeline.SetCounter(counter);
}

TimelineCounterGuard::~TimelineCounterGuard()
{
   GetTimeline().SetCounter(last_counter_);
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
