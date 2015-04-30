#include "radiant.h"
#include "lua_om.h"
#include "lua_component.h"
#include "lua_entity.h"
#include "lua_region.h"
#include "lua_data_store.h"
#include "lua_std_map_iterator_wrapper.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "lib/lua/script_host.h"
#include "lib/lua/register.h"
#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"
#include "dm/store.h"
#include "om/all_object_defs.h"
#include "om/all_component_defs.h"
#include "om/tiled_region.h"
#include "om/components/terrain_ring_tesselator.h"
#include "csg/util.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "lib/lua/dm/trace_wrapper.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

DEFINE_INVALID_JSON_CONVERSION(Region2BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region3BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region2fBoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region3fBoxedPtrBoxed)

DEFINE_INVALID_JSON_CONVERSION(DeepRegion2Guard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion3Guard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion2fGuard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion3fGuard)

IMPLEMENT_TRIVIAL_TOSTRING(Region2Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region3Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region2fBoxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region3fBoxed)

IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion2Guard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion3Guard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion2fGuard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion3fGuard)

template <typename BoxedType>
static void ModifyBoxed(BoxedType& boxed, luabind::object unsafe_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_cb.interpreter());
   luabind::object cb(cb_thread, unsafe_cb);

   boxed.Modify([cb](typename BoxedType::Value& value) mutable {
      try {
         cb(&value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "error modifying boxed object: " << e.what();
      }
   });
}

template <typename T>
static void TiledRegion_AddPoint(std::shared_ptr<T> tiled_region, csg::Point3f const& point)
{
   tiled_region->AddPoint(point);
}

template <typename T>
static csg::Point3f TiledRegion_GetTileSize(std::shared_ptr<T> tiled_region)
{
   return csg::ToFloat(tiled_region->GetTileSize());
}

template <typename T>
static typename T::TileType TiledRegion_GetTile(std::shared_ptr<T> tiled_region, csg::Point3f const& index)
{
   return tiled_region->GetTile(csg::ToInt(index));
}

template <typename T>
static typename T::TileType TiledRegion_FindTile(std::shared_ptr<T> tiled_region, csg::Point3f const& index)
{
   return tiled_region->FindTile(csg::ToInt(index));
}

template <typename T>
static void TiledRegion_ClearTile(std::shared_ptr<T> tiled_region, csg::Point3f const& index)
{
   tiled_region->ClearTile(csg::ToInt(index));
}

template <typename TiledRegion>
static scope RegisterTiledRegion(const char* name)
{
   return lua::RegisterTypePtr_NoTypeInfo<TiledRegion>(name)
      .def("is_empty",               &TiledRegion::IsEmpty)
      .def("clear",                  &TiledRegion::Clear)
      .def("add_point",              &TiledRegion::AddPoint)
      .def("add_point",              &TiledRegion_AddPoint<TiledRegion>)
      .def("add_cube",               &TiledRegion::AddCube)
      .def("add_region",             &TiledRegion::AddRegion)
      .def("subtract_point",         &TiledRegion::SubtractPoint)
      .def("subtract_cube",          &TiledRegion::SubtractCube)
      .def("subtract_region",        &TiledRegion::SubtractRegion)
      .def("intersect_point",        &TiledRegion::IntersectPoint)
      .def("intersect_cube",         &TiledRegion::IntersectCube)
      .def("intersect_region",       &TiledRegion::IntersectRegion)
      .def("contains",               &TiledRegion::Contains)
      .def("intersects_cube",        &TiledRegion::IntersectsCube)
      .def("intersects_region",      &TiledRegion::IntersectsRegion)
      .def("get_tile_size",          &TiledRegion_GetTileSize<TiledRegion>)
      .def("get_tile",               &TiledRegion_GetTile<TiledRegion>)
      .def("find_tile",              &TiledRegion_FindTile<TiledRegion>)
      .def("clear_tile",             &TiledRegion_ClearTile<TiledRegion>)
      .def("num_tiles",              &TiledRegion::NumTiles)
      .def("num_cubes",              &TiledRegion::NumCubes)
      .def("each_changed_index",     &TiledRegion::GetChangedSet, return_stl_iterator)
      .def("clear_changed_set",      &TiledRegion::ClearChangedSet)
      .def("optimize_changed_tiles", &TiledRegion::OptimizeChangedTiles);
}

