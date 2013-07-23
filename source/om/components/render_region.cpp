#include "pch.h"
#include "render_region.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderRegion::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;

   module(L) [
      class_<RenderRegion, std::weak_ptr<Component>>("RenderRegion")
         .def(tostring(self))
         .def("modify",         &om::RenderRegion::ModifyRegion)
         .def("get",            &om::RenderRegion::GetRegion)
         .def("alloc",          &om::RenderRegion::AllocRegion)
         .def("get_pointer",    &om::RenderRegion::GetRegionPointer)
         .def("set_pointer",    &om::RenderRegion::SetRegionPointer)
   ];
}

std::string RenderRegion::ToString() const
{
   std::ostringstream os;
   os << "(RenderRegion id:" << GetObjectId() << ")";
   return os.str();
}

void RenderRegion::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

csg::Region3& RenderRegion::ModifyRegion()
{
   return (*region_)->Modify();
}

const csg::Region3& RenderRegion::GetRegion() const
{
   return ***region_;
}

RegionPtr RenderRegion::AllocRegion()
{
   region_ = GetStore().AllocObject<Region>();
   return *region_;
}

RegionPtr RenderRegion::GetRegionPointer() const
{
   return *region_;
}

void RenderRegion::SetRegionPointer(RegionPtr r)
{
   region_ = r;
}

dm::Boxed<RegionPtr> const& RenderRegion::GetBoxedRegion() const
{
   return region_;
}
