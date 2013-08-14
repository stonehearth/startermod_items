#include "pch.h"
#include "lua/register.h"
#include "om/data_blob.h"
#include "lua_data_blob.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaDataBlob::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterObject<DataBlob>()
         .def("mark_changed",   &DataBlob::MarkChanged)
      ;
}
