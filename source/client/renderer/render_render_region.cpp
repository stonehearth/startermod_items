#include "pch.h"
#include "pipeline.h"
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
   node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "region"));

   // xxx: TraceSelected is a horribad name!
   selectedGuard_ = Renderer::GetInstance().TraceSelected(node_.get(), std::bind(&RenderRenderRegion::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)); 

   regionGuard_ = render_region->TraceRenderRegion("rendering render_region", [=](csg::Region3 const& r) {
      UpdateRenderRegion(r);
   });
}


void RenderRenderRegion::OnSelected(om::Selection& sel, const csg::Ray3& ray,
                                    const csg::Point3f& intersection, const csg::Point3f& normal)
{
#if 0
   csg::Point3 brick;

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
      H3DNodeUnique mesh = Pipeline::GetInstance().AddMeshNode(node_.get(), entry.second);
      meshes_.emplace_back(mesh);
   }
}
