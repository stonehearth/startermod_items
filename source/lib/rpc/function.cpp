#include "pch.h"
#include <sstream>
#include "function.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

std::string Function::desc() const
{
   std::ostringstream os;
   if (object.empty()) {
      os << "[function fn:" << call_id << " route:" << route << "]";
   } else {
      os << "[method obj:" << object << " fn:" << call_id << " route:" << route << "]";
   }
   return os.str();
}

