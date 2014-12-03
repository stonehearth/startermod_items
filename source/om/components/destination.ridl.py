from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.csg_types as csg
import ridl.std_types as std

class Destination(Component):

   region = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')
   reserved = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')
   adjacent = dm.Boxed(Region3fBoxedPtr(), set='declare', trace='deep_region')
   auto_update_adjacent = dm.Boxed(c.bool(), set='declare')
   allow_diagonal_adjacency = dm.Boxed(c.bool(), set='declare')
   get_point_of_interest = ridl.Method(c.bool(), ('from', csg.Point3f().const.ref), ('poi', csg.Point3f().ref), no_lua_impl = True).const
   _includes = [
      "om/region.h"
   ]
   _generate_construct_object = True


   _lua_impl = """

// returns nil if the entity is not in the world
luabind::object Destination_GetPointOfInterest(lua_State *L, std::weak_ptr<Destination> o, csg::Point3f const& from)
{
   auto instance = o.lock();
   if (instance) {
      csg::Point3f poi;
      bool is_valid = instance->GetPointOfInterest(from, poi);
      if (is_valid) {
         return luabind::object(L, poi);
      } else {
         return luabind::object();
      }
   }
   throw std::invalid_argument("invalid reference in destination::get_point_of_interest");
}
   """

   _private = \
   """
   void OnAutoUpdateAdjacentChanged();
   void Initialize() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3f const& r);
   csg::Point3f GetBestPointOfInterest(csg::Region3f const& region, csg::Point3f const& from) const;

private:
   DeepRegion3fGuardPtr      region_trace_;
   DeepRegion3fGuardPtr      reserved_trace_;
   """
