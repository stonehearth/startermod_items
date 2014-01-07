#include "pch.h"
#include "radiant.h"
#include "counter_data.h"
#include "render_context.h"
#include "timeline.h"
#include "timeline_column.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"
#include <numeric>
#include <queue>

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
   perfmon::CounterValueType     duration = 0;
   for (perfmon::Counter const* counter : frame->GetCounters()) {
      perfmon::CounterValueType time = counter->GetValue();

      if (entries.size() < max_traces || entries.top().duration < time) {
         if (entries.size() >= max_traces) {
            entries.pop();
         }
         entries.push(ColumnEntry(t.GetCounterData(counter->GetName()), time));
      }
      duration += time;
   }

   // push the remaining time entry on the front so it will end up at the top
   // of the graph (we render the graph in reverse so big bars end up on the
   // bottom).  As we push the top entries onto the list, subtract their duration
   // from the total.

   entries_.push_back(ColumnEntry(t.GetRemainingCounterData(), 0));
   while (!entries.empty()) {
      ColumnEntry const& counter = entries.top();
      duration -= counter.duration;
      entries_.push_back(counter);
      entries.pop();
   }

   // Set the "remaining time" duration to whatever's left-over.  Now the height of
   // the bar tracks the total time spent, even though we're only rendering a subset
   // of the counters.  yay!
   entries_.front().duration = duration;
}

TimelineColumn::~TimelineColumn()
{
}

TimelineColumn& TimelineColumn::Render(RenderContext & rc, csg::Rect2f const& rect)
{
   csg::Point2f min = rect.GetMin();
   csg::Point2f max = rect.GetMax();
   
   // Draw in reverse order so the bigger bars are on the bottom.
   float offset = max.y, height = max.y - min.y;
   int i, c = (int)entries_.size();
   for (i = c - 1; i >= 0; i--) {
      ColumnEntry const& entry = entries_[i];

      max.y = offset;
      offset -= height * entry.duration / rc.GetTimelineHeight();
      min.y = offset;
      rc.DrawBox(csg::Rect2f(min, max), entry.counter_data->color);
   }
   return *this;
}
