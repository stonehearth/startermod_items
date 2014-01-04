#ifndef _RADIANT_CLIENT_RENDERER_PERF_HUD_H
#define _RADIANT_CLIENT_RENDERER_PERF_HUD_H

#include "namespace.h"
#include "timeline.h"
#include "lib/perfmon/namespace.h"
#include <queue>

BEGIN_RADIANT_CLIENT_NAMESPACE

struct ColumnEntry {
   CounterData*               counter_data;
   perfmon::CounterValueType  duration;
   ColumnEntry() { }
   ColumnEntry(CounterData* data, perfmon::CounterValueType t) : counter_data(data), duration(t) { }
};

class TimelineColumn
{
public:
   TimelineColumn(Timeline& t, perfmon::Frame*, uint max_traces);
   ~TimelineColumn();

   TimelineColumn& Render(RenderContext & rc, csg::Rect2f const& rect);

private:
   perfmon::CounterValueType     duration_;
   std::vector<ColumnEntry>      entries_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERF_HUD_H