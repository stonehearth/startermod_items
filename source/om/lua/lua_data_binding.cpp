#include "pch.h"
#include "lua/register.h"
#include "om/data_binding.h"
#include "lua_data_binding.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

template <typename T>
dm::Object::LuaPromise<DataBinding>* DataBinding_Trace(T const& db, const char* reason)
{
   return new dm::Object::LuaPromise<DataBinding>(reason, db);
}

scope LuaDataBinding::RegisterLuaTypes(lua_State* L)
{
   return
      // shared pointers to DataBinding's are used where lua expliclity creates data bindings
      lua::RegisterWeakGameObject<DataBinding>()
         .def("update",         &DataBinding::SetDataObject)
         .def("get_data",       &DataBinding::GetDataObject)
         .def("mark_changed",   &DataBinding::MarkChanged)
         .def("trace",          &DataBinding_Trace<DataBinding>)
      ,
      // references to DataBinding's are used where lua should not be able to keep objects alive, e.g. component data
      lua::RegisterStrongGameObject<DataBindingP>()
         .def("update",         &DataBindingP::SetDataObject)
         .def("get_data",       &DataBindingP::GetDataObject)
         .def("mark_changed",   &DataBindingP::MarkChanged)
         .def("trace",          &DataBinding_Trace<DataBindingP>)
         .def("set_controller", &DataBindingP::SetModelObject)
      ,
      dm::Object::LuaPromise<DataBinding>::RegisterLuaType(L)         
      ;
}
