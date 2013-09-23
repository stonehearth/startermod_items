#include "pch.h"
#include "register.h"
#include "radiant_luabind.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

template <typename T>
std::string PointToJson(T const& pt, luabind::object state)
{
   std::stringstream os;
   os << "{ \"x\" :" << pt.x << ", \"y\" :" << pt.y << ", \"z\" :" << pt.z << "}";
   return os.str();
}
