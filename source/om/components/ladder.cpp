#include "pch.h"
#include "ladder.h"
#include "mob.h"
#include "post.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/grid/grid.h"
#include "component_helpers.h"

using namespace ::radiant;
using namespace ::radiant::om;

Ladder::Ladder() :
   BuildOrder()
{
   LOG(WARNING) << "creating new ladder...";
}

Ladder::~Ladder()
{
}

void Ladder::InitializeRecordFields()
{
   BuildOrder::InitializeRecordFields();
   AddRecordField("wall", wall_);
}

void Ladder::SetSupportStructure(WallPtr wall)
{
   wall_ = wall;
   wall->AddDependency(GetEntityRef());
}

csg::Region3 Ladder::GetMissingRegion() const
{
   WallPtr supporting = GetSupportingStructure();

   csg::Region3 region = supporting->GetMissingRegion();
   const int bottom = region.GetBounds().GetMin()[1];

   ASSERT(bottom >= 0);
   if (bottom == 0) {
      return csg::Region3();
   }
   return csg::Cube3(csg::Point3(1, bottom, 1)) - GetRegion();
}

void Ladder::ComputeStandingRegion()
{
   csg::Region3& result = ModifyStandingRegion();

   math3d::ipoint3 location = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();

   result += location + math3d::ipoint3( 1,  0,  0);
   result += location + math3d::ipoint3(-1,  0,  0);
   result += location + math3d::ipoint3( 0,  0,  1);
   result += location + math3d::ipoint3( 0,  0, -1);
}

bool Ladder::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   // reserve the bottom block...
   csg::Region3 region = GetUnreservedMissingRegion();
   if (region.IsEmpty()) {
      return false;
   }
   reserved = region.begin()->GetMin();
   ModifyReservedRegion() += reserved;
   return true;
}

void Ladder::ConstructBlock(const math3d::ipoint3& block)
{
   // Doesn't really matter which block, since the workers could come in a
   // different order than the reservations when out

   // knock off the bottom reservation...
   csg::Region3& reserved = ModifyReservedRegion();
   reserved -= reserved.begin()->GetMin();

   // grow the ladder by one.
   csg::Region3 missing = GetMissingRegion();
   if (!missing.IsEmpty()) {
      auto pt = missing.begin()->GetMin();
      ModifyRegion() += pt;
   }
}

void Ladder::StartProject()
{
   BuildOrder::CreateRegion();
   BuildOrder::StartProject();
   ComputeStandingRegion();
}

