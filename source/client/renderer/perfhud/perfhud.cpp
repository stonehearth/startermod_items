#include "pch.h"
#include "radiant.h"
#include "perfhud.h"
#include "lib/perfmon/perfmon.h"
#include "client/renderer/renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

PerfHud::PerfHud(Renderer& r)
{
   guard_ += r.TraceFrameStart([this]() {
      Render();
   });
   guard_ += r.OnScreenResize([this](csg::Point2 const& pt) {
      screen_size_ = pt;
      float aspect = (float)pt.x / pt.y;
      float pad_y = 10.0f / pt.y;
      float pad_x = pad_y * aspect;

      SetBounds(csg::Rect2f(csg::Point2f(pad_x, pad_y),
                            csg::Point2f(aspect - pad_x, 1.0f - pad_y)));
      rc_.SetScreenSize(pt);
   });
   rc_.SetTimelineHeight(perfmon::MillisecondsToCounter(100));
   timeline_.SetMaxColumns(300);
}

PerfHud::~PerfHud()
{
}

PerfHud& PerfHud::Render()
{
   timeline_.Render(rc_, bounds_);
   return *this;
}

Timeline& PerfHud::GetTimeline()
{
   return timeline_;
}


PerfHud& PerfHud::SetBounds(csg::Rect2f const& bounds)
{
   bounds_ = bounds;
   return *this;
}
