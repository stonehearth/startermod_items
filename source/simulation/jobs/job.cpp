#include "pch.h"
#include "job.h"
#include <sstream>

using namespace ::radiant;
using namespace ::radiant::simulation;

JobId Job::_nextJobId = 1;

Job::Job(Simulation& sim, std::string name)  :
   _sim(sim),
   _id(_nextJobId++)
{
   std::ostringstream s;
   s << name << " (" << _id << ")";
   _name = s.str();
}

JobId Job::GetId() const
{
   return _id;
}

std::string Job::GetName() const
{
   return _name;
}

Simulation& Job::GetSim() const
{
   return _sim;
}

std::string Job::GetProgress() const
{
   return "";
}

void Job::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}
