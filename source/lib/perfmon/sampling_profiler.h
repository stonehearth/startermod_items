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
   CounterValueType GetCount() const { return _count; }
   void IncrementCount(CounterValueType c, int lineNum);

   StackFrame* AddStackFrame(const char* name, unsigned int fnDefLine);

   void FinalizeCollection(res::ResourceManager2& resMan);
   void CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const;
   void CollectBottomUpStats(FunctionAtLineTimes &stats, FunctionNameStack& stack, int remainingDepth) const;

   struct LineCount {
      LineCount() : line(0), count(0) { }
      LineCount(int l, CounterValueType c) : line(l), count(c) { }

      int               line;
      CounterValueType  count;
   };

   struct SmallFrame {
      std::unordered_map<core::StaticString, unsigned int, core::StaticString::Hash> callers;
      int totalTime;
      std::vector<LineCount>    lines;
   };

   void Fuse(std::unordered_map<core::StaticString, SmallFrame, core::StaticString::Hash> &lookup);


private:
   unsigned int               _fnDefLine;
   const char*                _sourceName;
   core::StaticString         _fnName;
   CounterValueType           _count;
   std::vector<StackFrame>    _callers;
   std::vector<LineCount >    _lines;
   unsigned int               _callCount;
};

typedef std::unordered_map<core::StaticString, StackFrame::SmallFrame, core::StaticString::Hash> FusedFrames;

class SamplingProfiler
{
public:
   SamplingProfiler();

public:
   StackFrame* GetTopInvertedStackFrame();

   void Fuse(FusedFrames &lookup);
   void FinalizeCollection(res::ResourceManager2& resMan);
   void CollectStats(FunctionTimes& stats) const;
   void CollectBottomUpStats(FunctionAtLineTimes &stats, int maxDepth) const;

private:
   struct StackEntry {
      StackFrame*       frame;
      CounterValueType  start;

      CounterValueType GetElapsed() const;
      StackEntry(StackFrame* f);
   };
private:
   StackFrame              *_current;
   StackFrame              _invertedStack;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_SAMPLING_PROFILER_H
