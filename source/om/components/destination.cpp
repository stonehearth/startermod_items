#include "pch.h"
#include "destination.h"
#include "csg/csg_json.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Destination::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
   AddRecordField("reserved", reserved_);
   AddRecordField("adjacent", adjacent_);

   if (!IsRemoteRecord()) {
      region_ = GetStore().AllocObject<Region>();
      reserved_ = GetStore().AllocObject<Region>();
      adjacent_ = GetStore().AllocObject<Region>();
      lastUpdated_ = 0;

      auto update = [=](csg::Region3 const& rgn) { UpdateDerivedValues(); };

      guards_ += (*region_)->Trace("updating destination derived values (region changed)", update);
      guards_ += (*reserved_)->Trace("updating destination derived values (reserved changed)", update);
   }
}

std::string Destination::ToString() const
{
   std::ostringstream os;
   os << "(Destination id:" << GetObjectId() << ")";
   return os.str();
}

/*
 * Looks something like this:
 *
 *     "destination" : {
 *       "region" : [
 *          [[-3, 0, -3], [3, 1, 3]]
 *        ]
 *     }
 */

void Destination::ExtendObject(json::ConstJsonObject const& obj)
{
   if (obj.has("region")) {
      csg::Region3& region = (*region_)->Modify();
      region += obj.get("region", csg::Region3());
   }
}

void Destination::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;

   module(L) [
      class_<Destination, std::weak_ptr<Component>, Component>("Destination")
         .def(tostring(self))
         .def("get_region",   &om::Destination::GetRegion)
         .def("set_region",   &om::Destination::SetRegion)
   ];
}

void Destination::SetRegion(std::shared_ptr<csg::Region3> region)
{
   (*region_)->Modify() = *region;
}

void Destination::UpdateDerivedValues()
{
   // This last updated business is a deficency in the change tracker design.  if
   // you need 2 fragments of data to compute a derived value and both of them
   // change, you get notified twice about them.  What we need is to set a dirty
   // flag in the callack, then schedule a one-time "before we exit this gameloop,
   // please let me attend to some business" flag.  Then do the hard computation there.

   if (lastUpdated_ < GetStore().GetCurrentGenerationId()) {
      csg::Region3 const& region = ***region_;
      csg::Region3 const& reserved = ***reserved_;
      if (reserved.IsEmpty()) {
         ComputeAdjacentRegion(region);
      } else {
         ComputeAdjacentRegion(region - reserved);
      }
      lastUpdated_ = GetStore().GetCurrentGenerationId();
   }
}

void Destination::ComputeAdjacentRegion(csg::Region3 const& r)
{
   csg::Region3& adjacent = (*adjacent_)->Modify();

   adjacent.Clear();
   for (const csg::Cube3& c : r) {
      csg::Point3 p0 = c.GetMin();
      csg::Point3 p1 = c.GetMax();
      csg::Point3 delta = p1 - p0;
      int x = delta.x, y = delta.y, z = delta.z;

      adjacent.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(x, y, 0)));  // top
      adjacent.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, y, z)));  // left
      adjacent.Add(csg::Cube3(p0 + csg::Point3(0, 0, z), p0 + csg::Point3(x, y, z + 1)));  // bottom
      adjacent.Add(csg::Cube3(p0 + csg::Point3(x, 0, 0), p0 + csg::Point3(x + 1, y, z)));  // right
   }
   adjacent.Optimize();
}
