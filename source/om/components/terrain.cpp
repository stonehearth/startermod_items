#include "radiant.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "terrain_ring_tesselator.h"
#include "om/region.h"
#include "dm/dm.h"
#include "resources/res_manager.h"
#include "physics/namespace.h"

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
}

void Terrain::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}

void Terrain::ConstructObject()
{
   Component::ConstructObject();

   csg::Point3 tileSize(phys::TILE_SIZE, phys::TILE_SIZE, phys::TILE_SIZE);
   water_tight_region_ = std::make_shared<om::Region3Tiled>(tileSize, water_tight_region_tiles_);
   terrainRingTesselator_ = std::make_shared<TerrainRingTesselator>();
}

Terrain& Terrain::SetConfigFileName(std::string value)
{
   config_file_name_ = value;
   ReadConfigFile();
   return *this;
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
   if (IsEmpty()) {
      SetBounds(cube);
   } else {
      bounds_.Modify([&cube](csg::Cube3f& cursor) {
         cursor.Grow(cube);
      });
   }
}

// TODO: Kind of ugly that that we AddTile and GrowBounds outside of TiledRegion API
// (with the accessor obtained from GetTiles). However, the bounds needs to have a persistent datastore
// and not all TiledRegions want to track bounds, so these functions are still separate for the moment.
void Terrain::AddTile(csg::Region3f const& region)
{
   csg::Cube3f region_bounds = region.GetBounds();
   region_bounds.max.y = TILE_HEIGHT;
   GrowBounds(region_bounds);

   delta_region_ = csg::Region3f(region_bounds);

   GetTiles()->AddRegion(region);
}

// TODO: optimize this
csg::Point3f Terrain::GetPointOnTerrain(csg::Point3f const& location)
{
   csg::Point3f grid_location = csg::ToFloat(csg::ToClosestInt(location));
   csg::Cube3f const& bounds = GetBounds();
   csg::Point3f point;

   if (bounds.Contains(grid_location)) {
      point = grid_location;
   } else {
      point = bounds.GetClosestPoint(grid_location);
   }

   Region3BoxedTiledPtr tiles = GetTiles();
   bool blocked = tiles->ContainsPoint(point);
   csg::Point3f direction = blocked ? csg::Point3f::unitY : -csg::Point3f::unitY;

   while (bounds.Contains(point)) {
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

Region3BoxedTiledPtr Terrain::CreateTileAccessor(dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash>& tiles)
{
   return std::make_shared<Region3BoxedTiled>(TILE_SIZE, tiles);
}

Region3BoxedTiledPtr Terrain::GetTiles()
{
   if (!tile_accessor_) {
      tile_accessor_ = CreateTileAccessor(tiles_);
      tile_accessor_->SetModifiedCb([this](csg::Region3f const& region) {
         TERRAIN_LOG(8) << "Delta region contains " << (*delta_region_).GetCubeCount() << " cubes";
         delta_region_ = region;
      });
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

dm::Boxed<csg::Region3f>* Terrain::GetWaterTightRegionDelta()
{
   return &water_tight_region_delta_;
}

Region3TiledPtr Terrain::GetWaterTightRegion()
{
   return water_tight_region_;
}

void Terrain::ReadConfigFile()
{
   res::ResourceManager2::GetInstance().LookupJson(*config_file_name_, [&](const json::Node& node) {
      terrainRingTesselator_->LoadFromJson(node);
   });
}

void Terrain::OnLoadObject(dm::SerializationType r)
{
   ReadConfigFile();
}
