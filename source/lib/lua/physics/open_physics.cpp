#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "physics/octtree.h"
#include "physics/nav_grid.h"
#include "physics/physics_util.h"

using namespace ::radiant;
using namespace ::radiant::phys;
using namespace luabind;

bool Physics_IsStandable(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3 const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsStandable(entity, location);
}

bool Physics_IsStandablePoint(lua_State *L, OctTree &octTree, csg::Point3 const& location)
{
   return octTree.GetNavGrid().IsStandable(location);
}

csg::Point3 Physics_GetStandablePoint(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3 const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      throw std::logic_error("invalid entity reference in get_standable_point");
   }
   return octTree.GetNavGrid().GetStandablePoint(entity, location);
}

luabind::object Physics_GetEntitiesInCube(lua_State *L, OctTree &octTree, csg::Cube3 const& cube)
{
   NavGrid& navGrid = octTree.GetNavGrid();

   luabind::object result = luabind::newtable(L);
   navGrid.ForEachEntityInBounds(cube, [L, &result](om::EntityPtr entity) {
      ASSERT(entity);
      result[entity->GetObjectId()] = luabind::object(L, om::EntityRef(entity));
      return false; // keep iterating...
   });
   return result;
}

template <typename Shape>
Shape Physics_LocalToWorld(Shape const& s, om::EntityRef e)
{
   om::EntityPtr entity = e.lock();
   if (entity) {
      return phys::LocalToWorld(s, entity);
   }
   return s;
}

template <typename Shape>
Shape Physics_WorldToLocal(Shape const& s, om::EntityRef e)
{
   om::EntityPtr entity = e.lock();
   if (entity) {
      return phys::WorldToLocal(s, entity);
   }
   return s;
}

DEFINE_INVALID_JSON_CONVERSION(OctTree)
DEFINE_INVALID_LUA_CONVERSION(OctTree)
IMPLEMENT_TRIVIAL_TOSTRING(OctTree)

void lua::phys::open(lua_State* L, OctTree& octtree)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("physics") [
            luabind::class_<OctTree>("Physics")
               .def("is_standable",         &Physics_IsStandable)
               .def("is_standable",         &Physics_IsStandablePoint)
               .def("get_standable_point",  &Physics_GetStandablePoint)
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube),
            def("local_to_world",              &Physics_LocalToWorld<csg::Point3>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Cube3>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Region3>),
            def("world_to_local",              &Physics_WorldToLocal<csg::Point3>)
         ]
      ]
   ];
   globals(L)["_physics"] = object(L, &octtree);
}
