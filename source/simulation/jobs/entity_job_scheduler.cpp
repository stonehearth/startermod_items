#include "pch.h"
#include "om/entity.h"
#include "entity_job_scheduler.h"
#include "path_finder.h"

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
EntityJobScheduler::EntityJobScheduler(Simulation& sim, om::EntityPtr entity) :
   Job(sim, BUILD_STRING(*entity)),
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
      E_LOG(7) << "pumping pathfinder " << p->GetName() << " : " << p->GetProgress();
      p->Work(timer);
   }
}

/*
 * EntityJobScheduler::GetProgress
 *
 * Return a string summarizing the progress of the job.
 */
std::string EntityJobScheduler::GetProgress() const
{
   PathFinderPtr p = closest_pathfinder_.lock();
   if (p) {
      return p->GetProgress();
   }
   return "no active pathfinder";
}

/*
 * EntityJobScheduler::EncodeDebugShapes
 *
 * Encode some debug information about the job.
 */
void EntityJobScheduler::EncodeDebugShapes(protocol::shapelist *msg) const
{
   uint i, c = pathfinders_.size();
   for (i = 0; i < c; i++) {
      PathFinderPtr p = pathfinders_[i].pathfinder.lock();
      if (p) {
         p->EncodeDebugShapes(msg);
      } else {
         pathfinders_[i] = pathfinders_[--c];
      }
   }
   pathfinders_.resize(c);
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
   uint i, c = pathfinders_.size();
   for (i = 0; i < c; i++) {
      PathFinderPtr p = pathfinders_[i].pathfinder.lock();
      if (p) {
         if (!p->IsIdle()) {
            float distance = p->EstimateCostToSolution();
            if (distance < best_distance) {
               best_distance = distance;
               best = p;
            }
         }
      } else {
         pathfinders_[i] = pathfinders_[--c];
      }
   }
   pathfinders_.resize(c);

   return best;
}

/*
 * EntityJobScheduler::GetClosestPathfinder
 *
 * Associate the PathFinder with its entity so we can schedule it.
 */
void EntityJobScheduler::RegisterPathfinder(PathFinderPtr p)
{
   pathfinders_.emplace_back(PathFinderJob(p));
}
