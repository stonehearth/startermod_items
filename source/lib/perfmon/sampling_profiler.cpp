#include "radiant.h"
#include "sampling_profiler.h"
#include "timer.h"

using namespace radiant;
using namespace radiant::perfmon;

StackFrame::StackFrame(core::StaticString name) :
   _name(name),
   _count(0),
   _childTotalCount(0)
{
}

StackFrame* StackFrame::AddStackFrame(core::StaticString name)
{
   // O(n), but much more cache-friendly then a hashtable.
   for (StackFrame& c : _children) {
      if (c._name == name) {
         return &c;
      }
   }
   _children.emplace_back(name);
   return &_children.back();
}

void StackFrame::Clear()
{
   // we assume the next use of the SamplingProfiler will get an extremely similar
   // stack frame pattern, so zero out the entries rather than clearing the
   // whole table.

   _count = 0;
   for (StackFrame& c : _children) {
      c.Clear();
   }
}

std::vector<StackFrame> const& StackFrame::GetChildren() const
{
   std::vector<StackFrame> &children = const_cast<std::vector<StackFrame>&>(_children);

   std::sort(children.begin(), children.end(), [](StackFrame const& lhs, StackFrame const& rhs) -> bool {
      return strcmp(lhs._name, rhs._name) < 0;
   });
   return _children;
}

void StackFrame::CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const
{
   bool recursive = std::find(stack.begin(), stack.end(), _name) != stack.end();

   if (!recursive) {
      stats[_name] += _count;
   }

   stack.emplace_back(_name);
   for (StackFrame const& c : _children) {
      c.CollectStats(stats, stack);
   }
   stack.pop_back();
}


SamplingProfiler::SamplingProfiler() :
   _base("base"),
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

void SamplingProfiler::PushFrame(core::StaticString fn)
{
   if (_first) {
      _stack.push(&_base);
      _first = false;
   }

   ASSERT(!_stack.empty());
   StackFrame *s = _stack.top().frame->AddStackFrame(fn);
   _stack.push(StackEntry(s));
}

void SamplingProfiler::PopFrame(core::StaticString fn)
{
   StackEntry e = _stack.top();
   _stack.pop();

   if (e.frame->GetName() != fn) {
      LOG_(0) << "popping " << fn << " " << e.frame->GetName() << " " << strcmp(e.frame->GetName(), fn);
   }

   ASSERT(e.frame->GetName() == fn);
   ASSERT(!_stack.empty());

   e.frame->IncrementCount(e.GetElapsed());
}

SamplingProfiler::StackEntry::StackEntry(StackFrame* f) :
   frame(f),
   start(Timer::GetCurrentCounterValueType())
{
}

CounterValueType SamplingProfiler::StackEntry::GetElapsed() const
{
   return Timer::GetCurrentCounterValueType() - start;
}

void SamplingProfiler::CollectStats(FunctionTimes& stats) const
{
   _base.CollectStats(stats, StackFrame::FunctionNameStack());
}
