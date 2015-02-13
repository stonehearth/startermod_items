#include "radiant.h"
#include "timer.h"
#include "perfmon.h"
#include "core/config.h"

using namespace radiant;
using namespace radiant::perfmon;

#define T_LOG(level)   LOG(core.timer, level)

// If your cpu has more than 32 cores, you have to resort to shenanigans (cpu groups, or something), to get at them.
// Realistically, if you have that many cores, your QPC is probably implemented correctly....
#define MAX_PROCS 32

// For QPC, we store microseconds.
#define TIMER_RES 1000.0

static LARGE_INTEGER timer_freq_;
static bool broken_ = false;
static bool qpc_init_ = false;


// To quote MSDN: "Does QPC reliably work on multi-processor systems, multi-core system, and systems with hyper-threading?  Yes."
// And: "How do I determine and validate that QPC works on my machine? You don't need to perform such checks." 
// Well... http://youtu.be/gAYL5H46QnQ?t=1m45s .
// Some older broken multi-core CPUs (or is it just BIOSes?) don't return a consistent tick-count from QPC; rather, the tick-frequency is
// dependent on the core on which the thread that QPC is running.  So, loop through every logical core (up to MAX_PROCS!) that the CPU
// has, and compare all those values against one-another.  If they differ by a large amount, we're dealing with a broken implementation,
// and so resort to using timeGetTime.
void radiant::perfmon::Timer_Init() {
   if (qpc_init_) {
      return;
   }

   // Make sure we can force the low_res timer, for testing purposes.
   if (core::Config::GetInstance().Get<bool>("force_low_res_timer", false)) {
      broken_ = true;
      return;
   }   
   qpc_init_ = true;

   DWORD_PTR procMask, sysMask;
   GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);

   // Loop through all our cores (or lots, anyway), and see if any of them differ significantly between their
   // tick counts.  That would indicate a broken QPC implementation.
   std::vector<int64_t> counts;
   int cpuNum = 0;
   while (cpuNum < MAX_PROCS) {
      if (procMask & (1i64 << cpuNum)) {
         LARGE_INTEGER curTick;
		   DWORD_PTR threadAffMask = SetThreadAffinityMask( GetCurrentThread(), 1 << cpuNum);
		   QueryPerformanceFrequency(&timer_freq_);
         QueryPerformanceCounter(&curTick);
         T_LOG(1) << "QPC count: " << curTick.QuadPart << ", frequency: " << timer_freq_.QuadPart;
         counts.push_back(curTick.QuadPart);
      }
      cpuNum++;
   }
	SetThreadAffinityMask( GetCurrentThread(), procMask );

   for (size_t i = 1; i < counts.size(); i++) {
      if (1000 * (std::abs(counts[i - 1] - counts[i]) / (double)timer_freq_.QuadPart) > 5000) {
         // If the difference in counts between two cores is greater than 5 seconds, then there's probably something borked with QPC.
         broken_ = true;
      }
   }

   if (broken_) {
      T_LOG(0) << "QPC doesn't seem reliable; resorting to timeGetTime()";
   }
}

Timer::Timer()
{
   Reset();
}

Timer::~Timer()
{
}

void Timer::Start()
{
   started_ = true;
   start_time_ = GetCurrentCounterValueType();
}

CounterValueType Timer::Stop()
{
   if (!started_) {
      throw std::logic_error("call to Timer::Stop while timer's not running");
   }
   CounterValueType now = GetCurrentCounterValueType();
   CounterValueType elapsed = start_time_ > now ? 0 : now - start_time_; 
   started_ = false;
   elapsed_ += elapsed;
   return elapsed_;
}

CounterValueType Timer::Restart()
{
   if (!started_) {
      throw std::logic_error("call to Timer::Restart while timer's not running");
   }
   CounterValueType now = GetCurrentCounterValueType();
   CounterValueType delta = start_time_ > now ? 0 : now - start_time_; 

   CounterValueType elapsed = delta + elapsed_;
   start_time_ = now;
   elapsed_ = 0;
   return elapsed;
}

void Timer::Reset()
{
   started_ = false;
   elapsed_ = 0;
}

CounterValueType Timer::GetElapsed() const
{
   if (!started_) {
      return elapsed_;
   }

   CounterValueType now = GetCurrentCounterValueType();
   CounterValueType delta = start_time_ > now ? 0 : now - start_time_; 

   return elapsed_ + delta;
}

CounterValueType Timer::GetCurrentCounterValueType()
{
   if (broken_) {
      return static_cast<CounterValueType>(timeGetTime());
   }

   LARGE_INTEGER value;
   QueryPerformanceCounter(&value);
   CounterValueType result = (value.QuadPart * 1000 * (int)TIMER_RES) / timer_freq_.QuadPart;
   return result;
}

uint64 Timer::GetCurrentTimeMs()
{
   return perfmon::CounterToMilliseconds(GetCurrentCounterValueType());
}

uint perfmon::CounterToMilliseconds(CounterValueType value)
{
   if (broken_) {
      return static_cast<uint>(value);
   }
   return static_cast<uint>(ceil(value / TIMER_RES));
}

CounterValueType perfmon::MillisecondsToCounter(uint value)
{
   if (broken_) {
      return static_cast<CounterValueType>(value);
   }
   return static_cast<CounterValueType>(value * TIMER_RES);
}
