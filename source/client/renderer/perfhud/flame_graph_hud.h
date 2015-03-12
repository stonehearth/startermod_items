#ifndef _RADIANT_CLIENT_RENDERER_FLAME_GRAPH_HUD_H
#define _RADIANT_CLIENT_RENDERER_FLAME_GRAPH_HUD_H

#include "namespace.h"
#include "csg/cube.h"
#include "timeline.h"
#include "render_context.h"
#include "lib/perfmon/namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class FlameGraphHud
{
public:
   FlameGraphHud(Renderer& r);
   ~FlameGraphHud();

   FlameGraphHud& SetMaxColumns(int max);
   FlameGraphHud& SetBounds(csg::Rect2f const& bounds);
   Timeline& GetTimeline();

private:
   FlameGraphHud& Render();
   void RenderStackFrame(perfmon::StackFrame const& frame, csg::Rect2f const& rc, csg::Color3 const& color);

private:
   csg::Rect2f    bounds_;
   csg::Point2    screen_size_;
   core::Guard    guard_;
   RenderContext  rc_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_FLAME_GRAPH_HUD_H