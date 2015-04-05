#ifndef _RADIANT_PERFMON_SAMPLING_PROFILER_H
#define _RADIANT_PERFMON_SAMPLING_PROFILER_H

#include <stack>
#include <unordered_map>
#include "namespace.h"
#include "core/static_string.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

typedef std::unordered_map<core::StaticString, CounterValueType, core::StaticString::Hash> FunctionTimes;

class StackFrame {
public:
   typedef std::vector<core::StaticString> FunctionNameStack;

public:
   StackFrame(core::StaticString name);

   core::StaticString GetName() const { return _name; }
   CounterValueType GetCount() const { return _count; }
   void IncrementCount(CounterValueType c) { _count += c; }
   CounterValueType GetChildrenTotalCount() const { return _childTotalCount; }
   std::vector<StackFrame> const& GetChildren() const;

   StackFrame* AddStackFrame(core::StaticString name);
   void Clear();

   void CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const;

private:
   core::StaticString         _name;
   CounterValueType           _count;
   CounterValueType           _childTotalCount;
   std::vector<StackFrame>    _children;
};

class SamplingProfiler
{
public:
   SamplingProfiler();

public:
   void Clear();
   int GetDepth() const { return _depth; }
   StackFrame* GetBaseStackFrame();

   void PushFrame(core::StaticString fn);
   void PopFrame(core::StaticString fn);

   void CollectStats(FunctionTimes& stats) const;

private:
   struct StackEntry {
      StackFrame*       frame;
      CounterValueType  start;

      CounterValueType GetElapsed() const;
      StackEntry(StackFrame* f);
   };
private:
   bool                    _first;
   StackFrame              *_current;
   StackFrame              _base;
   std::stack<StackEntry>  _stack;
   int                     _depth;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_SAMPLING_PROFILER_H
