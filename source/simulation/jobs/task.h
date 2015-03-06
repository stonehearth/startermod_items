#ifndef _RADIANT_SIMULATION_TASK_H
#define _RADIANT_SIMULATION_TASK_H

#include "simulation/namespace.h"
#include "platform/utils.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

typedef unsigned int TaskId;

class Task {
   public:
      Task(std::string const& name);

      std::string const& GetName() const;
      virtual bool Work(const platform::timer &timer) = 0;

   private:
      static TaskId  nextTaskId_;
      TaskId         id_;
      std::string    name_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_TASK_H
