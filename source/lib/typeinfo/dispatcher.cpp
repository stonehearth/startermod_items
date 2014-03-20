#include <unordered_map>
#include "radiant.h"
#include "dispatcher.h"
#include "dispatch_table.h"

using namespace radiant;
using namespace radiant::typeinfo;

std::unordered_map<TypeId, DispatchTable> _dispatchTable;

Dispatcher::Dispatcher(dm::Store const& store, int flags) :
   store_(store),
   flags_(flags)
{
}

void Dispatcher::Register(TypeId id, DispatchTable const& entry)
{
   ASSERT(_dispatchTable.find(id) == _dispatchTable.end());
   _dispatchTable[id] = entry;
}

bool Dispatcher::LuaToProto(TypeId typeId, luabind::object const& obj, Protocol::Value* msg)
{
   auto i = _dispatchTable.find(typeId);
   if (i != _dispatchTable.end()) {
      DispatchTable const& entry = i->second;
      if (entry.lua_to_protobuf) {
         entry.lua_to_protobuf(store_, obj, msg, flags_);
         return true;
      }
   }
   return false;
}

bool Dispatcher::ProtoToLua(TypeId typeId, Protocol::Value const& msg, luabind::object& obj)
{
   auto i = _dispatchTable.find(typeId);
   if (i != _dispatchTable.end()) {
      DispatchTable const& entry = i->second;
      if (entry.protobuf_to_lua) {
         entry.protobuf_to_lua(store_, msg, obj, flags_);
         return true;
      }
   }
   return false;
}
