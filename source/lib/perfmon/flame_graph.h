#ifndef _RADIANT_PERFMON_FLAMEGRAPH_H
#define _RADIANT_PERFMON_FLAMEGRAPH_H

#include <unordered_map>
#include "namespace.h"
#include "core/static_string.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class StackFrame {
public:
   StackFrame(core::StaticString name);

   core::StaticString GetName() const { return _name; }
   int GetCount() const { return _count; }
   int GetChildrenTotalCount() const { return _childTotalCount; }
   std::vector<StackFrame> const& GetChildren() const;

   StackFrame* AddStackFrame(core::StaticString name);
   void Clear();

private:
   core::StaticString         _name;
   unsigned int               _count;
   unsigned int               _childTotalCount;
   std::vector<StackFrame>    _children;
};

class FlameGraph
{
public:
   FlameGraph();

   typedef std::function<core::StaticString(core::StaticString name)> TranslateFn;
   void AddLuaBacktrace(const char* backtrace, int len, TranslateFn translateFn);
   void Clear();
   int GetDepth() const { return _depth; }
   StackFrame const& GetBaseFrame() const { return _base; }

private:
   StackFrame        _base;
   int               _depth;
};

END_RADIANT_PERFMON_NAMESPACE

// This greatly annoys me, but is good for now.
#include "core/double_buffer.h"
extern ::radiant::core::DoubleBuffer<::radiant::perfmon::FlameGraph> flameGraphs;

#endif // _RADIANT_PERFMON_FLAMEGRAPH_H
