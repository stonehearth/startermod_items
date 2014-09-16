#include "../pch.h"
#include "open.h"
#include "csg/util.h"
#include "om/entity.h"
#include "physics/octtree.h"
#include "physics/nav_grid.h"
#include "physics/physics_util.h"

using namespace ::radiant;
using namespace ::radiant::phys;
using namespace luabind;

bool Physics_IsStandable(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsStandable(entity, csg::ToClosestInt(location));
}

bool Physics_IsStandablePoint(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsStandable(csg::ToClosestInt(location));
}

bool Physics_IsBlocked(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsBlocked(entity, csg::ToClosestInt(location));
}

bool Physics_IsBlockedPoint(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsBlocked(csg::ToClosestInt(location));
}


bool Physics_IsSupported(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsSupported(csg::ToClosestInt(location));
}


bool Physics_IsTerrain(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsTerrain(csg::ToClosestInt(location));
}

<<<<<<< HEAD
bool Physics_IsOccupied(lua_State *L, OctTree &octTree, csg::Point3f const& location)
=======
bool Physics_IsOccupiedPoint(lua_State *L, OctTree &octTree, csg::Point3 const& location)
>>>>>>> ca14fdc5827f8f55cdeb7a34035d5765223ad08a
{
   return octTree.GetNavGrid().IsOccupied(csg::ToClosestInt(location));
}

<<<<<<< HEAD
csg::Point3f Physics_GetStandablePoint(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
=======
bool Physics_IsOccupied(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3 const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsOccupied(entity, location);
}

csg::Point3 Physics_GetStandablePoint(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3 const& location)
>>>>>>> ca14fdc5827f8f55cdeb7a34035d5765223ad08a
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      throw std::logic_error("invalid entity reference in get_standable_point");
   }
   csg::Point3 standable = octTree.GetNavGrid().GetStandablePoint(entity, csg::ToClosestInt(location));
   return csg::ToFloat(standable);
}

template <typename Cube>
luabind::object Physics_GetEntitiesInCube(lua_State *L, OctTree &octTree, Cube const& cube)
{
   NavGrid& navGrid = octTree.GetNavGrid();

   luabind::object result = luabind::newtable(L);
   navGrid.ForEachEntityInBox(csg::ToFloat(cube), [L, &result](om::EntityPtr entity) {
      ASSERT(entity);
      result[entity->GetObjectId()] = luabind::object(L, om::EntityRef(entity));
      return false; // keep iterating...
   });
   return result;
}

template <typename Region>
luabind::object Physics_GetEntitiesInRegion(lua_State *L, OctTree &octTree, Region const& region)
{
   NavGrid& navGrid = octTree.GetNavGrid();

   luabind::object result = luabind::newtable(L);
   navGrid.ForEachEntityInShape(csg::ToFloat(region), [L, &result](om::EntityPtr entity) {
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
               .def("is_blocked",           &Physics_IsBlocked)
               .def("is_blocked",           &Physics_IsBlockedPoint)
               .def("is_supported",         &Physics_IsSupported)
               .def("is_terrain",           &Physics_IsTerrain)
               .def("is_occupied",          &Physics_IsOccupied)
               .def("is_occupied",          &Physics_IsOccupiedPoint)
               .def("get_standable_point",  &Physics_GetStandablePoint)
<<<<<<< HEAD
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube<csg::Cube3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Point3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Cube3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Region3f>),
            def("world_to_local",              &Physics_WorldToLocal<csg::Point3f>)
=======
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube<csg::Cube3>)
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube<csg::Cube3f>)
               .def("get_entities_in_region", &Physics_GetEntitiesInRegion<csg::Region3>)
               .def("get_entities_in_region", &Physics_GetEntitiesInRegion<csg::Region3f>)
            ,
            def("local_to_world",              &Physics_LocalToWorld<csg::Point3>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Cube3>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Region3>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Point3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Cube3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Region3f>),
            def("world_to_local",              &Physics_WorldToLocal<csg::Point3>)
>>>>>>> ca14fdc5827f8f55cdeb7a34035d5765223ad08a
         ]
      ]
   ];
   globals(L)["_physics"] = object(L, &octtree);
}
