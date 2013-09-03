#include "pch.h"
#include "lua/register.h"
#include "om/data_binding.h"
#include "lua_data_binding.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

dm::Object::LuaPromise<DataBinding>* DataBinding_Trace(DataBinding const& db, const char* reason)
{
   return new dm::Object::LuaPromise<DataBinding>(reason, db);
}

scope LuaDataBinding::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterObject<DataBinding>()
         .def("update",         &DataBinding::SetDataObject)
         .def("get_data",       &DataBinding::GetDataObject)
         .def("mark_changed",   &DataBinding::MarkChanged)
         .def("trace",          &DataBinding_Trace)
      ,
      dm::Object::LuaPromise<DataBinding>::RegisterLuaType(L)         
      ;
}
