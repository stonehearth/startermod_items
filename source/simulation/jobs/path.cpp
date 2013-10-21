#include "pch.h"
#include "path.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

std::ostream& Path::Format(std::ostream& os) const
{
   os << "[Path " << ((void*)this);
   if (!points_.empty()) {
      os << " " << points_.front() << " -> " << points_.back();
   } else {
      os << "no points on path";
   }
   os << "]";
   return os;
}

std::ostream& simulation::operator<<(std::ostream& os, const Path& in)
{
   return in.Format(os);
} 

