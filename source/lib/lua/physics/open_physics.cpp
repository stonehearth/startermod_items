#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "physics/octtree.h"
#include "physics/nav_grid.h"

using namespace ::radiant;
using namespace ::radiant::phys;
using namespace luabind;

bool Physics_CanStandOn(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3 const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }

   bool standable = octTree.CanStandOn(entity, location);
   return standable;
}

luabind::object Physics_GetEntitiesInCube(lua_State *L, OctTree &octTree, csg::Cube3 const& cube)
{
   NavGrid& navGrid = octTree.GetNavGrid();

   luabind::object result = luabind::newtable(L);
   navGrid.ForEachEntityInBounds(cube, [L, &result](om::EntityPtr entity) {
      ASSERT(entity);
      result[entity->GetObjectId()] = luabind::object(L, om::EntityRef(entity));
   });
   return result;
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
               .def("can_stand_on",         &Physics_CanStandOn)
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube)
         ]
      ]
   ];
   globals(L)["_physics"] = object(L, &octtree);
}
