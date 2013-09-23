#include "pch.h"
#include <sstream>
#include "function.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

std::string Function::desc() const
{
   std::ostringstream os;
   os << "[function " << call_id << " " << route << "]";
   return os.str();
}

