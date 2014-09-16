#include "pch.h"
#include "path_finder.h"
#include "path_finder_src.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/util.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder.astar, level, name_ << "(src)")

PathFinderSrc::PathFinderSrc(om::EntityRef e, std::string const& name, ChangedCb changed_cb) :
   entity_(e),
   name_(name),
   collision_cb_id_(0),
   changed_cb_(changed_cb),
   use_source_override_(false)
{
   ASSERT(!e.expired());
   auto entity = e.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         transform_trace_ = mob->TraceTransform("pf src", dm::PATHFINDER_TRACES)
                                    ->OnChanged([this](csg::Transform const&) {
                                       changed_cb_("src moved");
                                    });
      }
   }
}

PathFinderSrc::~PathFinderSrc()
{
}

void PathFinderSrc::SetSourceOverride(csg::Point3f const& location)
{
   source_override_ = location;
   use_source_override_ = true;
   transform_trace_ = nullptr;
}

void PathFinderSrc::InitializeOpenSet(std::vector<PathFinderNode>& open)
{
   csg::Point3 start;
   if (use_source_override_) {
      start = csg::ToClosestInt(source_override_);
   } else {
      auto entity = entity_.lock();
      if (entity) {
         auto mob = entity->GetComponent<om::Mob>();
         if (!mob) {
            PF_LOG(0) << "source entity has no om::Mob component in pathfinder!";
            return;
         }
<<<<<<< HEAD
         start = csg::ToClosestInt(mob->GetWorldGridLocation());
=======
         om::EntityRef entityRoot;
         start = mob->GetWorldGridLocation(entityRoot);
         if (!om::IsRootEntity(entityRoot)) {
            PF_LOG(0) << "source entity is not in the world";
         }
>>>>>>> ca14fdc5827f8f55cdeb7a34035d5765223ad08a
      }
   }
   open.push_back(start);
   PF_LOG(5) << "initialized open set with entity location " << start;
}

bool PathFinderSrc::IsIdle() const
{
   return false;
}

void PathFinderSrc::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}

csg::Point3f PathFinderSrc::GetSourceLocation() const
{
   return source_location_;
}

void PathFinderSrc::Start(std::vector<PathFinderNode>& open)
{
   if (use_source_override_) {
      source_location_ = source_override_;
   } else {
      auto entity = entity_.lock();
      if (entity) {
         auto mob = entity->GetComponent<om::Mob>();
         if (mob) {
            om::EntityRef entityRoot;
            source_location_ = mob->GetWorldGridLocation(entityRoot);
            if (!om::IsRootEntity(entityRoot)) {
               throw core::Exception("source entity is not in the world"); 
            }
         }
      }
   }
   InitializeOpenSet(open);
}
