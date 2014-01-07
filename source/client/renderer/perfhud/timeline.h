#ifndef _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H
#define _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H

#include "namespace.h"
#include "timeline_column.h"
#include "core/guard.h"
#include "lib/perfmon/namespace.h"
#include "csg/color.h"
#include "counter_data.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Timeline
{
public:

public:
   Timeline();
   ~Timeline();

   Timeline& SetMaxColumns(uint max);
   Timeline& SetMaxTopTraces(uint max);
   Timeline& Render(RenderContext & rc, csg::Rect2f const& rect);

private:
   friend TimelineColumn;

   CounterData* GetCounterData(std::string const& name);
   CounterData* GetRemainingCounterData();

private:
   void AddFrame(perfmon::Frame* frame);
   void RenderColumns(RenderContext & rc, csg::Point2f min, csg::Point2f max);
   void RenderLegend(RenderContext & rc, csg::Point2f min, csg::Point2f max);
   void ColorColumns();

private:
   std::deque<TimelineColumn>                      columns_;
   uint                                            max_columns_;
   uint                                            max_top_traces_;
   core::Guard                                     perfmon_guard_;
   std::vector<CounterData*>                       counter_data_sorted_;
   std::unordered_map<std::string, CounterData*>   counter_data_;
   std::unordered_map<std::string, perfmon::CounterValueType> counter_times_;

   perfmon::CounterValueType                       total_time_;
   std::vector<csg::Color3>                        colors_;
   csg::Color3                                     default_color_;
   CounterData                                     remaining_counter_data_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H