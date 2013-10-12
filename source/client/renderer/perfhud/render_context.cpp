#include "pch.h"
#include "render_context.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderContext::RenderContext() :
   default_(128, 128, 128)
{
   csg::Color3 const colors[] = {
      csg::Color3(255, 0,     0),
      csg::Color3(  0, 255,   0),
      csg::Color3(  0, 0,   255),
      csg::Color3(255, 255,   0),
      csg::Color3(  0, 255, 255),
   };
   for (auto const& c : colors) {
      unused_.push_back(c);
   }
   material_.reset(h3dAddResource(H3DResTypes::Material, "overlays/panel.material.xml", 0));
   counter_colors_["process job queue"] = csg::Color3(255, 0, 0);
   counter_colors_["process msgs"] = csg::Color3(0, 255, 0);
   counter_colors_["send msgs"] = csg::Color3(0, 128, 0);
   counter_colors_["fire traces"] = csg::Color3(0, 0, 255);
   counter_colors_["mainloop"] = csg::Color3(255, 255, 255);
   counter_colors_["poll browser"] = csg::Color3(255, 255, 0);
   counter_colors_["render"] = csg::Color3(255, 128, 0);
   counter_colors_["lua gc"] = csg::Color3(128, 0, 0);
   counter_colors_["unaccounted time"] = csg::Color3(128, 128, 128);
}


RenderContext::~RenderContext()
{
}

csg::Color3 RenderContext::GetCounterColor(std::string const& name)
{
   auto i = counter_colors_.find(name);
   if (i != counter_colors_.end()) {
      return i->second;
   }
   if (unused_.empty()) {
      LOG(WARNING) << "ran out of materials when trying to draw perf counter  " << name;
      return default_;
   }
   csg::Color3 color = unused_.back();
   unused_.pop_back();
   counter_colors_[name] = color;
   return color;
}

uint RenderContext::GetTimelineHeightMs() const
{
   return timeline_height_ms_;
}

RenderContext& RenderContext::SetTimelineHeightMs(uint value)
{
   timeline_height_ms_ = value;
   return *this;
}

void RenderContext::DrawBox(csg::Rect2f const& r, csg::Color3 const& col)
{
	float verts[] = {
      r.min.x, r.min.y, 0, 1,
      r.min.x, r.max.y, 0, 0,
	   r.max.x, r.max.y, 1, 0,
      r.max.x, r.min.y, 1, 1
   };

   h3dShowOverlays(verts, 4, col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 0.8f, material_.get(), 0);
}
