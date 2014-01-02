#ifndef _RADIANT_SIMULATION_JOBS_ENTITY_JOB_SCHEDULER_H
#define _RADIANT_SIMULATION_JOBS_ENTITY_JOB_SCHEDULER_H

#include "om/om.h"
#include "job.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Simulation;

/*
 * class EntityJobScheduler --
 *
 * The EntityJobScheduler contains all the jobs for the Entity.  Instead of schedulding
 * those jobs individually with the simulation, they should be registed with the
 * EntityJobScheduler, who itself will dispatch the Work call to the most appropriate
 * jobs.  The goal is to avoid soaking the CPU running all jobs in parallel.  Instead,
 * give the Job that's closest to unblocking the Entity from reacting to world events
 * a majority of the slice.  This includes, for example, the pathfinder that's closest
 * to reaching its destination.
 */

class EntityJobScheduler : public Job {
public:
   EntityJobScheduler(Simulation& sim, om::EntityPtr entity);
   virtual ~EntityJobScheduler();

public:
   void RegisterPathfinder(PathFinderPtr pathfinder);

public: // Job Interface
   bool IsIdle() const override;
   bool IsFinished() const override;
   void Work(const platform::timer &timer) override;
   std::string GetProgress() const override;
   void EncodeDebugShapes(protocol::shapelist *msg) const override;

private:
   PathFinderPtr GetClosestPathfinder() const;

private:
   struct PathFinderJob {
      PathFinderRef     pathfinder;

      PathFinderJob() { }
      PathFinderJob(PathFinderPtr p) : pathfinder(p) { }
   };

private:
   om::EntityRef                       entity_;
   mutable std::vector<PathFinderJob>  pathfinders_;
   mutable PathFinderRef               closest_pathfinder_;
};

typedef std::weak_ptr<EntityJobScheduler> EntityJobSchedulerRef;
typedef std::shared_ptr<EntityJobScheduler> EntityJobSchedulerPtr;

std::ostream& operator<<(std::ostream& o, const EntityJobScheduler& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_ENTITY_JOB_SCHEDULER_H

