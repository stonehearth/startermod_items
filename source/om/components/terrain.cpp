#include "pch.h"
#include "terrain.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Terrain::CreateNew()
{
}

void Terrain::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("zones", zones_);
   AddRecordField("zone_size", zone_size_);
   AddRecordField("zone_offset", zone_offset_);
}

int Terrain::GetZoneSize()
{
   return zone_size_;
}

void Terrain::SetZoneSize(int zone_size)
{
   zone_size_ = zone_size;
}

// translation required to transform a zone into world space
void Terrain::SetZoneOffset(csg::Point3 offset)
{
   zone_offset_ = offset;
}

void Terrain::AddZone(csg::Point3 const& region_origin, BoxedRegion3Ptr region3)
{
   // zones are stored using the location of their 0, 0 coordinate in the world
   zones_[region_origin] = region3;
}

void Terrain::PlaceEntity(EntityRef e, const csg::Point3& location)
{
   auto entity = e.lock();
   if (entity) {
      int max_y = INT_MIN;
      csg::Point3 region_origin;
      BoxedRegion3Ptr region_ptr = GetZone(location, region_origin);
      csg::Point3 const& region_local_pt = location - region_origin;

      if (!region_ptr) {
         throw std::invalid_argument(BUILD_STRING("point " << location << " is not in world"));
      }
      csg::Region3 const& region = region_ptr->Get();

      // O(n) search - consider optimizing
      for (csg::Cube3 const& cube : region) {
         if (region_local_pt.x >= cube.GetMin().x && region_local_pt.x < cube.GetMax().x &&
             region_local_pt.z >= cube.GetMin().z && region_local_pt.z < cube.GetMax().z) {
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

BoxedRegion3Ptr Terrain::GetZone(csg::Point3 const& pt, csg::Point3& region_origin)
{
   // transform from world coordinates to zone coordinates
   csg::Point3 const& pt_in_zone_space = pt - zone_offset_;

   // determine the origin of the region containing the point
   csg::Point3 region_origin_in_zone_space;
   for (int i = 0; i < 3; i++) {
      int c = pt_in_zone_space[i];
      if (c >= 0) {
         region_origin_in_zone_space[i] = c / zone_size_ * zone_size_;
      } else {
         // floor towards -infinity, not towards origin
         region_origin_in_zone_space[i] = (c-(zone_size_-1)) / zone_size_ * zone_size_;
      }
   }

   // transform back to world coordinates
   csg::Point3 region_origin_in_world_space;
   region_origin_in_world_space = region_origin_in_zone_space + zone_offset_;

   // get the region from the ZoneMap
   auto entry = zones_.find(region_origin_in_world_space);

   if (entry != zones_.end()) {
      region_origin = entry->first;
      ASSERT(pt.x >= region_origin.x && pt.x < region_origin.x + zone_size_ &&
             pt.z >= region_origin.z && pt.z < region_origin.z + zone_size_);
      return entry->second;
   }
   return nullptr;
}
