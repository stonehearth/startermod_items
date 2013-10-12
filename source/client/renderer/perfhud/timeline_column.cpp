#include "pch.h"
#include "radiant.h"
#include "timeline_column.h"
#include "render_context.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"

using namespace ::radiant;
using namespace ::radiant::client;

TimelineColumn::TimelineColumn(perfmon::Frame* frame)
{
   duration_ = 0;
   for (perfmon::Counter const* counter : frame->GetCounters()) {
      uint time = perfmon::CounterToMilliseconds(counter->GetValue());
      duration_ += time;
      entries_.push_back(std::make_pair(counter->GetName(), time));
   }
}

TimelineColumn::~TimelineColumn()
{
}

TimelineColumn& TimelineColumn::Render(RenderContext & rc, csg::Rect2f const& rect)
{
   csg::Point2f min = rect.GetMin();
   csg::Point2f max = rect.GetMax();

   float offset = max.y, height = max.y - min.y;
   for (const auto& entry : entries_) {
      max.y = offset;
      offset -= height * entry.second / rc.GetTimelineHeightMs();
      min.y = offset;
      rc.DrawBox(csg::Rect2f(min, max), rc.GetCounterColor(entry.first));
   }
   return *this;
}
