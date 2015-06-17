#ifndef _RADIANT_PERFMON_SAMPLING_PROFILER_H
#define _RADIANT_PERFMON_SAMPLING_PROFILER_H

#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "namespace.h"
#include "core/static_string.h"
#include "resources\res_manager.h"


BEGIN_RADIANT_PERFMON_NAMESPACE

typedef std::unordered_map<core::StaticString, CounterValueType, core::StaticString::Hash> FunctionTimes;
typedef std::unordered_map<std::string, CounterValueType> FunctionAtLineTimes;

class StackFrame {
public:
   typedef std::vector<const char*> FunctionNameStack;

public:
   StackFrame(const char* _sourceName, unsigned int fnDefLine);

   core::StaticString GetName() const { return _fnName; }
   CounterValueType GetSelfTime() const { return _selfTime; }
   void IncrementTimes(CounterValueType selfTime, int lineNum);

   StackFrame* AddStackFrame(const char* name, unsigned int fnDefLine);

   void FinalizeCollection(res::ResourceManager2& resMan);

   struct LineCount {
      LineCount() : line(0), count(0) { }
      LineCount(int l, CounterValueType c) : line(l), count(c) { }

      int               line;
      CounterValueType  count;
   };

   void WriteJson(std::ostream& os) const;

private:
   unsigned int               _fnDefLine;
   core::StaticString         _sourceName;
   core::StaticString         _fnName;
   CounterValueType           _selfTime;
   std::vector<StackFrame>    _callers;
   std::vector<LineCount>     _lines;
   unsigned int               _callCount;
};

class SamplingProfiler
{
public:
   SamplingProfiler();

public:
   StackFrame* GetStackTop();

   void FinalizeCollection(res::ResourceManager2& resMan);
   void WriteJson(std::ostream& os) const;

private:
   struct StackEntry {
      StackFrame*       frame;
      CounterValueType  start;

      CounterValueType GetElapsed() const;
      StackEntry(StackFrame* f);
   };
private:
   StackFrame              *_current;
   StackFrame              _stack;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_SAMPLING_PROFILER_H
