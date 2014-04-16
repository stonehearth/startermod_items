#include "pch.h"
#include "path_finder.h"
#include "path_finder_src.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder.astar, level, name_ << "(src)")

PathFinderSrc::PathFinderSrc(om::EntityRef e, std::string const& name, ChangedCb changed_cb) :
   entity_(e),
   name_(name),
   moving_(false),
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

         moving_trace_ = mob->TraceMoving("pf src", dm::PATHFINDER_TRACES)
                                    ->OnChanged([this](bool const& moving) {
                                       moving_ = moving;
                                       if (moving_) {
                                          changed_cb_("src moving changed");
                                      }
                                    });
      }
   }
}

PathFinderSrc::~PathFinderSrc()
{
}

void PathFinderSrc::SetSourceOverride(csg::Point3 const& location)
{
   source_override_ = location;
   use_source_override_ = true;
   transform_trace_ = nullptr;
   moving_trace_ = nullptr;
   moving_ = false;
}

void PathFinderSrc::InitializeOpenSet(std::vector<PathFinderNode>& open)
{
   csg::Point3 start;
   if (use_source_override_) {
      start = source_override_;
   } else {
      auto entity = entity_.lock();
      if (entity) {
         auto mob = entity->GetComponent<om::Mob>();
         if (!mob) {
            PF_LOG(0) << "source entity has no om::Mob component in pathfinder!";
            return;
         }
         start = mob->GetWorldGridLocation();
      }
   }
   open.push_back(start);
   PF_LOG(5) << "initialized open set with entity location " << start;
}

bool PathFinderSrc::IsIdle() const
{
   return moving_;
}

void PathFinderSrc::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}

csg::Point3 PathFinderSrc::GetSourceLocation() const
{
   return source_location_;
}

void PathFinderSrc::Start()
{
   if (use_source_override_) {
      source_location_ = source_override_;
   } else {
      auto entity = entity_.lock();
      if (entity) {
         auto mob = entity->GetComponent<om::Mob>();
         if (mob) {
            source_location_ = mob->GetWorldGridLocation();
         }
      }
   }
}
