#include "pch.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::om;

static bool RegionContains(Region const& r, csg::Point3 const& pt)
{
   return (*r).Contains(pt);
}

void Region::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;

   module(L) [
      class_<Region, std::shared_ptr<Region>, Component>("omRegion3")
         .def(tostring(self))
         .def("modify",    &om::Region::Modify)
         .def("contains",  &RegionContains)
   ];
}