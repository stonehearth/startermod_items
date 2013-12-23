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

PathFinderSrc::PathFinderSrc(PathFinder &pf, om::EntityRef e) :
   pf_(pf),
   entity_(e),
   moving_(false),
   collision_cb_id_(0)
{

   ASSERT(!e.expired());
   auto entity = e.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         transform_trace_ = mob->TraceTransform("pf src", dm::PATHFINDER_TRACES)
                                    ->OnChanged([this](csg::Transform const&) {
                                       pf_.RestartSearch("source moved");
                                    });

         moving_trace_ = mob->TraceMoving("pf src", dm::PATHFINDER_TRACES)
                                    ->OnChanged([this](bool const& moving) {
                                       moving_ = moving;
                                       if (moving_) {
                                          pf_.RestartSearch("source moving changed");
                                       }
                                    });
      }
   }
}

PathFinderSrc::~PathFinderSrc()
{
}

void PathFinderSrc::InitializeOpenSet(std::vector<csg::Point3>& open)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         open.push_back(mob->GetWorldGridLocation());
      }
   }
}

bool PathFinderSrc::IsIdle() const
{
   return moving_;
}

void PathFinderSrc::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}
