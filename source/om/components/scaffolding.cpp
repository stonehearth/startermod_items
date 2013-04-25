#include "pch.h"
#include "scaffolding.h"
#include "mob.h"
#include "post.h"
#include "peaked_roof.h"
#include "region_collision_shape.h"
#include "vertical_pathing_region.h"
#include "build_orders.h"
#include "build_order_dependencies.h"
#include "component_helpers.h"
#include "om/grid/grid.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

Scaffolding::Scaffolding() :
   RegionBuildOrder()
{
   LOG(WARNING) << "creating new scaffolding...";
}

Scaffolding::~Scaffolding()
{
}

void Scaffolding::InitializeRecordFields()
{
   RegionBuildOrder::InitializeRecordFields();
   AddRecordField("wall", wall_);
   AddRecordField("ladder", ladder_);
   AddRecordField("roof", roof_);
}

bool Scaffolding::ShouldTearDown()
{
   
   auto wall = (*wall_).lock();
   bool building = wall->NeedsMoreWork();
   if (!building) {
      auto roof = (*roof_).lock();
      if (roof) {
         building = roof->NeedsMoreWork();
      }
   }
   if (building != building_) {
      building_ = building;
      ComputeStandingRegion();
   }
   return !building_;
}

csg::Point3 Scaffolding::GetNormal() const
{
   auto wall = (*wall_).lock();
   return wall ? wall->GetNormal() : csg::Point3();
}

csg::Point3 Scaffolding::GetAbsNormal() const
{
   csg::Point3 normal = GetNormal();
   return csg::Point3(abs(normal.x), abs(normal.y), abs(normal.z));
}

csg::Region3 Scaffolding::GetMissingRegion() const
{
   if (building_) {
      return GetMissingBuildUpRegion();
   }
   return GetMissingTearDownRegion();
}

csg::Region3 Scaffolding::GetMissingBuildUpRegion() const
{
   auto wall = (*wall_).lock();

   csg::Region3 region = wall->GetMissingRegion();
   csg::Point3 normal = GetNormal();
   csg::Point3 absNormal = GetAbsNormal();

   if (!region.IsEmpty()) {
      // More work to do on the wall.  Compute the region...

      // Scaffolding is 2 blocks thick in the direction of the normal and spans
      // the entire width of the wall.  The column at ((0, 0, 0) + normal) is
      // actually a ladder that workers can use the climb the scaffolding.
      csg::Cube3 bounds = region.GetBounds();
      int bottom = bounds.GetMin().y;

      // If the bottom of the wall is already at the base of the structure,
      // we don't need any scaffolding at all.
      if (bottom == 0) { 
         return csg::Region3();
      }
       
      // Use the wall size to compute our region to make sure the
      // scaffolding spans the entire wall (important for the triangle parts
      // of a peaked roof).
      csg::Point3 min = csg::Point3(0, 0, 0);
      csg::Point3 max = wall->GetSize();

      // Construct a square region that reaches up to the bottom of the missing
      // region of the wall and spans the entire distance.  Make sure it's 2
      // units wide.  The wall is already 1 unit wide in the direction of the
      // normal, so just add another one in there to get there.
      bounds = csg::Cube3(csg::Point3(min.x, 0, min.z),
                          csg::Point3(max.x, bottom, max.z) + absNormal);
      // validation...
      csg::Point3 size = bounds.GetSize();
      ASSERT(size.x == 2 || size.z == 2);
      
      // Set the region.
      region = bounds;
      ASSERT(!region.IsEmpty());

      // Snip out the current scaffolding region. What's left are the missing parts
      // of the scaffolding needed to elevate a worker up to the construction region
      // of the wall.
      region -= GetRegion();

   } else {
      auto roof = (*roof_).lock();
      if (roof) {
         // Make sure the ladder reaches the base of the roof.  This is somewhat tricky...
         // If the portion of the roof which overlaps our ladder has been built, we need
         // to poke the ladder up high enough to reach it.  If it has not been built, we
         // just need to get to the top of the wall.
         csg::Point3 position = GetEntityGridLocation(roof);
         const csg::Region3& roofRegion = roof->GetRegion();
         const csg::Region3& wallRegion = wall->GetRegion();
         csg::Cube3 wallBounds = wallRegion.GetBounds();
         csg::Point3 max = wallBounds.GetMax();
         region = csg::Region3(csg::Cube3(csg::Point3(0, 0, 0), csg::Point3(max.x, max.y, max.z)));

         // Snip out the current scaffolding region
         // What's left are the missing parts of the scaffolding needed to elevate a
         // worker up to the construction region of the wall.  
         region -= GetRegion();
      }
   }

   return region;
}

csg::Region3 Scaffolding::GetMissingTearDownRegion() const
{
   csg::Region3 missing = GetRegion();

   // Take only the top rung to make sure we do an balanced tear
   // down
   if (!missing.IsEmpty()) {
      int top = missing.GetBounds().GetMax().y - 1;
      missing -= csg::Cube3(csg::Point3(-INT_MAX, -INT_MAX, -INT_MAX),
                            csg::Point3( INT_MAX,      top,  INT_MAX));
   }
   return missing;
}

