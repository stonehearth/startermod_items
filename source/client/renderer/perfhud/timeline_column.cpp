#include "pch.h"
#include "radiant.h"
#include "counter_data.h"
#include "render_context.h"
#include "timeline.h"
#include "timeline_column.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"

using namespace ::radiant;
using namespace ::radiant::client;

struct ColumnCompare {
   bool operator()(const ColumnEntry &first, const ColumnEntry &second) const
   {
      return first.duration > second.duration;
   }
};

TimelineColumn::TimelineColumn(Timeline& t, perfmon::Frame* frame, uint max_traces)
{
   std::priority_queue<ColumnEntry, std::vector<ColumnEntry>, ColumnCompare> entries;
   duration_ = 0;
   for (perfmon::Counter const* counter : frame->GetCounters()) {
      perfmon::CounterValueType time = counter->GetValue();

      if (entries.size() < max_traces || entries.top().duration < time) {
         if (entries.size() >= max_traces) {
            entries.pop();
         }
         entries.push(ColumnEntry(t.GetCounterData(counter->GetName()), time));
      }
      duration_ += time;
   }

   while (!entries.empty()) {
      entries_.push_back(entries.top());
      entries.pop();
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
      offset -= height * entry.duration / rc.GetTimelineHeight();
      min.y = offset;
      rc.DrawBox(csg::Rect2f(min, max), entry.counter_data->color);
   }
   return *this;
}
