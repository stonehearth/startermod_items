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
      BoxedRegion3Ptr region_ptr = GetRegion(pt);
      if (!region_ptr) {
         throw std::invalid_argument(BUILD_STRING("point " << pt << " is not in world"));
      }
      csg::Region3 const& region = region_ptr->Get();
      for (csg::Cube3 const& cube : region) {
         if (pt.x >= cube.GetMin().x && pt.x < cube.GetMax().x &&
               pt.z >= cube.GetMin().z && pt.z < cube.GetMax().z) {
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

BoxedRegion3Ptr Terrain::GetRegion(csg::Point3 const& pt)
{
   csg::Point3 location;
   for (int i = 0; i < 3; i++) {
      location[i] = (pt[i] / TILE_SIZE) * TILE_SIZE; // round down...
   }
   auto i = tiles_.find(location);
   return i != tiles_.end() ? i->second : nullptr;
}
