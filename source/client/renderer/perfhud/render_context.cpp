#include "pch.h"
#include "Horde3DUtils.h"
#include "render_context.h"
#include "lib/perfmon/timer.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderContext::RenderContext()
{
   material_.reset(h3dAddResource(H3DResTypes::Material, "overlays/panel.material.xml", 0));
   font_material_.reset(h3dAddResource(H3DResTypes::Material, "overlays/font.material.xml", 0));
   timeline_height_t_ = perfmon::MillisecondsToCounter(100);
}


RenderContext::~RenderContext()
{
}

perfmon::CounterValueType RenderContext::GetTimelineHeight() const
{ 
   return timeline_height_t_;
}

RenderContext& RenderContext::SetTimelineHeight(perfmon::CounterValueType value)
{
   timeline_height_t_ = value;
   return *this;
}

void RenderContext::DrawBox(csg::Rect2f const& r, csg::Color3 const& col)
{
   float verts[] = {
      (float)r.min.x, (float)r.min.y, 0, 1,
      (float)r.min.x, (float)r.max.y, 0, 0,
      (float)r.max.x, (float)r.max.y, 1, 0,
      (float)r.max.x, (float)r.min.y, 1, 1
   };

   h3dShowOverlays(verts, 4, col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f, material_.get(), 0);
}

void RenderContext::DrawString(std::string const& s, csg::Point2f const& pt, csg::Color3 const& col, csg::Color3 const& bg)
{
   char const* text = s.c_str();
   h3dutShowText(text, (float)pt.x, (float)pt.y, text_size_, bg.r, bg.g, bg.b, font_material_.get());
   h3dutShowText(text, (float)pt.x, (float)(pt.y + one_pixel_.y), text_size_, bg.r, bg.g, bg.b, font_material_.get());
   h3dutShowText(text, (float)(pt.x + one_pixel_.x), (float)pt.y, text_size_, bg.r, bg.g, bg.b, font_material_.get());
   h3dutShowText(text, (float)(pt.x + one_pixel_.x + one_pixel_.y), (float)pt.y, text_size_, bg.r, bg.g, bg.b, font_material_.get());
   h3dutShowText(text, (float)pt.x, (float)pt.y, text_size_, col.r, col.g, col.b, font_material_.get());
}

void RenderContext::SetScreenSize(csg::Point2 const& size)
{
   one_pixel_.x = 1.0f / size.x;
   one_pixel_.y = 1.0f / size.y;
   text_size_ = (float)(one_pixel_.y * 20);
}

csg::Point2f RenderContext::GetPixelSize()
{
   return one_pixel_;
}
