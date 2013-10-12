#include "pch.h"
#include "radiant.h"
#include "timeline.h"
#include "lib/perfmon/perfmon.h"

using namespace ::radiant;
using namespace ::radiant::client;

Timeline::Timeline()
{
   SetMaxColumns(60);

   perfmon_guard_ = perfmon::OnFrameEnd([=](perfmon::Frame* f) {
      AddFrame(f);
   });
}

Timeline::~Timeline()
{
}

Timeline& Timeline::SetMaxColumns(uint max)
{
   max_columns_ = max;
   while (columns_.size() > max_columns_) {
      columns_.pop_front();
   }
   return *this;
}

void Timeline::AddFrame(perfmon::Frame* frame)
{
   columns_.emplace_back(TimelineColumn(frame));
   if (columns_.size() > max_columns_) {
      columns_.pop_front();
   }
}

Timeline& Timeline::Render(RenderContext & rc, csg::Rect2f const& r)
{
   csg::Point2f min = r.min;
   csg::Point2f max = r.max;

   uint i = 0, c = columns_.size();
   float width = (max.y - min.y) / c;

   for (TimelineColumn& c : columns_) {
      min.x = r.min.x + (i++ * width);
      max.x = min.x + width;
      c.Render(rc, csg::Rect2f(min, max));
   }
   return *this;
}
