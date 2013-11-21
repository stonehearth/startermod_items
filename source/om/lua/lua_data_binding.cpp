#include "pch.h"
#include "lib/lua/register.h"
#include "om/components/data_store.ridl.h"
#include "lua_data_binding.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

template <typename T>
dm::Object::LuaPromise<DataStore>* DataStore_Trace(T const& db, const char* reason)
{
   return new dm::Object::LuaPromise<DataStore>(reason, db);
}

scope LuaDataStore::RegisterLuaTypes(lua_State* L)
{
   return
      // references to DataStore's are used where lua should not be able to keep objects alive, e.g. component data
      lua::RegisterStrongGameObject<DataStore>()
         .def("update",         &DataStore::SetDataObject)
         .def("get_data",       &DataStore::GetDataObject)
         .def("mark_changed",   &DataStore::MarkChanged)
         .def("trace",          &DataStore_Trace<DataStore>)
         .def("set_controller", &DataStore::SetModelObject)
      ,
      dm::Object::LuaPromise<DataStore>::RegisterLuaType(L)         
      ;
}
