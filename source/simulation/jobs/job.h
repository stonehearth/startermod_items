#ifndef _RADIANT_SIMULATION_JOB_H
#define _RADIANT_SIMULATION_JOB_H

#include "simulation/namespace.h"
#include "platform/utils.h"
#include "radiant.pb.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

typedef unsigned int JobId;

class Job {
   public:
      Job(std::string name);

      std::string GetName() const;

      virtual bool IsFinished() const = 0;
      virtual bool IsIdle(int now) const = 0;
      virtual void Work(int now, const platform::timer &timer) = 0;
      virtual void LogProgress(std::ostream&) const;
      virtual void EncodeDebugShapes(radiant::protocol::shapelist *msg) const;

   private:
      static JobId                     _nextJobId;

      JobId                            _id;
      std::string                           _name;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOB_H
