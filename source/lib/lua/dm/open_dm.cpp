#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "lib/lua/register.h"
#include "dm/map_trace.h"
#include "dm/boxed_trace.h"
#include "map_trace_wrapper.h"
#include "boxed_trace_wrapper.h"
#include "lib/lua/dm_iterators.h"
#include "open.h"

using namespace luabind;
using namespace radiant;
using namespace radiant::dm;

template <typename T>
void RegisterMapType(lua_State* L)
{
   typedef lua::MapTraceWrapper<MapTrace<T>> Trace;
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
               .def("on_changed",         &Trace::OnChanged)
               .def("on_removed",         &Trace::OnRemoved)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
         ]
      ]
   ];
}

template <typename T>
void RegisterBoxedType(lua_State* L)
{
   typedef lua::BoxedTraceWrapper<BoxedTrace<T>> Trace;
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            luabind::class_<Trace, std::shared_ptr<Trace>>(GetShortTypeName<Trace>())
               .def("on_changed",         &Trace::OnChanged)
               .def("push_object_state",  &Trace::PushObjectState)
               .def("destroy",            &Trace::Destroy)
         ]
      ]
   ];
}

static void OpenDm(lua_State* L)
{
#undef CREATE_MAP
#undef CREATE_BOXED
#define CREATE_MAP(M)      RegisterMapType<M>(L);
#define CREATE_BOXED(B)    RegisterBoxedType<B>(L);
   ALL_DM_TYPES
}

void lua::dm::open(lua_State* L)
{
   OpenDm(L);
}
