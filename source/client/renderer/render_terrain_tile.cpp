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
static const char* planes[] = {
   "BOTTOM",
   "TOP",
   "LEFT",
   "RIGHT",
   "FRONT",
   "BACK"
};

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
   bool atEdge = false;
   csg::Point3 tileSize = _terrain.GetTileSize();

   switch (pi.which) {
   case csg::RegionTools3::LEFT_PLANE:
      atEdge = (pi.reduced_value == _location.x);
      break;
   case csg::RegionTools3::RIGHT_PLANE:
      atEdge = (pi.reduced_value == _location.x + tileSize.x);
      break;
   case csg::RegionTools3::BOTTOM_PLANE:
      atEdge = (pi.reduced_value == _location.y);
      break;
   case csg::RegionTools3::TOP_PLANE:
      atEdge = (pi.reduced_value == _location.y + tileSize.y);
      break;
   case csg::RegionTools3::FRONT_PLANE:
      atEdge = (pi.reduced_value == _location.z);
      break;
   case csg::RegionTools3::BACK_PLANE:
      atEdge = (pi.reduced_value == _location.z + tileSize.z);
      break;
   }
   return atEdge ? _neighborClipPlanes[pi.which] : nullptr;
}

int RenderTerrainTile::UpdateClipPlanes()
{
   om::Region3BoxedPtr region = _region.lock();
   if (region) {
      csg::Point3 tileSize = _terrain.GetTileSize();
      csg::Region3 const& rgn = region->Get();

      for (int d = 0; d < csg::RegionTools3::NUM_PLANES; d++) {
         _clipPlanes[d].Clear();
      }
      for (csg::Cube3 const & cube : csg::EachCube(rgn)) {
         if (cube.min.z == _location.z) {
            T_LOG(9) << "adding plane to FRONT clip plane " << cube << " -> xy -> " << csg::ProjectOntoXY(cube);
            _clipPlanes[csg::RegionTools3::FRONT_PLANE].AddUnique(csg::ProjectOntoXY(cube));
         }
         if (cube.max.z == _location.z + tileSize.z) {
            T_LOG(9) << "adding plane to BACK clip plane " << cube << " -> xy -> " << csg::ProjectOntoXY(cube);
            _clipPlanes[csg::RegionTools3::BACK_PLANE].AddUnique(csg::ProjectOntoXY(cube));
         }
         if (cube.min.x == _location.x) {
            T_LOG(9) << "adding plane to LEFT clip plane " << cube << " -> yz -> " << csg::ProjectOntoYZ(cube);
            _clipPlanes[csg::RegionTools3::LEFT_PLANE].AddUnique(csg::ProjectOntoYZ(cube));
         }
         if (cube.max.x == _location.x + tileSize.x) {
            T_LOG(9) << "adding plane to RIGHT clip plane " << cube << " -> yz -> " << csg::ProjectOntoYZ(cube);
            _clipPlanes[csg::RegionTools3::RIGHT_PLANE].AddUnique(csg::ProjectOntoYZ(cube));
         }
         if (cube.min.y == _location.y) {
            T_LOG(9) << "adding plane to BOTTOM clip plane " << cube << " -> xz -> " << csg::ProjectOntoXZ(cube);
            _clipPlanes[csg::RegionTools3::BOTTOM_PLANE].AddUnique(csg::ProjectOntoXZ(cube));
         }
         if (cube.max.y == _location.y + tileSize.y) {
            T_LOG(9) << "adding plane to TOP clip plane " << cube << " -> xz -> " << csg::ProjectOntoXZ(cube);
            _clipPlanes[csg::RegionTools3::TOP_PLANE].AddUnique(csg::ProjectOntoXZ(cube));
         }
      }
   }
   return -1;     // everything for now... optimize later!
}

void RenderTerrainTile::UpdateGeometry()
{
   om::Region3BoxedPtr region = _region.lock();

   for (int i = 0; i < csg::RegionTools3::NUM_PLANES; i++) {
      _geometry[i].clear();
   }

   if (region) {
      csg::Region3 const& rgn = region->Get();
	  csg::Region3 afterCut(rgn);

	  for (const auto& cut : _cutSet) {
		  afterCut -= csg::ToInt(cut->Get());
	  }

      _regionTools.ForEachPlane(afterCut, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
         Geometry& g = _geometry[pi.which];
         csg::Region2 const* clipper = GetClipPlaneFor(pi);

         if (clipper) {
            T_LOG(9) << "adding clipped " << planes[pi.which] << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << " area: " << (plane - *clipper).GetArea() << ")";
            g[pi.reduced_value].AddUnique(plane - *clipper);
         } else {
            T_LOG(9) << "adding unclipped " << planes[pi.which] << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << ")";
            g[pi.reduced_value].AddUnique(plane);
         }
      });
   }
}

void RenderTerrainTile::SetClipPlane(csg::RegionTools3::Plane direction, csg::Region2 const* clipPlane)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);
   T_LOG(9) << "setting neighbor plane " << planes[direction] << " to " << clipPlane->GetBounds();
   _neighborClipPlanes[direction] = clipPlane;
}

csg::Region2 const* RenderTerrainTile::GetClipPlane(csg::RegionTools3::Plane direction)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);

   return &_clipPlanes[direction];
}

RenderTerrainTile::Geometry const& RenderTerrainTile::GetGeometry(csg::RegionTools3::Plane direction)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);

   return _geometry[direction];
}

bool RenderTerrainTile::BoundsIntersect(csg::Region3 const& other) const
{
	om::Region3BoxedPtr region = _region.lock();

	return region && other.GetBounds().Intersects(region->Get().GetBounds());
}

csg::Point3 const& RenderTerrainTile::GetLocation() const
{
	return _location;
}

void RenderTerrainTile::UpdateCut(om::Region3fBoxedPtr const& cut)
{
	// Just blindly insert.
	_cutSet.insert(cut);
}

void RenderTerrainTile::RemoveCut(om::Region3fBoxedPtr const& cut)
{
   _cutSet.erase(cut);
}
