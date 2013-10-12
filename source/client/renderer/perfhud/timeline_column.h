#ifndef _RADIANT_CLIENT_RENDERER_PERF_HUD_H
#define _RADIANT_CLIENT_RENDERER_PERF_HUD_H

#include "namespace.h"
#include "lib/perfmon/namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class TimelineColumn
{
public:
   TimelineColumn(perfmon::Frame*);
   ~TimelineColumn();

   TimelineColumn& Render(RenderContext & rc, csg::Rect2f const& rect);

private:
   int                                          duration_;
   std::vector<std::pair<std::string, uint>>    entries_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERF_HUD_H