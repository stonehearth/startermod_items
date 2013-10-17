#include "pch.h"
#include "radiant.h"
#include "timeline.h"
#include "counter_data.h"
#include "render_context.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"

using namespace ::radiant;
using namespace ::radiant::client;

Timeline::Timeline() :
   default_color_(255, 255, 255),
   total_time_(0)
{
   csg::Color3 const colors[] = {
      csg::Color3::FromString("#bf3632"),
      csg::Color3::FromString("#ffb300"),
      csg::Color3::FromString("#5e172d"),
      csg::Color3::FromString("#d95e00"),
      csg::Color3::FromString("#002857"),
      csg::Color3::FromString("#55517b"),
      csg::Color3::FromString("#65551c"),
      csg::Color3::FromString("#8683a4"),
      csg::Color3::FromString("#d3cd8b"),
      csg::Color3::FromString("#aea444"),
      csg::Color3::FromString("#007a87"),
      csg::Color3::FromString("#21578a"),
   };
   for (auto const& c : colors) {
      colors_.push_back(c);
   }

   SetMaxColumns(300);

   perfmon_guard_ = perfmon::OnFrameEnd([=](perfmon::Frame* f) {
      AddFrame(f);
   });
}

Timeline::~Timeline()
{
   for (auto& entry : counter_data_) {
      delete entry.second;
   }
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
   for (perfmon::Counter const* counter : frame->GetCounters()) {
      perfmon::CounterValueType time = counter->GetValue();
      total_time_ += time;
      counter_times_[counter->GetName()] += time;
   }

   columns_.emplace_back(TimelineColumn(*this, frame));
   if (columns_.size() > max_columns_) {
      columns_.pop_front();
   }
}

Timeline& Timeline::Render(RenderContext & rc, csg::Rect2f const& r)
{
   ColorColumns();

   csg::Point2f min = r.min;
   csg::Point2f max = r.max;

   float mid = min.x + (max.x - min.x) * 3 / 4;

   RenderColumns(rc, min, csg::Point2f(mid, max.y));
   RenderLegend(rc, csg::Point2f(mid, min.y), max);
   return *this;
}

void Timeline::RenderColumns(RenderContext & rc, csg::Point2f min, csg::Point2f max)
{
   uint i = 0;
   float width = (max.x - min.x) / max_columns_;

   csg::Point2f cmin = min;
   csg::Point2f cmax = max;
   for (TimelineColumn& c : columns_) {
      cmin.x = min.x + (i++ * width);
      cmax.x = cmin.x + width;
      c.Render(rc, csg::Rect2f(cmin, cmax));
   }
}

void Timeline::RenderLegend(RenderContext &rc, csg::Point2f min, csg::Point2f max)
{
   float box_height = 16;
   csg::Point2f one = rc.GetPixelSize();
   csg::Rect2f box(min, min + one * box_height);
   float h = (box_height + 4) * one.y;

   for (CounterData* c : counter_data_sorted_) {
      rc.DrawBox(box, c->color);
      rc.DrawString(c->name, box.min + csg::Point2f(one.x * (box_height + 5), 0), csg::Color3::white);
      box.min.y += h;
      box.max.y += h;
   }
}

CounterData* Timeline::GetCounterData(std::string const& name)
{
   auto i = counter_data_.find(name);
   if (i != counter_data_.end()) {
      return i->second;
   }

   CounterData* cd = new CounterData(name);
   counter_data_[name] = cd;
   counter_data_sorted_.push_back(cd);
   return cd;
}

void Timeline::ColorColumns()
{
   std::sort(counter_data_sorted_.begin(), counter_data_sorted_.end(),
               [=](CounterData* lhs, CounterData* rhs) {
                  return counter_times_[lhs->name] > counter_times_[rhs->name];
               });

   uint i, c = counter_data_sorted_.size(), color_count = colors_.size();
   for (i = 0; i < c; i++) {
      counter_data_sorted_[i]->color = i < color_count ? colors_[i] : default_color_;
   }
}

