#include "pch.h"
#include "task.h"
#include <sstream>

using namespace ::radiant;
using namespace ::radiant::simulation;

TaskId Task::nextTaskId_ = 1;

Task::Task(std::string const& name)  :
   id_(nextTaskId_++)
{
   std::ostringstream s;
   s << name << " (" << id_ << ")";
   name_ = s.str();
}

std::string const& Task::GetName() const
{
   return name_;
}
