#include "pch.h"
#include "path.h"
#include "path_finder.h"
#include "path_finder_dst.h"
#include "movement_helpers.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder.astar, level, name_ << "(dst " << std::setw(5) << id_ << ")")

PathFinderDst::PathFinderDst(Simulation& sim, om::EntityRef e, std::string const& name, ChangedCb changed_cb) :
   sim_(sim),
   entity_(e),
   name_(name),
   changed_cb_(changed_cb),
   moving_(false)
{
   ASSERT(!e.expired());
   om::EntityPtr entity  = entity_.lock();
   if (entity) {
      id_ = entity->GetObjectId();
      PF_LOG(3) << "adding path finder dst for " << *entity;
   }
   CreateTraces();
}

PathFinderDst::~PathFinderDst()
{
   DestroyTraces();
}

void PathFinderDst::CreateTraces()
{
   auto entity = entity_.lock();
   if (entity) {
      PF_LOG(7) << "creating traces";

      auto destination_may_have_changed = [this](const char* reason) {
         auto ep = entity_.lock();
         if (ep) {
            ClipAdjacentToTerrain();
            changed_cb_(reason);
         }
      };

      auto& o = sim_.GetOctTree();
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         moving_ = mob->GetMoving();

         transform_trace_ = mob->TraceTransform("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](csg::Transform const&) {
                                    PF_LOG(7) << "mob transform changed";
                                    destination_may_have_changed("mob transform changed");
                                 });

         moving_trace_ = mob->TraceMoving("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](bool const& moving) {
                                    if (moving != moving_) {
                                       PF_LOG(7) << "mob moving changed";
                                       moving_ = moving;
                                       if (moving_) {
                                          changed_cb_("mob moving changed");
                                       }
                                    }
                                 });
      }
      auto dst = entity->GetComponent<om::Destination>();
      if (dst) {
         region_guard_ = dst->TraceAdjacent("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([this, destination_may_have_changed](csg::Region3 const&) {
                                    PF_LOG(7) << "adjacent region changed";
                                    destination_may_have_changed("adjacent region changed");
                                 });
      }
      ClipAdjacentToTerrain();
   }
}

void PathFinderDst::ClipAdjacentToTerrain()
{
   world_space_adjacent_region_.Clear();

   om::EntityPtr entity = entity_.lock();
   if (entity) {
      world_space_adjacent_region_ = MovementHelper().GetRegionAdjacentToEntity(sim_, entity);
   }
}

float PathFinderDst::EstimateMovementCost(csg::Point3 const& start) const
{
   if (world_space_adjacent_region_.IsEmpty()) {
      return FLT_MAX;
   }
   csg::Point3 end = world_space_adjacent_region_.GetClosestPoint(start);
   return sim_.GetOctTree().GetMovementCost(start, end);
}

csg::Point3 PathFinderDst::GetPointOfInterest(csg::Point3 const& adjacent_pt) const
{
   return MovementHelper().GetPointOfInterest(adjacent_pt, GetEntity());
}

void PathFinderDst::DestroyTraces()
{
   PF_LOG(7) << "destroying traces";
   moving_trace_ = nullptr;
   transform_trace_ = nullptr;
   region_guard_ = nullptr;
}

bool PathFinderDst::IsIdle() const
{
   return world_space_adjacent_region_.IsEmpty();
}

void PathFinderDst::EncodeDebugShapes(radiant::protocol::shapelist *msg, csg::Color4 const& debug_color) const
{
   if (LOG_IS_ENABLED(simulation.pathfinder.astar, 7)) {
      auto region = msg->add_region();
      debug_color.SaveValue(region->mutable_color());
      for (auto const& cube : world_space_adjacent_region_) {
         cube.SaveValue(region->add_cubes());
      }
   }
}

dm::ObjectId PathFinderDst::GetEntityId() const
{
   return id_;
}

om::EntityPtr PathFinderDst::GetEntity() const
{
   return entity_.lock();
}

