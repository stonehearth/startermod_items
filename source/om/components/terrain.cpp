#include "radiant.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "terrain_ring_tesselator.h"
#include "om/entity.h"
#include "om/region.h"
#include "csg/iterators.h"
#include "dm/dm.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

static const csg::Point3 TILE_SIZE(32, 5, 32);
static const double TILE_HEIGHT = 256;

std::ostream& operator<<(std::ostream& os, Terrain const& o)
{
   return (os << "[Terrain]");
}

void Terrain::LoadFromJson(json::Node const& obj)
{
   // TODO: fix how vertical bounds are calculated
   bounds_ = csg::Cube3f(csg::Point3f::zero, csg::Point3f(0, 256, 0));
}

void Terrain::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}

void Terrain::ConstructObject()
{
   Component::ConstructObject();

   terrainRingTesselator_ = std::make_shared<TerrainRingTesselator>();

   config_file_name_trace_ = TraceConfigFileName("terrain", dm::OBJECT_MODEL_TRACES)
      ->OnModified([this]() {
         res::ResourceManager2::GetInstance().LookupJson(config_file_name_, [&](const json::Node& node) {
            terrainRingTesselator_->LoadFromJson(node);
         });
      });
}

TerrainRingTesselatorPtr Terrain::GetTerrainRingTesselator() const
{
   return terrainRingTesselator_;
}

bool Terrain::IsEmpty()
{
   bool empty = GetTiles()->IsEmpty();
   return empty;
}

// if we start streaming in tiles, this will need to return a region instead of a cube
// WARNING: when testing GetBounds().Contains() you must pass in a grid location
csg::Cube3f Terrain::GetBounds()
{
   if (IsEmpty()) {
      throw core::Exception("GetBounds is undefined when there are no tiles."); 
   }

   return bounds_;
}

void Terrain::SetBounds(csg::Cube3f const& bounds)
{
   bounds_ = bounds;
}

void Terrain::GrowBounds(csg::Cube3f const& cube)
{
   bounds_.Modify([&cube](csg::Cube3f& cursor) {
      cursor.Grow(cube);
   });
}

// TODO: Kind of ugly that that we AddTile and GrowBounds outside of TiledRegion API
// (with the accessor obtained from GetTiles). However, the bounds needs to have a persistent datastore
// and not all TiledRegions want to track bounds, so these functions are still separate for the moment.
void Terrain::AddTile(csg::Region3f const& region)
{
   csg::Cube3f region_bounds = region.GetBounds();
   region_bounds.max.y = TILE_HEIGHT;

   if (IsEmpty()) {
      SetBounds(region_bounds);
   } else {
      GrowBounds(region_bounds);
   }

   GetTiles()->AddRegion(region);
}

// TODO: optimize this
csg::Point3f Terrain::GetPointOnTerrain(csg::Point3f const& location)
{
   csg::Point3f grid_location = csg::ToFloat(csg::ToClosestInt(location));
   csg::Point3f point;

   if ((*bounds_).Contains(grid_location)) {
      point = grid_location;
   } else {
      point = (*bounds_).GetClosestPoint(grid_location);
   }

   Region3BoxedTiledPtr tiles = GetTiles();
   bool blocked = tiles->ContainsPoint(point);
   csg::Point3f direction = blocked ? csg::Point3f::unitY : -csg::Point3f::unitY;

   while ((*bounds_).Contains(point)) {
      // if we started blocked, keep going up until open
      // if we started open, keep going down until blocked
      if (tiles->ContainsPoint(point) != blocked) {
         break;
      }
      point += direction;
   }

   if (!blocked) {
      // if we're dropping down to the terrain, return the point above the terrain
      point += csg::Point3f::unitY;
   } else {
      // point is already above the terrain
   }

   return point;
}

Region3BoxedTiledPtr Terrain::CreateTileAccessor(Region3BoxedMapWrapper::TileMap& tiles)
{
   std::shared_ptr<Region3BoxedMapWrapper> wrapper = std::make_shared<Region3BoxedMapWrapper>(tiles);
   return std::make_shared<Region3BoxedTiled>(TILE_SIZE, wrapper);
}

Region3BoxedTiledPtr Terrain::GetTiles()
{
   if (!tile_accessor_) {
      tile_accessor_ = CreateTileAccessor(tiles_);
   }
   return tile_accessor_;
}

Region3BoxedTiledPtr Terrain::GetInteriorTiles()
{
   if (!interior_tile_accessor_) {
      interior_tile_accessor_ = CreateTileAccessor(interior_tiles_);
   }
   return interior_tile_accessor_;
}

csg::Point3 const& Terrain::GetTileSize() const
{
   return TILE_SIZE;
}
