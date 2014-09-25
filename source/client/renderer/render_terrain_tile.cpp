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

static const char* coords[] = { "x", "y", "z" };
static const char* neighbors[] = {
   "FRONT",
   "BACK",
   "LEFT",
   "RIGHT",
   "TOP",
   "BOTTOM"
};

static const char* PlaneToString(csg::PlaneInfo3 const& pi)
{
   switch (pi.which) {
   case csg::RegionTools3::BOTTOM_PLANE:  return "BOTTOM";
   case csg::RegionTools3::TOP_PLANE:     return "TOP";
   case csg::RegionTools3::LEFT_PLANE:    return "LEFT";
   case csg::RegionTools3::RIGHT_PLANE:   return "RIGHT";
   case csg::RegionTools3::FRONT_PLANE:   return "FRONT";
   case csg::RegionTools3::BACK_PLANE:    return "BACK";
   }
   return "";
}

#define T_LOG(level)      LOG(renderer.terrain, level) << "(tile @ " << _location << ") "

RenderTerrainTile::RenderTerrainTile(RenderTerrain& terrain, csg::Point3 const& location, om::Region3BoxedRef r) :
   _terrain(terrain),
   _location(location),
   _region(r)
{
   om::Region3BoxedPtr region = r.lock();

   for (int i = 0; i < 6; i++) {
      _neighborClipPlanes[i] = nullptr;
   }

   if (region) {
      _trace = region->TraceChanges("render", dm::RENDER_TRACES)
                                    ->OnChanged([this](csg::Region3 const& region) {
                                       _terrain.MarkDirty(_location);
                                    });
   }
}

RenderTerrainTile::~RenderTerrainTile()
{
   T_LOG(5) << "destroying tile";
}

csg::Region2 const* RenderTerrainTile::GetClipPlaneFor(csg::PlaneInfo3 const& pi)
{
   static csg::Region2 zero;

   int plane = -1;
   int tileSize = _terrain.GetTileSize();

   switch (pi.which) {
   case csg::RegionTools3::LEFT_PLANE:
      if (pi.reduced_value == _location.x) {
         plane = LEFT;
      }
      break;
   case csg::RegionTools3::RIGHT_PLANE:
      if (pi.reduced_value == _location.x + tileSize) {
         plane = RIGHT;
      }
      break;
   case csg::RegionTools3::BOTTOM_PLANE:
      if (pi.reduced_value == _location.y) {
         plane = BOTTOM;
      };
      break;
   case csg::RegionTools3::TOP_PLANE:
      if (pi.reduced_value == _location.y + tileSize) {
         plane = TOP;
      }
      break;
   case csg::RegionTools3::FRONT_PLANE:
      if (pi.reduced_value == _location.z) {
         plane = FRONT;
      }
      break;
   case csg::RegionTools3::BACK_PLANE:
      if (pi.reduced_value == _location.z + tileSize) {
         plane = BACK;
      }
      break;
   }
   return plane >= 0 ? _neighborClipPlanes[plane] : nullptr;
}

int RenderTerrainTile::UpdateClipPlanes()
{
   om::Region3BoxedPtr region = _region.lock();
   if (region) {
      int tileSize = _terrain.GetTileSize();
      csg::Region3 const& rgn = region->Get();

      for (int d = 0; d < NUM_NEIGHBORS; d++) {
         _clipPlanes[d].Clear();
      }
      for (csg::Cube3 const & cube : rgn) {
         if (cube.min.z == _location.z) {
            T_LOG(9) << "adding plane to FRONT clip plane " << cube << " -> xy -> " << csg::ProjectOntoXY(cube);
            _clipPlanes[FRONT].AddUnique(csg::ProjectOntoXY(cube));
         }
         if (cube.max.z == _location.z + tileSize) {
            T_LOG(9) << "adding plane to BACK clip plane " << cube << " -> xy -> " << csg::ProjectOntoXY(cube);
            _clipPlanes[BACK].AddUnique(csg::ProjectOntoXY(cube));
         }
         if (cube.min.x == _location.x) {
            T_LOG(9) << "adding plane to LEFT clip plane " << cube << " -> yz -> " << csg::ProjectOntoYZ(cube);
            _clipPlanes[LEFT].AddUnique(csg::ProjectOntoYZ(cube));
         }
         if (cube.max.x == _location.x + tileSize) {
            T_LOG(9) << "adding plane to RIGHT clip plane " << cube << " -> yz -> " << csg::ProjectOntoYZ(cube);
            _clipPlanes[RIGHT].AddUnique(csg::ProjectOntoYZ(cube));
         }
         if (cube.min.y == _location.y) {
            T_LOG(9) << "adding plane to BOTTOM clip plane " << cube << " -> xz -> " << csg::ProjectOntoXZ(cube);
            _clipPlanes[BOTTOM].AddUnique(csg::ProjectOntoXZ(cube));
         }
         if (cube.max.y == _location.y + tileSize) {
            T_LOG(9) << "adding plane to TOP clip plane " << cube << " -> xz -> " << csg::ProjectOntoXZ(cube);
            _clipPlanes[TOP].AddUnique(csg::ProjectOntoXZ(cube));
         }
      }
   }
   return -1;     // everything for now... optimize later!
}

void RenderTerrainTile::UpdateGeometry()
{
   om::Region3BoxedPtr region = _region.lock();

   if (region) {
      ASSERT(render_tile);
      csg::Region3 const& rgn = region->Get();
      csg::Mesh mesh;

      mesh.SetColorMap(&_terrain.GetColorMap());
      _regionTools.ForEachPlane(rgn, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
         csg::Region2 const* clipper = GetClipPlaneFor(pi);
         if (clipper) {
            T_LOG(9) << "adding clipped " << PlaneToString(pi) << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << " area: " << (plane - *clipper).GetArea() << ")";
            if ((plane - *clipper).GetArea() > 0) {
               T_LOG(9) << "OOPS!";
            }
            mesh.AddRegion(plane - *clipper, pi);
         } else {
            T_LOG(9) << "adding unclipped " << PlaneToString(pi) << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << ")";
            mesh.AddRegion(plane, pi);
         }
      });

      _node = RenderNode::CreateCsgMeshNode(_terrain.GetGroupNode(), mesh)
                              ->SetMaterial("materials/terrain.material.xml")
                              ->SetUserFlags(UserFlags::Terrain);
   }
}

void RenderTerrainTile::SetClipPlane(Neighbor direction, csg::Region2 const* clipPlane)
{
   ASSERT(direction >= 0 && direction < NUM_NEIGHBORS);
   T_LOG(9) << "setting neighbor plane " << neighbors[direction] << " to " << clipPlane->GetBounds();
   _neighborClipPlanes[direction] = clipPlane;
}

csg::Region2 const* RenderTerrainTile::GetClipPlane(Neighbor direction)
{
   ASSERT(direction >= 0 && direction < NUM_NEIGHBORS);

   return &_clipPlanes[direction];
}

