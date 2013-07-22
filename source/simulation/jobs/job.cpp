#include "pch.h"
#include "job.h"
#include <sstream>

using namespace ::radiant;
using namespace ::radiant::simulation;

JobId Job::_nextJobId = 1;

Job::Job(std::string name)  :
   _id(_nextJobId++)
{
   std::ostringstream s;
   s << name << " (" << _id << ")";
   _name = s.str();
}

std::string Job::GetName() const
{
   return _name;
}

void Job::LogProgress(std::ostream& ) const
{
}

void Job::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}

Task::Task(std::string const& name) :
   name_(name)
{
}

std::string const& Task::GetName() const
{
   return name_;
}