void Scaffolding::ComputeStandingRegion()
{
   math3d::ipoint3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   csg::Region3 missing = GetUnreservedMissingRegion();

   csg::Region3& standing = ModifyStandingRegion();
   standing.Clear();

   if (!missing.IsEmpty()) {
      csg::Point3 normal = GetNormal();
      csg::Point3 absNormal = GetAbsNormal();
      if (true || building_) {
         // The missing region will contain the entire, 2-block wide part of the
         // scaffolding that's missing.  We need to be standing at the base of that
         // region, 1-unit away in the direction of the normal.
         int bottom = missing.GetBounds().GetMin().y;

         // Thin the region by 1, keeping only the outer layer
         if (normal.x == 1) {
            missing = missing.ProjectOnto(0, 2);
         } else if (normal.x == -1) {
            missing = missing.ProjectOnto(0, -1);
         } else if (normal.z == 1) {
            missing = missing.ProjectOnto(2, 2);
         } else {
            missing = missing.ProjectOnto(2, -1);
         }
         standing = missing.ProjectOnto(1, 0);
         standing.Translate(origin);
      } else {
         // Project the missing region down to the ground and away from the wall
         int top = missing.GetBounds().GetMax().y - 1;
         normal.y = -top;
         missing.Translate(origin + normal);
      }
      ASSERT(!standing.IsEmpty());
   }
}

bool Scaffolding::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   const csg::Region3 missing = GetUnreservedMissingRegion();
   const csg::Point3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   const csg::Point3 standing = pt - origin;
   const csg::Point3 normal = GetNormal();

   // If we're building up, we want to reserve the bottom block in the missing
   // region (which is at the top of the scaffolding).  If we're tearing down,
   // reserve a block at the top of our region!
   int yOffset;
   if (building_) {
      yOffset = missing.GetBounds().GetMin().y;
   } else {
      yOffset = missing.GetBounds().GetMax().y - 1;
   }

   // There should be a point in the missing region exactly 1 normal unit away
   // at the bottom of the missing region.
   const csg::Point3 block = standing - normal + csg::Point3(0, yOffset, 0);
   if (missing.Contains(block)) {
      const csg::Point3 innerBlock = block - normal;
      reserved = block;
      ModifyReservedRegion() += block;
      ModifyReservedRegion() += innerBlock;
      ComputeStandingRegion();
      return true;
   }
   
   // XXX: CRAZY STUPID TEST
   auto cr = GetConstructionRegion();
   auto r = **cr;
   r.Translate(-origin);
   bool contains = r.Contains(standing);

   LOG(WARNING) << "Scaffolding::ReserveAdjacent " << GetObjectId() << " failed " << standing << ".  Last modify: " << cr->GetLastModified() << 
                   " (" << contains << ").";
   return false;
}

// xxx: this whole function can be GREATLY optimized
void Scaffolding::ConstructBlock(const math3d::ipoint3& block)
{
   csg::Region3& region = ModifyRegion();
   csg::Point3 normal = GetNormal();
   csg::Point3 innerBlock = block - normal;

   bool isLadder;
   if (normal.x + normal.z < 0) {
      isLadder = block.x + block.z == 0;
   } else {
      isLadder = block.x + block.z == 1;
   }
   if (building_) {
      
      if (region.Contains(block)) {
         // ASSERT_BUG(...)
         LOG(WARNING) << "trying to construct duplicate block " << block << "!!  Ignoring.";
         return;
      }

      region += block;
      region += innerBlock;

      if (isLadder) {
         // offset the ladder by the normal to make sure the unit can
         // actually path there (it's not so much of a ladder as a vertical
         // chute).
         (*ladder_)->Modify() += block;
         (*ladder_)->Modify() += block + csg::Point3(0, 1, 0);
         (*ladder_)->Modify() += block + csg::Point3(0, 2, 0);
         (*ladder_)->Modify() += block + normal;
         (*ladder_)->Modify() += block + csg::Point3(0, 1, 0) + normal;
         (*ladder_)->Modify() += block + csg::Point3(0, 2, 0) + normal;
      }
   } else {
      ASSERT(region.Contains(block));

      region -= block;
      region -= innerBlock;
      if (isLadder) {
         (*ladder_)->Modify() -= block + csg::Point3(0, 1, 0);
         (*ladder_)->Modify() -= block + csg::Point3(0, 2, 0);
         (*ladder_)->Modify() -= block + csg::Point3(0, 1, 0) + normal;
         (*ladder_)->Modify() -= block + csg::Point3(0, 2, 0) + normal;
      }
   }
   ModifyReservedRegion() -= block;
   ModifyReservedRegion() -= innerBlock;
   ComputeStandingRegion();
}

void Scaffolding::StartProject(WallPtr wall, const dm::CloneMapping& mapping)
{
   RegionBuildOrder::CreateRegion();
   RegionBuildOrder::StartProject(mapping);

   wall_ = wall;

   // Put the origin of the scaffolding at the base of the wall, in a position
   // where the scaffolding grows in the positive x and z direction.
   csg::Point3 normal = wall->GetNormal();
   csg::Point3 origin = GetEntityGridLocation(wall);
   if (normal.x + normal.z > 0) {
      origin += normal;
   } else {
      origin += normal * 2;
   }
   MobPtr mob = GetEntity().AddComponent<Mob>();
   mob->SetInterpolateMovement(false);
   mob->MoveToGridAligned(origin);

   ComputeStandingRegion();
   auto dep = wall->GetEntity().AddComponent<BuildOrderDependencies>();
   dep->AddDependency(GetEntityRef());

   guards_ += wall->GetRegionPtr()->Trace("scaffolding", [=](const csg::Region3&) {
      ComputeStandingRegion();
   });

   ladder_ = GetStore().AllocObject<Region>();
   GetEntity().AddComponent<VerticalPathingRegion>()->SetRegionPtr(*ladder_);
}

void Scaffolding::SetRoof(GridBuildOrderPtr roof)
{
   roof_ = roof;

   auto dep = roof->GetEntity().AddComponent<BuildOrderDependencies>();
   dep->AddDependency(GetEntityRef());
}
