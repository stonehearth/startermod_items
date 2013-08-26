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
   AddRecordField("region", region_);
}

void Terrain::AddCube(csg::Cube3 const& cube)
{
   region_.Modify() += cube;
}

void Terrain::RemoveCube(csg::Cube3 const& cube)
{
   region_.Modify() -= cube;
}

void Terrain::AddRegion(csg::Region3 const& region)
{
   region_.Modify() += region;
}

void Terrain::PlaceEntity(EntityRef e, const csg::Point3& pt)
{
   auto entity = e.lock();
   if (entity) {
      int max_y = INT_MIN;
      auto& region = GetRegion();
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
