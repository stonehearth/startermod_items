#include "pch.h"
#include "om/entity.h"
#include "entity_job_scheduler.h"
#include "path_finder.h"
#include "lib/perfmon/timer.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define E_LOG(level)   LOG(simulation.jobs, level) << "[" << GetName() << "] "

#if defined(CHECK_HEAPINESS)
#  define VERIFY_HEAPINESS() \
   do { \
      if (!rebuildHeap_) { \
         _Debug_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, _1, _2)); \
      } \
   } while (false);
#else
#  define VERIFY_HEAPINESS()
#endif


/*
 * EntityJobScheduler::EntityJobScheduler
 *
 * Construct a new entity job scneduler.
 */
EntityJobScheduler::EntityJobScheduler(om::EntityPtr entity) :
   Job(BUILD_STRING(*entity)),
   _recordPathfinderTimes(false),
   entity_(entity)
{
}

/*
 * EntityJobScheduler::EntityJobScheduler
 *
 * Destroy the job scheduler
 */
EntityJobScheduler::~EntityJobScheduler()
{
}

/*
 * EntityJobScheduler::IsIdle
 *
 * The job scheduler is idle if all of the containing jobs are idle.  Right now we
 * just have pathfinders, so if we couldn't find one that needs work to do, return
 * true.
 */
bool EntityJobScheduler::IsIdle() const
{
   closest_pathfinder_ = GetClosestPathfinder();
   return closest_pathfinder_.expired();
}

/*
 * EntityJobScheduler::IsFinished
 *
 * We're finished when the entity we're assoicated with is destroyed.
 */
bool EntityJobScheduler::IsFinished() const
{ 
   return entity_.expired();
}

/*
 * EntityJobScheduler::Work
 *
 * Delegate work to the job most likely to unblock the Entity.  For now, just pump
 * the pathfinder that's closest to succeeding the hardest.
 */
void EntityJobScheduler::Work(const platform::timer &timer)
{
   PathFinderPtr p = closest_pathfinder_.lock();
   if (p) {
      perfmon::CounterValueType time;
      if (_recordPathfinderTimes) {
         time = perfmon::Timer::GetCurrentCounterValueType();
      }

      E_LOG(7) << "pumping pathfinder " << p->GetName() << " : " << p->GetProgress();
      p->Work(timer);

      if (_recordPathfinderTimes) {
         _pathfinderTimes[p->GetName()] += perfmon::Timer::GetCurrentCounterValueType() - time;
      }
   }
}

/*
 * EntityJobScheduler::GetProgress
 *
 * Return a string summarizing the progress of the job.
 */
std::string EntityJobScheduler::GetProgress()
{
   std::ostringstream s;

   for (auto const& entry : pathfinders_) {
      PathFinderPtr pathfinder = entry.second.lock();
      if (pathfinder) {
         s << pathfinder->GetProgress() << "\n";
      }
   }
   return s.str();
}

/*
 * EntityJobScheduler::EncodeDebugShapes
 *
 * Encode some debug information about the job.
 */
void EntityJobScheduler::EncodeDebugShapes(protocol::shapelist *msg) const
{
   for (auto const& entry : pathfinders_) {
      PathFinderPtr p = entry.second.lock();
      if (p) {
         p->EncodeDebugShapes(msg);
      }
   }
}

/*
 * EntityJobScheduler::GetClosestPathfinder
 *
 * Return a pointer to the pathfinder that's closest to reaching its destination.
 * Also prunes abandoned pathfinders.
 */
PathFinderPtr EntityJobScheduler::GetClosestPathfinder() const
{
   PathFinderPtr best;
   float best_distance = FLT_MAX;

   stdutil::ForEachPrune<int, PathFinder>(pathfinders_, [this, &best, &best_distance] (const int& id, PathFinderPtr p) {
      if (!p->IsIdle()) {
         float distance = p->EstimateCostToSolution();
         if (distance < best_distance) {
            best_distance = distance;
            best = p;
         }
      }
   });
   return best;
}

/*
 * EntityJobScheduler::AddPathfinder
 *
 * Associate the PathFinder with its entity so we can schedule it.
 */
void EntityJobScheduler::AddPathfinder(PathFinderPtr p)
{
   pathfinders_[p->GetId()] = p;
}

void EntityJobScheduler::RemovePathfinder(PathFinderPtr p)
{
   pathfinders_.erase(p->GetId());
   if (closest_pathfinder_.lock() == p) {
      closest_pathfinder_.reset();
   }
}

void EntityJobScheduler::SetRecordPathfinderTimes(bool enabled)
{
   _recordPathfinderTimes = enabled;
}

EntityJobScheduler::PathfinderTimeMap const& EntityJobScheduler::GetPathfinderTimes()
{
   return _pathfinderTimes;
}

void EntityJobScheduler::ResetPathfinderTimes()
{
   _pathfinderTimes.clear();
}

om::EntityRef EntityJobScheduler::GetEntity() const
{
   return entity_;
}

EntityJobScheduler::PathFinderMap const& EntityJobScheduler::GetPathFinders() const
{
   return pathfinders_;
}
