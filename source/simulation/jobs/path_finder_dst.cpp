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
#include "csg/util.h"
#include "a_star_path_finder.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder.astar, level, name_ << "(dst " << std::setw(5) << id_ << ")")

PathFinderDst::PathFinderDst(Simulation& sim, AStarPathFinder& pathfinder, om::EntityRef src, om::EntityRef dst, std::string const& name, ChangedCb changed_cb) :
   sim_(sim),
   dstEntity_(dst),
   srcEntity_(src),
   name_(name),
   pathfinder_(pathfinder),
   changed_cb_(changed_cb)
{
   ASSERT(!srcEntity_.expired());
   ASSERT(!dstEntity_.expired());
   om::EntityPtr entity  = dstEntity_.lock();
   if (entity) {
      id_ = entity->GetObjectId();
      PF_LOG(3) << "adding path finder dst for " << *entity;
   }
   Start();
}

PathFinderDst::~PathFinderDst()
{
   DestroyTraces();
}

void PathFinderDst::Start()
{
   auto dstEntity = dstEntity_.lock();
   if (dstEntity) {
      PF_LOG(7) << "starting path finder dst for " << *dstEntity;

      auto destination_may_have_changed = [this](const char* reason) {
         auto ep = dstEntity_.lock();
         if (ep) {
            ClipAdjacentToTerrain();
            changed_cb_(*this, reason);
         }
      };

      auto& o = sim_.GetOctTree();
      auto mob = dstEntity->GetComponent<om::Mob>();
      if (mob) {
         transform_trace_ = mob->TraceTransform("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](csg::Transform const&) {
                                    PF_LOG(7) << "mob transform changed";
                                    destination_may_have_changed("mob transform changed");
                                 });
      }
      auto dst = dstEntity->GetComponent<om::Destination>();
      if (dst) {
         region_guard_ = dst->TraceAdjacent("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([this, destination_may_have_changed](csg::Region3f const&) {
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

   om::EntityPtr dstEntity = dstEntity_.lock();
   om::EntityPtr srcEntity = srcEntity_.lock();
   if (dstEntity && srcEntity) {
      world_space_adjacent_region_ = MovementHelper().GetRegionAdjacentToEntity(sim_, srcEntity, dstEntity);
      pathfinder_.WatchWorldRegion(world_space_adjacent_region_);
      PF_LOG(7) << "world space region for " << *dstEntity << " is " << world_space_adjacent_region_ << "(bounds:" << world_space_adjacent_region_.GetBounds() << ")";

      phys::OctTree& octTree = sim_.GetOctTree();
      octTree.GetNavGrid().RemoveNonStandableRegion(srcEntity, world_space_adjacent_region_);
      PF_LOG(7) << "standable world space region for " << *dstEntity << " is " << world_space_adjacent_region_ << "(bounds:" << world_space_adjacent_region_.GetBounds() << ")";
   }
}

float PathFinderDst::EstimateMovementCost(csg::Point3f const& start) const
{
   if (world_space_adjacent_region_.IsEmpty()) {
      return FLT_MAX;
   }
   csg::Point3f end = world_space_adjacent_region_.GetClosestPoint(start);
   return sim_.GetOctTree().GetSquaredMovementCost(csg::ToClosestInt(start), csg::ToClosestInt(end));
}

csg::Point3f PathFinderDst::GetPointOfInterest(csg::Point3f const& adjacent_pt) const
{
   return MovementHelper().GetPointOfInterest(csg::ToFloat(adjacent_pt), GetEntity());
}

void PathFinderDst::DestroyTraces()
{
   PF_LOG(7) << "destroying traces";
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
   return dstEntity_.lock();
}

csg::Region3f const& PathFinderDst::GetWorldSpaceAdjacentRegion() const
{
   return world_space_adjacent_region_;
}
