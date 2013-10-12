#ifndef _RADIANT_CLIENT_RENDERER_RENDER_CONTEXT_H
#define _RADIANT_CLIENT_RENDERER_RENDER_CONTEXT_H

#include "namespace.h"
#include "client/renderer/h3d_resource_types.h"
#include "csg/color.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderContext
{
public:
   RenderContext();
   ~RenderContext();

   csg::Color3 GetCounterColor(std::string const& name);
   uint GetTimelineHeightMs() const;

   RenderContext& SetTimelineHeightMs(uint value);

   void DrawBox(csg::Rect2f const& r, csg::Color3 const& col);

private:
   H3DResUnique                                    material_;
   std::unordered_map<std::string, csg::Color3>    counter_colors_;
   std::vector<csg::Color3>                        unused_;
   csg::Color3                                     default_;
   uint                                            timeline_height_ms_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_CONTEXT_H