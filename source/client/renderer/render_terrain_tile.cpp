#include "pch.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "render_terrain_tile.h"
#include "csg/region_tools.h"
#include "om/region.h"
#include "Horde3D.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderTerrainTile::RenderTerrainTile(RenderTerrain& terrain, csg::Point3 const& location, om::Region3BoxedRef r) :
   _terrain(terrain),
   _location(location),
   _region(r)
{
   om::Region3BoxedPtr region = r.lock();

   if (region) {
      _trace = region->TraceChanges("render", dm::RENDER_TRACES)
                                    ->OnModified([this]{
                                       _terrain.MarkDirty(shared_from_this());
                                    });
   }
}

void RenderTerrainTile::UpdateRenderRegion()
{
   om::Region3BoxedPtr region_ptr = _region.lock();

   if (region_ptr) {
      ASSERT(render_tile);
      csg::Region3 const& region = region_ptr->Get();
      csg::Mesh mesh;

      mesh.SetColorMap(&_terrain.GetColorMap());
      csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
         mesh.AddRegion(plane, pi);
      });

      _node = RenderNode::CreateCsgMeshNode(_terrain.GetGroupNode(), mesh)
                              ->SetMaterial("materials/terrain.material.xml")
                              ->SetUserFlags(UserFlags::Terrain);
   }
}

