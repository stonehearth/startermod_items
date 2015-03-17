#include "pch.h"
#include "job.h"
#include <sstream>

using namespace ::radiant;
using namespace ::radiant::simulation;

JobId Job::_nextJobId = 1;

Job::Job(std::string const& name)  :
   _id(_nextJobId++),
   _name(BUILD_STRING("jid:" << _id << " " << name))
{
}

JobId Job::GetId() const
{
   return _id;
}

std::string const& Job::GetName() const
{
   return _name;
}

std::string Job::GetProgress()
{
   return "";
}

void Job::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}
