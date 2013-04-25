#include "pch.h"
#include "om/entity.h"
#include "region_build_order.h"
#include "region_collision_shape.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RegionBuildOrder::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region",            region_);
   AddRecordField("reserved",          reserved_);
   AddRecordField("standing_region",   standingRegion_);   
   AddRecordField("blueprint",         blueprint_);
}

bool RegionBuildOrder::NeedsMoreWork()
{
   return !GetUnreservedMissingRegion().IsEmpty();
}

const RegionPtr RegionBuildOrder::GetConstructionRegion() const
{
   return standingRegion_;
}

csg::Region3 RegionBuildOrder::GetUnreservedMissingRegion() const
{
   return GetMissingRegion() - GetReservedRegion();
}

void RegionBuildOrder::StartProject(const dm::CloneMapping& mapping)
{
   dm::ObjectId id = GetObjectId();
   auto i = mapping.dynamicObjectsInverse.find(id);
   if (i != mapping.dynamicObjectsInverse.end()) {
      // It's ok to not have a blueprint (e.g. scaffolding)...
      blueprint_ = std::static_pointer_cast<RegionBuildOrder>(i->second);
   }

   standingRegion_ = GetStore().AllocObject<Region>();
   ModifyRegion().Clear();
   ModifyReservedRegion().Clear();

   GetEntity().AddComponent<RegionCollisionShape>()->SetRegionPtr(region_);
}

void RegionBuildOrder::CreateRegion()
{
   region_ = GetStore().AllocObject<Region>();
}

csg::Region3 RegionBuildOrder::GetMissingRegion() const
{
   csg::Region3 missing;
   auto blueprint = (*blueprint_).lock();
   if (blueprint) {
      missing = blueprint->GetRegion() - GetRegion();
   } else {
      LOG(WARNING) << "no blueprint in RegionBuildOrder::GetMissingRegion.  Probably a logical error.";
   }
   return missing;
}


void RegionBuildOrder::CompleteTo(int percent)
{
   auto blueprint = (*blueprint_).lock();
   if (!blueprint) {
      return;
   }


   csg::Region3 const& completed = GetRegion();

   while (1) {
      csg::Region3 missing = blueprint->GetRegion() - completed;
      if (missing.IsEmpty()) {
         break;
      }

      int num_completed = completed.GetArea();
      int num_missing = missing.GetArea();
      int desired = percent * (num_completed + num_missing) / 100;

      if (num_completed > desired) {
         break;
      }

      RegionPtr ptr = GetConstructionRegion();
      if (!ptr) {
         break;
      }
      csg::Region3 const& rgn = **ptr;
      if (rgn.IsEmpty()) {
         break;
      }
      csg::Point3 standing  = rgn.begin()->GetMin();
      for (csg::Cube3 const& c : rgn) {
         if (c.GetMin().y < standing.y) {
            standing = c.GetMin();
         }
      }

      math3d::ipoint3 reserved;
      if (!ReserveAdjacent(standing, reserved)) {
         break;
      }
      ConstructBlock(reserved);
      num_completed++;
   }
}
