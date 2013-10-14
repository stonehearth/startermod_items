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

   uint GetTimelineHeightMs() const;

   RenderContext& SetTimelineHeightMs(uint value);
   void SetScreenSize(csg::Point2 const& size);
   void DrawBox(csg::Rect2f const& r, csg::Color3 const& col);
   void DrawString(std::string const& s, csg::Point2f const& pt, csg::Color3 const& col, csg::Color3 const& border = csg::Color3::black);
   csg::Point2f GetPixelSize();

private:
   H3DResUnique                                    material_;
   H3DResUnique                                    font_material_;
   float                                           text_size_;
   csg::Point2f                                    one_pixel_;
   uint                                            timeline_height_ms_;
};


END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_CONTEXT_H