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

      JobId GetId() const;
      virtual bool IsFinished() const = 0;
      virtual bool IsIdle() const = 0;
      virtual void Work(const platform::timer &timer) = 0;
      virtual void LogProgress(std::ostream&) const;
      virtual void EncodeDebugShapes(radiant::protocol::shapelist *msg) const;

   private:
      static JobId                     _nextJobId;

      JobId                            _id;
      std::string                           _name;
};

class Task {
   public:
      Task(std::string const& name);

      std::string const& GetName() const;
      virtual bool Work(const platform::timer &timer) = 0;

   private:
      std::string    name_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOB_H
