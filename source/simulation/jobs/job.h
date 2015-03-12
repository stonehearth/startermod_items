#ifndef _RADIANT_SIMULATION_JOB_H
#define _RADIANT_SIMULATION_JOB_H

#include "simulation/namespace.h"
#include "platform/utils.h"
#include "protocols/radiant.pb.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

typedef unsigned int JobId;

// Call back into the Simulation to measure how much time this job is taking
#define MEASURE_JOB_TIME() \
   std::unique_ptr<perfmon::TimelineCounterGuard> __tg; \
   if (Simulation::GetInstance().GetEnableJobLogging()) { \
      __tg.reset(new perfmon::TimelineCounterGuard(Simulation::GetInstance().GetJobsPerfTimeline(), core::StaticString(GetName()))); \
   }

class Job {
   public:
      Job(std::string const& name);

      std::string const& GetName() const;

      JobId GetId() const;
      virtual bool IsFinished() const = 0;
      virtual bool IsIdle() const = 0;
      virtual void Work(const platform::timer &timer) = 0;
      virtual std::string GetProgress();
      virtual void EncodeDebugShapes(radiant::protocol::shapelist *msg) const;

   private:
      static JobId                     _nextJobId;
      JobId                            _id;
      std::string                      _name;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOB_H
