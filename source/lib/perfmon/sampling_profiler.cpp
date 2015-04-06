#include "radiant.h"
#include "sampling_profiler.h"
#include "timer.h"

using namespace radiant;
using namespace radiant::perfmon;

const char* BASE_FRAME = "base";

StackFrame::StackFrame(const char* sourceName, unsigned int fnDefLine) :
   _sourceName(sourceName),
   _fnDefLine(fnDefLine),
   _count(0),
   _childTotalCount(0)
{
}

StackFrame* StackFrame::AddStackFrame(const char* sourceName, unsigned int fnDefLine)
{
   // O(n), but much more cache-friendly then a hashtable.
   for (StackFrame& c : _children) {
      if (c._fnDefLine == fnDefLine && !strcmp(c._sourceName, sourceName)) {
         return &c;
      }
   }
   _children.emplace_back(sourceName, fnDefLine);
   return &_children.back();
}

void StackFrame::Clear()
{
   // we assume the next use of the SamplingProfiler will get an extremely similar
   // stack frame pattern, so zero out the entries rather than clearing the
   // whole table.

   _count = 0;

   for (int i = 0; i < _lineNumTimes.size(); i++) {
      _lineNumTimes[i] = 0;
   }

   for (StackFrame& c : _children) {
      c.Clear();
   }
}

std::vector<StackFrame> const& StackFrame::GetChildren() const
{
   std::vector<StackFrame> &children = const_cast<std::vector<StackFrame>&>(_children);

   std::sort(children.begin(), children.end(), [](StackFrame const& lhs, StackFrame const& rhs) -> bool {
      return strcmp(lhs._sourceName, rhs._sourceName) < 0;
   });
   return _children;
}

void StackFrame::ResolveFnNames(res::ResourceManager2& resMan)
{
   if (strcmp(_sourceName, BASE_FRAME)) {
      _fnName = resMan.MapFileLineToFunction(_sourceName, _fnDefLine);
   } else {
      _fnName = BASE_FRAME;
   }
   for (StackFrame& c : _children) {
      c.ResolveFnNames(resMan);
   }
}

void StackFrame::CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const
{
   bool recursive = std::find(stack.begin(), stack.end(), _fnName) != stack.end();

   if (!recursive) {
      stats[_fnName] += _count;
   }

   stack.emplace_back(_fnName);
   for (StackFrame const& c : _children) {
      c.CollectStats(stats, stack);
   }
   stack.pop_back();
}


void StackFrame::CollectBottomUpStats(FunctionAtLineTimes &stats, FunctionNameStack& stack, int remainingDepth) const
{
   bool recursive = std::find(stack.begin(), stack.end(), _fnName) != stack.end();

   if (!recursive) {
      for (int i = 0; i < _lineNumLookups.size(); i++) {
         stats[BUILD_STRING(_fnName << ":" << _lineNumLookups[i])] += _lineNumTimes[i];
      }
   }

   if (remainingDepth > 0) {
      stack.emplace_back(_fnName);
      for (StackFrame const& c : _children) {
         c.CollectBottomUpStats(stats, stack, remainingDepth - 1);
      }
      stack.pop_back();
   }
}

SamplingProfiler::SamplingProfiler() :
   _base("base", 999999999),
   _first(true),
   _depth(0)
{
}

StackFrame *SamplingProfiler::GetBaseStackFrame()
{
   return &_base;
}

void SamplingProfiler::Clear()
{
   _base.Clear();
}

SamplingProfiler::StackEntry::StackEntry(StackFrame* f) :
   frame(f),
   start(Timer::GetCurrentCounterValueType())
{
}

void SamplingProfiler::ResolveFnNames(res::ResourceManager2& resMan)
{
   _base.ResolveFnNames(resMan);
}

CounterValueType SamplingProfiler::StackEntry::GetElapsed() const
{
   return Timer::GetCurrentCounterValueType() - start;
}

void SamplingProfiler::CollectStats(FunctionTimes& stats) const
{
   _base.CollectStats(stats, StackFrame::FunctionNameStack());
}

void SamplingProfiler::CollectBottomUpStats(FunctionAtLineTimes& stats, int maxDepth) const
{

   _base.CollectBottomUpStats(stats, StackFrame::FunctionNameStack(), maxDepth);
}
