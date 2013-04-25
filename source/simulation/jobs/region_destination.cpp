#include "pch.h"
#include "debug_colors.h"
#include "simulation/simulation.h"
#include "physics/octtree.h"
#include "region_destination.h"
#include "om/region.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

RegionDestination::RegionDestination(om::EntityRef entity, const om::RegionPtr region) :
   Destination(entity),
   region_(region)
{
}

int RegionDestination::EstimateCostToDestination(const math3d::ipoint3& from)
{
   auto e = GetEntity().lock();
   if (e) {
      const auto& o = Simulation::GetInstance().GetOctTree();
      return o.EstimateMovementCost(from, GetRegion());
   }
   return INT_MAX;
}

void RegionDestination::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   auto e = GetEntity().lock();
   if (e) {
      const math3d::color4& color = IsEnabled() ?  DebugColors::standingColor : DebugColors::standingColorDisabled;
      auto region = msg->add_region();
      GetRegion().SaveValue(region);
      color.SaveValue(region->mutable_color());
   }
}

bool RegionDestination::IsEnabled() const
{
   return Destination::IsEnabled() && !GetRegion().IsEmpty();
}

int RegionDestination::GetLastModificationTime() const
{
   return region_->GetLastModified();
}

const csg::Region3& RegionDestination::GetRegion() const
{
   return **region_;
}
