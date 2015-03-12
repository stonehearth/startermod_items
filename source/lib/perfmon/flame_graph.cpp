#include "radiant.h"
#include "flame_graph.h"

using namespace radiant;
using namespace radiant::perfmon;

core::DoubleBuffer<perfmon::FlameGraph> flameGraphs;

StackFrame::StackFrame(core::StaticString name) :
   _name(name),
   _count(1),
   _childTotalCount(0)
{
}

StackFrame* StackFrame::AddStackFrame(core::StaticString name)
{
   ++_childTotalCount;

   // O(n), but much more cache-friendly then a hashtable.
   for (StackFrame& c : _children) {
      if (c._name == name) {
         ++c._count;
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

FlameGraph::FlameGraph() :
   _base("base"),
   _depth(0)
{
}

void FlameGraph::AddLuaBacktrace(const char* backtrace, int len, TranslateFn translateFn)
{
   const char *start = backtrace, *p = backtrace;
   StackFrame *current = &_base;
   int depth = 0;

   for (int i = 0; i < len; i++, p++) {
      if (*p == ';') {
         core::StaticString name(start, p - start);
         current = current->AddStackFrame(translateFn(name));
         start = p + 1;
         ++depth;
      }
   }
   _depth = std::max(_depth, depth);
}

void FlameGraph::Clear()
{
   _base.Clear();
}
