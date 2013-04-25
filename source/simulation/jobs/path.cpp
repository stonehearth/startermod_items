#include "pch.h"
#include "path.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

std::ostream& Path::Format(std::ostream& os) const
{
   return (os << "[Path " << ((void*)this) << " " << " " << points_.front() << " -> " << points_.back() << "]");
}

std::ostream& simulation::operator<<(std::ostream& os, const Path& in)
{
   return in.Format(os);
} 

