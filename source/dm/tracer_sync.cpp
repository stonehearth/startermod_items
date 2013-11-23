#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer_sync.h"
#include "alloc_trace.h"

using namespace radiant;
using namespace radiant::dm;

TracerSync::TracerSync()
{
}

TracerSync::~TracerSync()
{
}

void TracerSync::OnObjectChanged(ObjectId id)
{
   // Nothing to do.
}

void TracerSync::OnObjectAlloced(ObjectPtr obj)
{
   stdutil::ForEachPrune<AllocTrace>(alloc_traces_, [obj](AllocTracePtr trace) {
      trace->SignalAlloc(obj);
   });
}

void TracerSync::OnObjectDestroyed(ObjectId id)
{
   // Nothing to do.
}
