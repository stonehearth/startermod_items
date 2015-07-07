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

csg::Region3f Physics_ClipRegion(lua_State *L, OctTree &octTree, csg::Region3f const& r, NavGrid::ClippingMode mode)
{
   return octTree.GetNavGrid().ClipRegion(r, mode);
}

csg::Region3f Physics_ProjectRegion(lua_State *L, OctTree &octTree, csg::Region3f const& r, NavGrid::ClippingMode mode)
{
   return octTree.GetNavGrid().ProjectRegion(r, mode);
}

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

bool Physics_IsSupported(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsSupported(entity, csg::ToClosestInt(location));
}

bool Physics_IsSupportedPoint(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsSupported(csg::ToClosestInt(location));
}


bool Physics_IsTerrain(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsTerrain(csg::ToClosestInt(location));
}

bool Physics_IsOccupiedPoint(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().IsOccupied(csg::ToClosestInt(location));
}

bool Physics_IsOccupied(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return false;
   }
   return octTree.GetNavGrid().IsOccupied(entity, csg::ToClosestInt(location));
}

float Physics_GetMovementSpeedAt(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   return octTree.GetNavGrid().GetMovementSpeedAt(csg::ToInt(location));
}

csg::Point3f Physics_GetStandable(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      throw std::logic_error("invalid entity reference in get_standable_point");
   }
   csg::Point3 standable = octTree.GetNavGrid().GetStandablePoint(entity, csg::ToClosestInt(location));
   return csg::ToFloat(standable);
}

csg::Point3f Physics_GetStandablePoint(lua_State *L, OctTree &octTree, csg::Point3f const& location)
{
   csg::Point3 standable = octTree.GetNavGrid().GetStandablePoint(csg::ToClosestInt(location));
   return csg::ToFloat(standable);
}

luabind::object Physics_GetEntitiesInCube(lua_State *L, OctTree &octTree, csg::Cube3f const& cube)
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

luabind::object Physics_GetEntitiesInRegion(lua_State *L, OctTree &octTree, csg::Region3f const& region)
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

luabind::object Physics_GetBlockingEntities(lua_State *L, OctTree &octTree, om::EntityRef entityRef, csg::Point3f const& location)
{
   NavGrid& navGrid = octTree.GetNavGrid();
   luabind::object result = luabind::newtable(L);

   om::EntityPtr entity = entityRef.lock();
   if (!entity) {
      return result;
   }

   navGrid.ForEachBlockingEntity(entity, csg::ToClosestInt(location), [L, &result](om::EntityPtr entity) {
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

std::shared_ptr<core::Guard>
Physics_AddNotifyDirtyTileFn(lua_State *L, OctTree &octTree, luabind::object unsafe_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_cb.interpreter());
   luabind::object cb = luabind::object(cb_thread, unsafe_cb);

   core::Guard g = octTree.GetNavGrid().NotifyTileDirty([cb, cb_thread](csg::Point3 const& pt) mutable {
      try {
         cb(csg::ToFloat(pt));
      }
      catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(cb_thread, e);
      }
   });
   return std::make_shared<core::Guard>(std::move(g));
}

void
Physics_ForEachEntityInTile(lua_State *L, OctTree &octTree, csg::Point3f const& pt, luabind::object unsafe_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_cb.interpreter());
   luabind::object cb = luabind::object(cb_thread, unsafe_cb);

   NavGrid::ForEachEntityCb callback = [cb, cb_thread](om::EntityPtr entity) mutable -> bool {
      bool stop = false;
      try {
         luabind::object result = cb(om::EntityRef(entity));
         if (result.is_valid() && luabind::type(result) == LUA_TBOOLEAN && luabind::object_cast<bool>(result) == false) {
            stop = true;
         }
      }
      catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(cb_thread, e);
      }
      return stop;
   };

   octTree.GetNavGrid().ForEachEntityAtIndex(csg::ToInt(pt), callback);
}

DEFINE_INVALID_JSON_CONVERSION(OctTree)
IMPLEMENT_TRIVIAL_TOSTRING(OctTree)

void lua::phys::open(lua_State* L, OctTree& octtree)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("physics") [

            luabind::class_<OctTree>("Physics")
               .enum_("constants") [
                  value("CLIP_SOLID",       NavGrid::ClippingMode::CLIP_SOLID),
                  value("CLIP_TERRAIN",     NavGrid::ClippingMode::CLIP_TERRAIN),
                  value("CLIP_STANDABLE",   NavGrid::ClippingMode::CLIP_STANDABLE)
               ]
               .def("clip_region",          &Physics_ClipRegion)
               .def("project_region",       &Physics_ProjectRegion)
               .def("is_standable",         &Physics_IsStandable)
               .def("is_standable",         &Physics_IsStandablePoint)
               .def("is_blocked",           &Physics_IsBlocked)
               .def("is_blocked",           &Physics_IsBlockedPoint)
               .def("is_supported",         &Physics_IsSupported)
               .def("is_supported",         &Physics_IsSupportedPoint)
               .def("is_terrain",           &Physics_IsTerrain)
               .def("is_occupied",          &Physics_IsOccupied)
               .def("is_occupied",          &Physics_IsOccupiedPoint)
               .def("get_standable_point",  &Physics_GetStandable)
               .def("get_standable_point",  &Physics_GetStandablePoint)
               .def("get_entities_in_cube", &Physics_GetEntitiesInCube)
               .def("get_entities_in_region",   &Physics_GetEntitiesInRegion)
               .def("get_blocking_entities",    &Physics_GetBlockingEntities)
               .def("get_movement_speed_at",    &Physics_GetMovementSpeedAt)
               .def("add_notify_dirty_tile_fn", &Physics_AddNotifyDirtyTileFn)
               .def("for_each_entity_in_tile",  &Physics_ForEachEntityInTile)
            ,
            def("local_to_world",              &Physics_LocalToWorld<csg::Point3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Cube3f>),
            def("local_to_world",              &Physics_LocalToWorld<csg::Region3f>),
            def("world_to_local",              &Physics_WorldToLocal<csg::Point3f>)
         ]
      ]
   ];
   globals(L)["_physics"] = object(L, &octtree);
}
