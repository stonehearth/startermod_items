#include "pch.h"
#include "metrics.h"
#include "path_finder.h"
#include "path_finder_src.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/destination.h"
#include "om/region.h"
#include "csg/color.h"
#include "simulation/trace_categoies.h"

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
         auto transform_trace = mob->GetBoxedTransform().TraceChanges("pf src", PATHFINDER_TRACES);
         transform_trace->OnChanged([this](csg::Transform const&) {
            pf_.RestartSearch();
         });
         traces_.push_back(transform_trace);

         auto moving_trace = mob->GetBoxedMoving().TraceChanges( "pf src", PATHFINDER_TRACES);
         moving_trace->OnChanged([this](bool const& moving) {
            moving_ = moving;
            if (moving_) {
               pf_.RestartSearch();
            }
         });
         traces_.push_back(transform_trace);
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
