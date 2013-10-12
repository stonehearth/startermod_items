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
   });
   rc_.SetTimelineHeightMs(100);
   timeline_.SetMaxColumns(60);
   SetBounds(csg::Rect2f(csg::Point2f(0.2f, 0.2f), csg::Point2f(0.8f, 0.8f)));
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
