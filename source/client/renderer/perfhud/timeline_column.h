#ifndef _RADIANT_CLIENT_RENDERER_PERF_HUD_H
#define _RADIANT_CLIENT_RENDERER_PERF_HUD_H

#include "namespace.h"
#include "timeline.h"
#include "lib/perfmon/namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class TimelineColumn
{
public:
   TimelineColumn(Timeline& t, perfmon::Frame*);
   ~TimelineColumn();

   TimelineColumn& Render(RenderContext & rc, csg::Rect2f const& rect);

private:
   struct ColumnEntry {
      CounterData*         counter_data;
      int                  duration;
      ColumnEntry() { }
      ColumnEntry(CounterData* data, uint t) : counter_data(data), duration(t) { }
   };

private:
   int                           duration_;
   std::vector<ColumnEntry>      entries_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERF_HUD_H