#include "radiant.h"
#include "sampling_profiler.h"
#include "timer.h"

using namespace radiant;
using namespace radiant::perfmon;

const char* TOP_FRAME = "top";

StackFrame::StackFrame(const char* sourceName, unsigned int fnDefLine) :
   _sourceName(sourceName),
   _fnDefLine(fnDefLine),
   _count(0)
{
}

StackFrame* StackFrame::AddStackFrame(const char* sourceName, unsigned int fnDefLine)
{
   // O(n), but much more cache-friendly then a hashtable.
   for (StackFrame& c : _callers) {
      if (c._fnDefLine == fnDefLine && !strcmp(c._sourceName, sourceName)) {
         return &c;
      }
   }
   _callers.emplace_back(sourceName, fnDefLine);
   return &_callers.back();
}

void StackFrame::IncrementCount(CounterValueType c, int line) { 
   _count += c;

   for (auto& lc : _lines) {
      if (lc.line == line) {
         lc.count += c;
         return;
      }
   }
   _lines.emplace_back(line, c);
}

void StackFrame::FinalizeCollection(res::ResourceManager2& resMan)
{
   // Figure out the name of the function
   if (strcmp(_sourceName, TOP_FRAME)) {
      _fnName = resMan.MapFileLineToFunction(_sourceName, _fnDefLine);
   } else {
      _fnName = TOP_FRAME;
   }
   for (StackFrame& c : _callers) {
      c.FinalizeCollection(resMan);
   }
}

void StackFrame::CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const
{
   bool recursive = std::find(stack.begin(), stack.end(), _fnName) != stack.end();

   if (!recursive) {
      stats[_fnName] += _count;
   }

   stack.emplace_back(_fnName);
   for (StackFrame const& c : _callers) {
      c.CollectStats(stats, stack);
   }
   stack.pop_back();
}


void StackFrame::CollectBottomUpStats(FunctionAtLineTimes &stats, FunctionNameStack& stack, int remainingDepth) const
{
   bool recursive = std::find(stack.begin(), stack.end(), _fnName) != stack.end();

   if (!recursive) {
      for (auto& lc : _lines) {
         stats[BUILD_STRING(_fnName << ":" << lc.line)] += lc.count;
      }
   }

   if (remainingDepth > 0) {
      stack.emplace_back(_fnName);
      for (StackFrame const& c : _callers) {
         c.CollectBottomUpStats(stats, stack, remainingDepth - 1);
      }
      stack.pop_back();
   }
}

SamplingProfiler::SamplingProfiler() :
   _invertedStack("top", 999999999)
{
}

StackFrame *SamplingProfiler::GetTopInvertedStackFrame()
{
   return &_invertedStack;
}

SamplingProfiler::StackEntry::StackEntry(StackFrame* f) :
   frame(f),
   start(Timer::GetCurrentCounterValueType())
{
}

void SamplingProfiler::FinalizeCollection(res::ResourceManager2& resMan)
{
   _invertedStack.FinalizeCollection(resMan);
}

CounterValueType SamplingProfiler::StackEntry::GetElapsed() const
{
   return Timer::GetCurrentCounterValueType() - start;
}

void SamplingProfiler::CollectStats(FunctionTimes& stats) const
{
   _invertedStack.CollectStats(stats, StackFrame::FunctionNameStack());
}

void SamplingProfiler::CollectBottomUpStats(FunctionAtLineTimes& stats, int maxDepth) const
{

   _invertedStack.CollectBottomUpStats(stats, StackFrame::FunctionNameStack(), maxDepth);
}
