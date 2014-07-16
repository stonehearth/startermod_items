#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "lib/lua/register.h"
#include "dm/map.h"
#include "dm/set_trace.h"
#include "dm/map_trace.h"
#include "dm/boxed_trace.h"
#include "dm/store_save_state.h"
#include "map_iterator_wrapper.h"
#include "dm/tracer_buffered.h"
#include "set_iterator.h"
#include "map_trace_wrapper.h"
#include "set_trace_wrapper.h"
#include "boxed_trace_wrapper.h"
#include "trace_wrapper.h"
#include "dm/lua_types.h"
#include "open.h"

using namespace luabind;
using namespace radiant;
using namespace radiant::dm;

DEFINE_INVALID_JSON_CONVERSION(StoreSaveState);

IMPLEMENT_TRIVIAL_TOSTRING(NumberMap)
DEFINE_INVALID_JSON_CONVERSION(NumberMapPtr)

template <typename M>
void LuaMap_Each(lua_State* L, std::shared_ptr<M> map)
{
   if (map) {
      typedef lua::MapIteratorWrapper<typename M::MapType> Iterator;
      lua_pushcfunction(L, Iterator::NextIteration);   // f
      object(L, new Iterator(map, *map)).push(L);      // s
      object(L, 1).push(L);                            // var (ignored)
   }
}

template <typename M>
void LuaMap_Add(M& map, typename M::Key key, luabind::object value)
{
   map.Add(key, value);
}

template <typename M>
void LuaMap_Remove(M& map, typename M::Key key)
{
   map.Remove(key);
}

template <typename M>
luabind::object LuaMap_Get(M& map, typename M::Key key)
{
   return map.Get(key, luabind::object());
}

template <typename M>
bool LuaMap_Contains(M& map, typename M::Key key)
{
   return map.Contains(key);
}


void RegisterGenericTrace(lua_State* L)
{
   typedef lua::TraceWrapper Trace;

   module(L) [
      namespace_("_radiant") [
         namespace_("dm") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetTypeName<Trace>())
               .def("on_changed",         &Trace::OnChanged)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
         ]
      ]
   ];
}

template <typename T>
void RegisterMap(lua_State* L)
{
   typedef lua::MapIteratorWrapper<T> Iterator;
   typedef lua::MapTraceWrapper<MapTrace<T>> Trace;
   module(L) [
      namespace_("_radiant") [
         namespace_("dm") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetTypeName<Trace>())
               .def("on_added",           &Trace::OnAdded)
               .def("on_removed",         &Trace::OnRemoved)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
            ,
            luabind::class_<Iterator, std::shared_ptr<Iterator>>(GetTypeName<Iterator>())
         ]
      ]
   ];
}

template <typename T>
void RegisterBoxed(lua_State* L)
{
   typedef lua::BoxedTraceWrapper<BoxedTrace<T>> Trace;
   module(L) [
      namespace_("_radiant") [
         namespace_("dm") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetTypeName<Trace>())
               .def("on_changed",         &Trace::OnChanged)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
         ]
      ]
   ];
}

template <typename T>
void RegisterSet(lua_State* L)
{
   typedef lua::SetIterator<T> Iterator;
   typedef lua::SetTraceWrapper<SetTrace<T>> Trace;
   module(L) [
      namespace_("_radiant") [
         namespace_("dm") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetTypeName<Trace>())
               .def("on_added",           &Trace::OnAdded)
               .def("on_removed",         &Trace::OnRemoved)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
            ,
            luabind::class_<Iterator, std::shared_ptr<Iterator>>(GetTypeName<Iterator>())
         ]
      ]
   ];
}

static void OpenDm(lua_State* L)
{
#undef CREATE_SET
#undef CREATE_MAP
#undef CREATE_BOXED
#define CREATE_SET(S)      RegisterSet<S>(L);
#define CREATE_MAP(M)      RegisterMap<M>(L);
#define CREATE_BOXED(B)    RegisterBoxed<B>(L);
   ALL_DM_TYPES

   RegisterGenericTrace(L);
   module(L) [
      namespace_("_radiant") [
         namespace_("dm") [
            lua::RegisterTypePtr<NumberMap>("NumberMap")
               .def("add",       &LuaMap_Add<NumberMap>)
               .def("remove",    &LuaMap_Remove<NumberMap>)
               .def("get",       &LuaMap_Get<NumberMap>)
               .def("contains",  &LuaMap_Contains<NumberMap>)
               .def("each",      &LuaMap_Each<NumberMap>)
               .def("get_size",  &NumberMap::GetSize)
               ,
            class_<TraceCategories>("TraceCategories")
               .enum_("constants") [
                  value("SYNC_TRACE", dm::LUA_SYNC_TRACES),
                  value("ASYNC_TRACE", dm::LUA_ASYNC_TRACES)
               ]
            ,
            lua::RegisterTypePtr_NoTypeInfo<StoreSaveState>("SaveState")
         ]
      ]
   ];
}

void lua::dm::open(lua_State* L)
{
   OpenDm(L);
}
