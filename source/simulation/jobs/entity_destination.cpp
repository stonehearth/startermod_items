#include "pch.h"
#include "debug_colors.h"
#include "simulation/simulation.h"
#include "dm/store.h"
#include "physics/octtree.h"
#include "entity_destination.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

EntityDestination::EntityDestination(om::EntityRef entity, const PointList& dst) :
   Destination(entity),
   dst_(dst)
{
   modified_ = Simulation::GetInstance().GetStore().GetCurrentGenerationId();
}

int EntityDestination::EstimateCostToDestination(const math3d::ipoint3& from)
{
   const auto& o = Simulation::GetInstance().GetOctTree();
   return o.EstimateMovementCost(from, dst_);
}

void EntityDestination::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   for (auto& to : dst_) {
      const math3d::color4& color = IsEnabled() ?  DebugColors::standingColor : DebugColors::standingColorDisabled;
      to.SaveValue(msg->add_coords(), color);
   }
}

int EntityDestination::GetLastModificationTime() const
{
   return modified_;
}