static scope RegisterTerrainRingTesselator()
{
   return lua::RegisterTypePtr_NoTypeInfo<TerrainRingTesselator>("TerrainRingTesselator")
      .def("tesselate", (csg::Region3f (TerrainRingTesselator::*)(csg::Region3f const&, csg::Rect2f const*))&TerrainRingTesselator::Tesselate);
}

#define OM_OBJECT(Cls, cls) scope Register ## Cls(lua_State* L);
OM_ALL_COMPONENTS
#undef OM_OBJECT
scope RegisterEffect(lua_State* L);
scope RegisterSensor(lua_State* L);
scope RegisterModelLayer(lua_State* L);

void radiant::om::RegisterLuaTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            LuaComponent_::RegisterLuaTypes(L),
#define OM_OBJECT(Cls, cls) Register ## Cls(L),
            OM_ALL_COMPONENTS
#undef OM_OBJECT
            RegisterSensor(L),
            RegisterEffect(L),
            RegisterModelLayer(L),
            LuaEntity::RegisterLuaTypes(L),
            LuaDataStore::RegisterLuaTypes(L),
            lua::RegisterStrongGameObject<Region2Boxed>(L, "Region2iBoxed")
               .def("get",                 &Region2Boxed::Get)
               .def("modify",              &ModifyBoxed<Region2Boxed>)
            ,
            lua::RegisterStrongGameObject<Region2fBoxed>(L, "Region2Boxed")
               .def("get",                 &Region2fBoxed::Get)
               .def("modify",              &ModifyBoxed<Region2fBoxed>)
            ,
            lua::RegisterStrongGameObject<Region3Boxed>(L, "Region3iBoxed")
               .def("get",                 &Region3Boxed::Get)
               .def("modify",              &ModifyBoxed<Region3Boxed>)
            ,
            lua::RegisterStrongGameObject<Region3fBoxed>(L, "Region3Boxed")
               .def("get",                 &Region3fBoxed::Get)
               .def("modify",              &ModifyBoxed<Region3fBoxed>)
            ,
            luabind::class_<LuaDeepRegion2Guard, LuaDeepRegion2GuardPtr>("LuaDeepRegion2iGuard")
               .def("on_changed",         &LuaDeepRegion2Guard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion2Guard::PushObjectState)
               .def("destroy",            &LuaDeepRegion2Guard::Destroy)
            ,
            luabind::class_<LuaDeepRegion2fGuard, LuaDeepRegion2fGuardPtr>("LuaDeepRegion2Guard")
               .def("on_changed",         &LuaDeepRegion2fGuard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion2fGuard::PushObjectState)
               .def("destroy",            &LuaDeepRegion2fGuard::Destroy)
            ,
            luabind::class_<LuaDeepRegion3Guard, LuaDeepRegion3GuardPtr>("LuaDeepRegion3Guard")
               .def("on_changed",         &LuaDeepRegion3Guard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion3Guard::PushObjectState)
               .def("destroy",            &LuaDeepRegion3Guard::Destroy)
            ,
            luabind::class_<LuaDeepRegion3fGuard, LuaDeepRegion3fGuardPtr>("LuaDeepRegion3fGuard")
               .def("on_changed",         &LuaDeepRegion3fGuard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion3fGuard::PushObjectState)
               .def("destroy",            &LuaDeepRegion3fGuard::Destroy)
            ,
            RegisterTiledRegion<Region3BoxedTiled>("Region3BoxedTiled")
            ,
            RegisterTiledRegion<Region3Tiled>("Region3Tiled")
            ,
            RegisterTerrainRingTesselator()
         ]
      ]
   ];
}
