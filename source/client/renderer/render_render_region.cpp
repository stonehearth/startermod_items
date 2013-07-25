#include "pch.h"
#include "renderer.h"
#include "render_render_region.h"
#include "om/components/render_region.h"
#include "csg/meshtools.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

RenderRenderRegion::RenderRenderRegion(const RenderEntity& entity, om::RenderRegionPtr render_region) :
   entity_(entity)
{
   node_ = h3dAddGroupNode(entity_.GetNode(), "region");
   selectedGuard_ = Renderer::GetInstance().TraceSelected(node_, std::bind(&RenderRenderRegion::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)); 

   auto update_render_region = [=](const csg::Region3 &r){
      UpdateRenderRegion(r);
   };

   auto traceRegionPtr = [=](om::RegionPtr const &v) {
      regionGuard_ = v->Trace("render_region renderer", update_render_region);
      update_render_region(v->GetRegion());
   };

   regionPtrGuard_ = render_region->GetBoxedRegion().TraceValue("render render_region ptr trace", traceRegionPtr);
   traceRegionPtr(render_region->GetRegionPointer());
}


void RenderRenderRegion::OnSelected(om::Selection& sel, const math3d::ray3& ray,
                                    const math3d::point3& intersection, const math3d::point3& normal)
{
#if 0
   math3d::ipoint3 brick;

   for (int i = 0; i < 3; i++) {
      // The brick origin is at the center of mass.  Adding 0.5f to the
      // coordinate and flooring it should return a brick coordinate.
      brick[i] = (int)std::floor(intersection[i] + 0.5f);

      // We want to choose the brick that the mouse is currently over.  The
      // intersection point is actually a point on the surface.  So to get the
      // brick, we need to move in the opposite direction of the normal
      if (fabs(normal[i]) > k_epsilon) {
         brick[i] += normal[i] > 0 ? -1 : 1;
      }
   }
   sel.AddBlock(brick);
#endif
}

void RenderRenderRegion::UpdateRenderRegion(csg::Region3 const& region)
{
   csg::mesh_tools::meshmap meshmap;
   csg::mesh_tools().optimize_region(region, meshmap);
   
   for (auto const& entry : meshmap) {
      H3DNode node;
      // xxx: why is this called TerrainNode?
      node = h3dRadiantCreateTerrainNode(node_, "render_region node", entry.first, entry.second)->GetNode();
   }
}
