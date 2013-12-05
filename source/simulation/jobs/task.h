#ifndef _RADIANT_SIMULATION_TASK_H
#define _RADIANT_SIMULATION_TASK_H

#include "simulation/namespace.h"
#include "platform/utils.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Task {
   public:
      Task(Simulation& sim, std::string const& name) :
         sim_(sim), name_(name) { }

      std::string const& GetName() const { return name_; }
      virtual bool Work(const platform::timer &timer) = 0;

      Simulation& GetSim() const { return sim_; }

   private:
      Simulation&    sim_;
      std::string    name_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_TASK_H
