#include "pch.h"
#include <sstream>
#include "session.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

std::ostream& rpc::operator<<(std::ostream& os, Session const& s)
{
   return (os << "[session faction:" << s.faction << "]");
}
