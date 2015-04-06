#ifndef _RADIANT_PERFMON_SAMPLING_PROFILER_H
#define _RADIANT_PERFMON_SAMPLING_PROFILER_H

#include <stack>
#include <unordered_map>
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
   void IncrementCount(CounterValueType c, int lineNum) { 
      _count += c;

      for (int i = 0; i < _lineNumLookups.size(); i++) {
         if (_lineNumLookups[i] == lineNum) {
            _lineNumTimes[i] += c;
            return;
         }
      }
      _lineNumLookups.push_back(lineNum);
      _lineNumTimes.push_back(c);
   }
   CounterValueType GetChildrenTotalCount() const { return _childTotalCount; }
   std::vector<StackFrame> const& GetChildren() const;

   StackFrame* AddStackFrame(const char* name, unsigned int fnDefLine);
   void Clear();

   void ResolveFnNames(res::ResourceManager2& resMan);
   void CollectStats(FunctionTimes &stats, FunctionNameStack& stack) const;
   void CollectBottomUpStats(FunctionAtLineTimes &stats, FunctionNameStack& stack, int remainingDepth) const;

private:
   unsigned int               _fnDefLine;
   const char*                _sourceName;
   core::StaticString         _fnName;
   CounterValueType           _count;
   CounterValueType           _childTotalCount;
   std::vector<StackFrame>    _children;
   std::vector<CounterValueType> _lineNumTimes;
   std::vector<unsigned int>     _lineNumLookups;
};

class SamplingProfiler
{
public:
   SamplingProfiler();

public:
   void Clear();
   int GetDepth() const { return _depth; }
   StackFrame* GetBaseStackFrame();

   void ResolveFnNames(res::ResourceManager2& resMan);

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
   bool                    _first;
   StackFrame              *_current;
   StackFrame              _base;
   std::stack<StackEntry>  _stack;
   int                     _depth;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_SAMPLING_PROFILER_H
