#include "pch.h"
#include <sstream>
#include "trace.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

std::atomic<int> Request::next_call_id_(1);


std::string Trace::desc() const
{
   std::ostringstream os;
   os << "[trace " << call_id << " " << route << "]";
   return os.str();
}

