#include "radiant.h"
#include "sampling_profiler.h"
#include "timer.h"

using namespace radiant;
using namespace radiant::perfmon;

const char* TOP_FRAME = "top";

StackFrame::StackFrame(const char* sourceName, unsigned int fnDefLine) :
   _sourceName(sourceName),
   _fnDefLine(fnDefLine),
   _selfTime(0),
   _callCount(1)
{
}

StackFrame* StackFrame::AddStackFrame(const char* sourceName, unsigned int fnDefLine)
{
   // O(n), but much more cache-friendly then a hashtable.
   for (StackFrame& c : _callers) {
      if (c._fnDefLine == fnDefLine && !strcmp(c._sourceName, sourceName)) {
         c._callCount++;
         return &c;
      }
   }
   _callers.emplace_back(sourceName, fnDefLine);
   return &_callers.back();
}

void StackFrame::IncrementTimes(CounterValueType selfTime, int line) { 
   _selfTime += selfTime;

   for (auto& lc : _lines) {
      if (lc.line == line) {
         // We don't really know how long a given line has been 'running', so just record that we saw it.
         lc.count++;
         return;
      }
   }
   _lines.emplace_back(line, 1);
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

void StackFrame::WriteJson(std::ostream& os) const
{
   os << "{" << std::endl
      << "   \"file\": \""       << _sourceName    << "\"," << std::endl
      << "   \"function\": \""   << _fnName        << "\"," << std::endl
      << "   \"line\":   "       << _fnDefLine     << "," << std::endl
      << "   \"selfTime\":   "   << _selfTime      << "," << std::endl
      << "   \"callCount\":   "  << _callCount     << "," << std::endl;

   // lines...
   os << "   \"lines\": [" << std::endl;
   for (uint i = 0; i < _lines.size(); i++) {
      LineCount const& lc = _lines[i];

      os << "{" << std::endl
         << "   \"line\": "   << lc.line << "," << std::endl
         << "   \"count\": "  << lc.count << std::endl
         << "}";
      if (i < _lines.size() - 1) {
         os << ",";
      }
      os << std::endl;
   }
   os << "]," << std::endl;

   // callers...
   os << "   \"callers\": [" << std::endl;
   for (uint i = 0; i < _callers.size(); i++) {
      StackFrame const& caller = _callers[i];
      caller.WriteJson(os);
      if (i < _callers.size() - 1) {
         os << ",";
      }
      os << std::endl;
   }
   os << "]" << std::endl;

   os << "}" << std::endl;
}

SamplingProfiler::SamplingProfiler() :
   _stack("top", 999999999)
{
}

StackFrame *SamplingProfiler::GetStackTop()
{
   return &_stack;
}

SamplingProfiler::StackEntry::StackEntry(StackFrame* f) :
   frame(f),
   start(Timer::GetCurrentCounterValueType())
{
}

void SamplingProfiler::FinalizeCollection(res::ResourceManager2& resMan)
{
   _stack.FinalizeCollection(resMan);
}

CounterValueType SamplingProfiler::StackEntry::GetElapsed() const
{
   return Timer::GetCurrentCounterValueType() - start;
}

void SamplingProfiler::WriteJson(std::ostream& os) const
{
   _stack.WriteJson(os);
}

