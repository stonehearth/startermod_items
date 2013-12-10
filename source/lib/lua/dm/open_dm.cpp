#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "lib/lua/register.h"
#include "dm/set_trace.h"
#include "dm/map_trace.h"
#include "dm/boxed_trace.h"
#include "map_iterator.h"
#include "set_iterator.h"
#include "map_trace_wrapper.h"
#include "set_trace_wrapper.h"
#include "boxed_trace_wrapper.h"
#include "trace_wrapper.h"
#include "open.h"

using namespace luabind;
using namespace radiant;
using namespace radiant::dm;


void RegisterGenericTrace(lua_State* L)
{
   typedef lua::TraceWrapper Trace;

   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
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
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
               .def("on_added",           &Trace::OnAdded)
               .def("on_removed",         &Trace::OnRemoved)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
            ,
            luabind::class_<Iterator, std::shared_ptr<Iterator>>(GetShortTypeName<Iterator>())
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
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
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
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
               .def("on_added",           &Trace::OnAdded)
               .def("on_removed",         &Trace::OnRemoved)
               .def("on_destroyed",       &Trace::OnDestroyed)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
            ,
            luabind::class_<Iterator, std::shared_ptr<Iterator>>(GetShortTypeName<Iterator>())
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
}

void lua::dm::open(lua_State* L)
{
   OpenDm(L);
}
