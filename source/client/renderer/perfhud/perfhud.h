#ifndef _RADIANT_CLIENT_RENDERER_PERFHUD_H
#define _RADIANT_CLIENT_RENDERER_PERFHUD_H

#include "namespace.h"
#include "csg/cube.h"
#include "timeline.h"
#include "render_context.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class PerfHud
{
public:
   PerfHud(Renderer& r);
   ~PerfHud();

   PerfHud& SetMaxColumns(int max);
   PerfHud& SetBounds(csg::Rect2f const& bounds);
   Timeline& GetTimeline();

private:
   PerfHud& Render();

private:
   Timeline       timeline_;
   csg::Rect2f    bounds_;
   csg::Point2    screen_size_;
   core::Guard    guard_;
   RenderContext  rc_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERFHUD_H