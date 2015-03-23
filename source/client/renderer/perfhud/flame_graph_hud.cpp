#include "pch.h"
#include "radiant.h"
#include "flame_graph_hud.h"
#include "lib/perfmon/flame_graph.h"
#include "client/renderer/renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

#pragma optimize( "", off )

FlameGraphHud::FlameGraphHud(Renderer& r)
{
   guard_ += r.OnRenderFrameStart([this](FrameStartInfo const&) {
      perfmon::TimelineCounterGuard tcg("update perfhud");
      Render();
   });
   guard_ += r.OnScreenResize([this](csg::Rect2 const& bounds) {
      screen_size_ = bounds.max - bounds.min;
      float aspect = (float)screen_size_.x / screen_size_.y;
      float pad_y = 10.0f / screen_size_.y;
      float pad_x = pad_y * aspect;

      bounds_.min = csg::Point2f(pad_x, pad_y),
      bounds_.max = csg::Point2f(aspect - pad_x, 1.0f - pad_y);

      rc_.SetScreenSize(screen_size_);
   });
}

FlameGraphHud::~FlameGraphHud()
{
}

FlameGraphHud& FlameGraphHud::Render()
{
   perfmon::FlameGraph &g = flameGraphs.LockFrontBuffer();

   double height = bounds_.GetHeight() / (g.GetDepth() + 1);
   csg::Rect2f rc = bounds_;
   rc.min.y = rc.max.y - height;

   RenderStackFrame(*g.GetBaseStackFrame(), rc, csg::Color3::red);
   flameGraphs.UnlockFrontBuffer();
   return *this;
}

void FlameGraphHud::RenderStackFrame(perfmon::StackFrame const& frame, csg::Rect2f const& rc, csg::Color3 const& color)
{
   rc_.DrawBox(rc, color);

   double height = rc.GetHeight();
   double scale = rc.GetWidth() / frame.GetChildrenTotalCount();

   csg::Rect2f box(csg::Point2f(rc.min.x, rc.min.y - height), csg::Point2f(0, rc.min.y));  // move up a row

   for (perfmon::StackFrame const& c : frame.GetChildren()) {
      csg::Color3 color(rand(), rand(), rand());
      box.max.x = box.min.x + (c.GetCount() * scale);
      RenderStackFrame(c, box, color);
      box.min.x = box.max.x;
   }
}
