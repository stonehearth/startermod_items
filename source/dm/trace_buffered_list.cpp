
#include "radiant.h"
#include "radiant_stdutil.h"
#include "trace_buffered_list.h"

using namespace ::radiant;
using namespace ::radiant::dm;

TraceBufferedList::TraceBufferedList() :
   _firing(false)
{
}

bool TraceBufferedList::IsEmpty() const
{
   auto i = _traces.begin(), end = _traces.end();
   while (i != end) {
      if (i->lock()) {
         return false;
      }
      ++i;
   }

   i = _pendingTraces.begin(), end = _pendingTraces.end();
   while (i != end) {
      if (i->lock()) {
         return false;
      }
      ++i;
   }
   return true;
}

void TraceBufferedList::AddTrace(TraceBufferedRef t)
{
   if (!_firing) {
      _traces.push_back(t);
   } else {
      _pendingTraces.push_back(t);
   }
}

void TraceBufferedList::ForEachTrace(std::function<void(TraceBufferedPtr)> cb)
{
   ASSERT(!_firing);
   ASSERT(_pendingTraces.empty());

   _firing = true;
   stdutil::ForEachPrune<TraceBuffered>(_traces, cb);
   _firing = false;

   _traces.insert(_traces.end(), _pendingTraces.begin(), _pendingTraces.end());
   _pendingTraces.clear();
}
