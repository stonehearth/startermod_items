#ifndef _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H
#define _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H

#include "namespace.h"
#include "timeline_column.h"
#include "core/guard.h"
#include "lib/perfmon/namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Timeline
{
public:
   Timeline();
   ~Timeline();

   Timeline& SetMaxColumns(uint max);
   Timeline& Render(RenderContext & rc, csg::Rect2f const& rect);

private:
   void AddFrame(perfmon::Frame* frame);
   
private:
   std::deque<TimelineColumn>      columns_;
   uint                            max_columns_;
   core::Guard                     perfmon_guard_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_PERFHUD_TIMELINE_H