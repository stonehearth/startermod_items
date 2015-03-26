#include "pch.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "render_terrain_layer.h"
#include "csg/region_tools.h"
#include "om/region.h"
#include "Horde3D.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define T_LOG(level)      LOG(renderer.terrain, level) << "(layer @ " << _location << ") "

RenderTerrainLayer::RenderTerrainLayer(RenderTerrain& terrain, csg::Point3 const& location) :
   _terrain(terrain),
   _location(location)
{
}

RenderTerrainLayer::~RenderTerrainLayer()
{
   T_LOG(5) << "destroying layer";
}

RenderNodePtr RenderTerrainLayer::GetNode()
{
   return _node;
}

void RenderTerrainLayer::BeginUpdate()
{
   for (int i = 0; i < csg::RegionTools3::NUM_PLANES; i++) {
      _geometry[i].clear();
   }
}

void RenderTerrainLayer::AddGeometry(csg::RegionTools3::Plane plane, RenderTerrainTile::Geometry const& g)
{
   for (auto const& entry : g) {
      _geometry[plane][entry.first].AddUnique(entry.second);
   }
}

void RenderTerrainLayer::AddGeometryToMesh(csg::Mesh& mesh, csg::PlaneInfo3 pi)
{
   for (auto& entry : _geometry[pi.which]) {
      csg::Region2 &r = entry.second;
      pi.reduced_value = entry.first;
      r.OptimizeByMerge("meshing terrain layer");
      mesh.AddRegion(r, pi);
   }
}

void RenderTerrainLayer::EndUpdate()
{
   csg::Mesh mesh;
   mesh.SetColorMap(&_terrain.GetColorMap());

   int which = 0;
   for (csg::PlaneInfo3 pi : csg::RegionToolsTraits3::planes) {
      pi.which = which;
      pi.normal_dir = -1;
      AddGeometryToMesh(mesh, pi);

      pi.which++;
      pi.normal_dir = 1;
      AddGeometryToMesh(mesh, pi);

      which += 2;
   }

   if (mesh.IsEmpty()) {
      _node.reset();
   } else {
      _node = RenderNode::CreateCsgModelNode(_terrain.GetGroupNode(), mesh)
                              ->SetMaterial("materials/terrain.material.json")
                              ->SetUserFlags(UserFlags::Terrain);
   }
}
