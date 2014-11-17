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
static const int UNKNOWN_TAG = 1;

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

csg::Region2 const* RenderTerrainTile::GetClipPlaneFor(csg::PlaneInfo3 const& pi) const
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

int RenderTerrainTile::UpdateClipPlanes(int clip_height, std::shared_ptr<csg::Region3> xray_tile)
{
   om::Region3BoxedPtr region = _region.lock();
   if (region) {
      csg::Point3 tileSize = _terrain.GetTileSize();
      csg::Region3 cut_terrain_storage;
      csg::Region2 cross_section;
      csg::Region3 const& rgn = ComputeCutTerrainRegion(cut_terrain_storage, clip_height, cross_section, xray_tile);

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

void RenderTerrainTile::UpdateGeometry(int clip_height, std::shared_ptr<csg::Region3> xray_tile)
{
   om::Region3BoxedPtr region = _region.lock();

   for (int i = 0; i < csg::RegionTools3::NUM_PLANES; i++) {
      _geometry[i].clear();
   }

   if (region) {
      csg::Region3 cut_terrain_storage;
      csg::Region2 cross_section;

      // BUG: the cross section is empty when clip_height is on terrain boundaries, so we fail to hide anything
      csg::Region3 const& after_cut = ComputeCutTerrainRegion(cut_terrain_storage, clip_height, cross_section, xray_tile);

      _regionTools.ForEachPlane(after_cut, [this, clip_height, &after_cut, &cross_section](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
         csg::Region2 new_plane;
         Geometry& g = _geometry[pi.which];

         if (pi.which == csg::RegionTools3::TOP_PLANE && pi.reduced_value == clip_height) {
            // only create rects for geometry that extends through the clip plane
            new_plane = cross_section & plane;
            // hide the block types since we can't see inside the earth
            // TODO: verify that the unknown tag is defined in the json
            new_plane.SetTag(UNKNOWN_TAG);
            // add any blocks that extend up to the clip plane, but do not cross it
            csg::Region2 residual = plane - cross_section;
            new_plane += residual;
            //T_LOG(9) << "adding clipped " << planes[pi.which] << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << " area: " << new_plane.GetArea() << ")";
            //g[pi.reduced_value].AddUnique(new_plane);
         } else {
            csg::Region2 const* clipper = GetClipPlaneFor(pi);
            if (clipper) {
               new_plane = plane - *clipper;
              // T_LOG(9) << "adding clipped " << planes[pi.which] << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << " area: " << (plane - *clipper).GetArea() << ")";
               //g[pi.reduced_value].AddUnique(new_plane);
            } else {
               new_plane = plane;
               //T_LOG(9) << "adding unclipped " << planes[pi.which] << " plane (@: " << coords[pi.reduced_coord] << " == " << pi.reduced_value << ")";
               //g[pi.reduced_value].AddUnique(plane);
            }
         }
         g[pi.reduced_value].AddUnique(new_plane);
      });
   }
}

void RenderTerrainTile::SetClipPlane(csg::RegionTools3::Plane direction, csg::Region2 const* clipPlane)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);
   T_LOG(9) << "setting neighbor plane " << planes[direction] << " to " << clipPlane->GetBounds();
   _neighborClipPlanes[direction] = clipPlane;
}

csg::Region2 const* RenderTerrainTile::GetClipPlane(csg::RegionTools3::Plane direction) const
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);

   return &_clipPlanes[direction];
}

RenderTerrainTile::Geometry const& RenderTerrainTile::GetGeometry(csg::RegionTools3::Plane direction) const
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);

   return _geometry[direction];
}

void RenderTerrainTile::AddCut(dm::ObjectId cutId, csg::Region3 const* cut)
{
   _cutMap[cutId] = cut;
}

void RenderTerrainTile::RemoveCut(dm::ObjectId cutId)
{
   _cutMap.erase(cutId);
}

//
// -- RenderTerrainTile::ComputeCutTerrainRegion
//
// Computes the shape of this terrain tile after y-clipping and terrain cuts have been taken
// into consideration.  The `storage` parameter must be guaranteed to live as long as you
// plan to use the result.  If there is no y-clipping or cutting of this tile, this returns
// a direct reference to the terrain tile (super fast!).  If not, it copies the region and
// applies the cuts.
//
csg::Region3 const& RenderTerrainTile::ComputeCutTerrainRegion(csg::Region3& storage, int clip_height, csg::Region2& cross_section, std::shared_ptr<csg::Region3> xray_tile) const
{
   om::Region3BoxedPtr region = _region.lock();
   if (!region) {
      return csg::Region3::zero;
   }

   csg::Point3 tile_size = _terrain.GetTileSize();
   bool is_clipped = csg::IsBetween(_location.y, clip_height, _location.y + tile_size.y);
   bool is_cut = !_cutMap.empty();
   bool is_intersect = xray_tile;

   if (clip_height == _location.y + tile_size.y) {
      // on the upper boundary, the cross section is the bottom clip plane of the top neighbor
      // BUG: unfortunately, that clip plane is empty, because it was clipped out
      csg::PlaneInfo3 pi;
      pi.which = csg::RegionTools3::TOP_PLANE;
      pi.reduced_value = clip_height;
      csg::Region2 const* neighbor_clip_plane = GetClipPlaneFor(pi);
      if (neighbor_clip_plane) {
         cross_section = *neighbor_clip_plane;
      }
   }

   if (!is_cut && !is_clipped && !is_intersect) {
      return region->Get();
   }

   storage = region->Get();

   if (is_intersect) {
      storage &= *xray_tile;
   }

   if (is_cut) {
      for (const auto& cuts : _cutMap) {
         storage -= *cuts.second;
      }
   }

   // modify the geometry if the clipping plane intersects this tile
   if (is_clipped) {
      T_LOG(9) << "removing geometry above clipping plane";
      csg::RegionTools3 tools;
      cross_section = tools.GetCrossSection(storage, 1, clip_height);

      csg::Cube3 hidden_volume(_location, _location + tile_size);
      hidden_volume.min.y = clip_height;
      storage -= hidden_volume;
   }

   return storage;
}

