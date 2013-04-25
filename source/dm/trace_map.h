#pragma once
#include <atomic>
#include <unordered_map>
#include "guard.h"

BEGIN_RADIANT_DM_NAMESPACE

// xxx: this isn't a trace.  this is an object which calls another random
// function when it's destroyed.  traces fire callbacks when data changes.
// you would use one of these to automatically destroy a trace when some
// client object which has registered the callback is destroyed!

class TraceMapData {
protected:
   static std::atomic<dm::TraceId>  nextTraceId_;
};

template <typename K>
class TraceMap : public TraceMapData
{
public:
   TraceMap() : firing_(false) { }

   ~TraceMap() {
      for (const auto& entry : traces_) {
         ASSERT(entry.second.empty());
      }
   }

   typedef std::function<void()> TraceCallbackFn;

   Guard AddTrace(const K& category, TraceCallbackFn fn) {
      TraceId tid = nextTraceId_++;
      traces_[category][tid] = fn;

      return Guard([=]() {
         ASSERT(!firing_);
         ASSERT(stdutil::contains(traces_[category], tid));
         traces_[category].erase(tid);
      });
   }

   void FireTraces(const K& category) {
      firing_ = true;
      for (const auto& entry : traces_[category]) {
         entry.second();
      }
      firing_ = false;
   }

private:
   NO_COPY_CONSTRUCTOR(TraceMap);

private:
   typedef std::unordered_map<TraceId, TraceCallbackFn> Traces;
   typedef std::unordered_map<K, Traces> TraceCategories;

   TraceCategories   traces_;
   bool              firing_;
};

END_RADIANT_DM_NAMESPACE
