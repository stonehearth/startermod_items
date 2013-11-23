#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer.h"
#include "alloc_trace.h"

using namespace radiant;
using namespace radiant::dm;

Tracer::Tracer()
{
}

Tracer::~Tracer()
{
}

AllocTracePtr Tracer::TraceAlloced(Store const& store, const char* reason)
{
   AllocTracePtr trace = std::make_shared<AllocTrace>(store);
   alloc_traces_.push_back(trace);
   return trace;
}
