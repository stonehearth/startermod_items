#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "lib/lua/register.h"
#include "dm/set_trace.h"
#include "dm/map_trace.h"
#include "dm/boxed_trace.h"
#include "dm/store_save_state.h"
#include "map_iterator.h"
#include "dm/tracer_buffered.h"
#include "set_iterator.h"
#include "map_trace_wrapper.h"
#include "set_trace_wrapper.h"
#include "boxed_trace_wrapper.h"
#include "trace_wrapper.h"
#include "open.h"

using namespace luabind;
using namespace radiant;
using namespace radiant::dm;

DEFINE_INVALID_JSON_CONVERSION(StoreSaveState);

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
   typedef lua::MapIterator<T> Iterator;
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
