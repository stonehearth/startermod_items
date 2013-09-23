#include "pch.h"
#include <sstream>
#include "untrace.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

std::string UnTrace::desc() const
{
   std::ostringstream os;
   os << "[untrace " << call_id << "]";
   return os.str();
}

