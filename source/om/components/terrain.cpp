#include "pch.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "om/entity.h"
#include "om/region.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Terrain const& o)
{
   return (os << "[Terrain]");
}

void Terrain::CreateNew()
{
}

void Terrain::ExtendObject(json::Node const& obj)
{
}

void Terrain::AddZone(csg::Point3 const& zone_offset, Region3BoxedPtr region3)
{
   // zones are stored using the location of their 0, 0 coordinate in the world
   zones_.Add(zone_offset, region3);
}

csg::Cube3 Terrain::GetBounds()
{
   csg::Cube3 result = csg::Cube3::zero;

   if (zones_.Size() > 0) {
      const auto& firstZone = zones_.begin();
      result = csg::Cube3(firstZone->first, firstZone->first + csg::Point3(GetZoneSize(), GetZoneSize(), GetZoneSize()));
      for (const auto& zone : zones_) {
         result.Grow(zone.first);
         result.Grow(zone.first + csg::Point3(GetZoneSize(), GetZoneSize(), GetZoneSize()));
      }
   }
   return result;
}

void Terrain::PlaceEntity(EntityRef e, const csg::Point3& location)
{
   auto entity = e.lock();
   if (entity) {
      int max_y = INT_MIN;
      csg::Point3 zone_offset;
      Region3BoxedPtr region_ptr = GetZone(location, zone_offset);
      csg::Point3 const& regionLocalPt = location - zone_offset;

      if (!region_ptr) {
         throw std::invalid_argument(BUILD_STRING("point " << location << " is not in world"));
      }
      csg::Region3 const& region = region_ptr->Get();

      // O(n) search - consider optimizing
      for (csg::Cube3 const& cube : region) {
         if (regionLocalPt.x >= cube.GetMin().x && regionLocalPt.x < cube.GetMax().x &&
             regionLocalPt.z >= cube.GetMin().z && regionLocalPt.z < cube.GetMax().z) {
            max_y = std::max(cube.GetMax().y, max_y);
         }
      }
      if (max_y != INT_MIN) {
         auto mob = entity->GetComponent<Mob>();
         if (mob) {
            mob->MoveToGridAligned(csg::Point3(location.x, max_y, location.z));
         }
      }
   }
}

Region3BoxedPtr Terrain::GetZone(csg::Point3 const& location, csg::Point3& zone_offset)
{
   // O(n) search - consider optimizing
   for (auto& entry : zones_) {
      zone_offset = entry.first;
      if (location.x >= zone_offset.x && location.x < zone_offset.x + zone_size_ &&
          location.z >= zone_offset.z && location.z < zone_offset.z + zone_size_) {
         return entry.second;
      }
   }
   return nullptr;
}
