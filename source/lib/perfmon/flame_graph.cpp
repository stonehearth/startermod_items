#include "radiant.h"
#include "flame_graph.h"
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
   // we assume the next use of the FlameGraph will get an extremely similar
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

void StackFrame::CollectStats(TimeTable &stats, FunctionNameStack& stack) const
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


FlameGraph::FlameGraph() :
   _base("base"),
   _first(true),
   _depth(0)
{
}

StackFrame *FlameGraph::GetBaseStackFrame()
{
   return &_base;
}

void FlameGraph::Clear()
{
   _base.Clear();
}

void FlameGraph::PushFrame(core::StaticString fn)
{
   if (_first) {
      _stack.push(&_base);
      _first = false;
   }

   ASSERT(!_stack.empty());
   StackFrame *s = _stack.top().frame->AddStackFrame(fn);
   _stack.push(StackEntry(s));
}

void FlameGraph::PopFrame(core::StaticString fn)
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

FlameGraph::StackEntry::StackEntry(StackFrame* f) :
   frame(f),
   start(Timer::GetCurrentCounterValueType())
{
}

CounterValueType FlameGraph::StackEntry::GetElapsed() const
{
   return Timer::GetCurrentCounterValueType() - start;
}

void FlameGraph::CollectStats(TimeTable& stats) const
{
   _base.CollectStats(stats, StackFrame::FunctionNameStack());
}
