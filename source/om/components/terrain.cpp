#include "pch.h"
#include "terrain.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int TILE_SIZE = 256;

void Terrain::CreateNew()
{
}

void Terrain::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("tiles", tiles_);
}

void Terrain::AddRegion(csg::Point3 const& location, BoxedRegion3Ptr r)
{
   tiles_[location] = r;
}

void Terrain::PlaceEntity(EntityRef e, const csg::Point3& pt)
{
   auto entity = e.lock();
   if (entity) {
      int max_y = INT_MIN;
      csg::Point3 const* regionOffset;
      BoxedRegion3Ptr region_ptr = GetRegion(pt, regionOffset);
      csg::Point3 const& regionLocalPt = pt - *regionOffset;

      if (!region_ptr) {
         throw std::invalid_argument(BUILD_STRING("point " << pt << " is not in world"));
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
            mob->MoveToGridAligned(csg::Point3(pt.x, max_y, pt.z));
         }
      }
   }
}

BoxedRegion3Ptr Terrain::GetRegion(csg::Point3 const& pt, csg::Point3 const*& regionOffset)
{
   // O(n) search - consider optimizing
   for (auto& entry : tiles_) {
      csg::Point3 const& zoneOffset = entry.first;
      if (pt.x >= zoneOffset.x && pt.x < zoneOffset.x + TILE_SIZE &&
          pt.z >= zoneOffset.z && pt.z < zoneOffset.z + TILE_SIZE) {
         regionOffset = &zoneOffset;
         return entry.second;
      }
   }
   return nullptr;
}
