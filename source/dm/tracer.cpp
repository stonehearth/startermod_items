#include "pch.h"
#include "trace_map.h"

using namespace ::radiant;
using namespace ::radiant::dm;

std::atomic<dm::TraceId>  TraceMapData::nextTraceId_(1);
