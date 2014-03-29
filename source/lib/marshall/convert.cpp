#include "radiant.h"
#include "convert.h"
#include "dm/store.h"
#include "protocols/store.pb.h"
#include "lib/lua/bind.h"
#include "lib/typeinfo/dispatcher.h"
#include <boost/algorithm/string/predicate.hpp>

#include "lib/lua/script_host.h"

using namespace radiant;
using namespace radiant::marshall;

Convert::Convert(dm::Store const& store, int flags) :
   store_(store),
   flags_(flags)
{
}

void Convert::ToProtobuf(luabind::object const& from, Protocol::Value* to) {
   std::vector<luabind::object> tables;

   Protocol::LuaObject* msg = to->MutableExtension(Protocol::LuaObject::extension);
   LuaToProtobuf(from, msg, msg, tables);
}

void Convert::ToLua(Protocol::Value const& from, luabind::object &to)
{
   std::unordered_map<uint, luabind::object> tables;
   Protocol::LuaObject const& msg = from.GetExtension(Protocol::LuaObject::extension);
   ProtobufToLua(msg, to, msg, tables);
};

static luabind::object GetObjectSavedVariables(luabind::object const& obj)
{
   luabind::object saved_variables = obj;

   int t = luabind::type(saved_variables);
   while (t == LUA_TTABLE) {
      luabind::object o = saved_variables["__saved_variables"];
      if (!o) {
         break;
      }
      saved_variables = o;
      t = luabind::type(saved_variables);
   }
   return saved_variables;
}

void Convert::UserdataToProtobuf(luabind::object const& obj, Protocol::Value* msg)
{
   lua_State* L = store_.GetInterpreter();
   typeinfo::TypeId typeId = 0;

   try {
      lua::ScriptHost *s = lua::ScriptHost::GetScriptHost(L);
      typeId = luabind::call_function<int>(obj["__get_userdata_type_id"]);
   } catch (std::exception const& e) {
      LOG_(0) << "wtf? " << e.what();
   }
   if (!typeId) {
      luabind::class_info ci = luabind::call_function<luabind::class_info>(luabind::globals(L)["class_info"], obj);
      throw std::logic_error(BUILD_STRING("object type " << ci.name << " has no __userdata_type_id field"));
   }

   if (!typeinfo::Dispatcher(store_, flags_).LuaToProto(typeId, obj, msg)) {
      luabind::class_info ci = luabind::call_function<luabind::class_info>(luabind::globals(L)["class_info"], obj);
      throw std::logic_error(BUILD_STRING("unknown conversion from object type " << ci.name << " to protobuf"));
   }
}

void Convert::ProtobufToUserdata(Protocol::Value const& msg, luabind::object& obj)
{
   lua_State* L = store_.GetInterpreter();
   typeinfo::TypeId typeId = msg.type_id();

   if (!typeinfo::Dispatcher(store_, flags_).ProtoToLua(typeId, msg, obj)) {
      luabind::class_info ci = luabind::call_function<luabind::class_info>(luabind::globals(L)["class_info"], obj);
      throw std::logic_error(BUILD_STRING("unknown conversion from object type " << ci.name << " to protobuf"));
   }
}

void Convert::LuaToProtobuf(luabind::object const &from, Protocol::LuaObject* msg, Protocol::LuaObject* rootmsg, std::vector<luabind::object>& tables)
{
   luabind::object obj = GetObjectSavedVariables(from);
   int t = luabind::type(obj);

   switch (t) {
   case LUA_TSTRING:
      msg->set_type(Protocol::LuaObject::STRING);
      msg->set_string_value(luabind::object_cast<std::string>(obj));
      break;
   case LUA_TNUMBER:
      msg->set_type(Protocol::LuaObject::NUMBER);
      msg->set_number_value(luabind::object_cast<double>(obj));
      break;
   case LUA_TBOOLEAN:
      msg->set_type(Protocol::LuaObject::BOOLEAN);
      msg->set_boolean_value(luabind::object_cast<bool>(obj));
      break;
   case LUA_TFUNCTION:
      msg->set_type(Protocol::LuaObject::FUNCTION);
      break;
   case LUA_TNIL:
      msg->set_type(Protocol::LuaObject::NIL);
      break;
   case LUA_TUSERDATA:
      {
         msg->set_type(Protocol::LuaObject::USERDATA);
         Protocol::Value *userdata_msg = msg->mutable_userdata_value();
         UserdataToProtobuf(obj, userdata_msg);
      }
      break;
   case LUA_TTABLE:
      {
         uint i;
         msg->set_type(Protocol::LuaObject::TABLE);
         for (i = 0; i < tables.size(); i++) {
            if (obj == tables[i]) {
               break;
            }
         }
         msg->set_table_value(i);
         if (i == tables.size()) {
            tables.push_back(obj);

            Protocol::LuaObject_Table* table_msg = rootmsg->add_tables();
            for (luabind::iterator i(from), end; i != end; i++) {
               luabind::object key = i.key();
            
               if (flags_ & Convert::REMOTE) {
                  // private variables don't get marshalled remotely.
                  if (luabind::type(key) == LUA_TSTRING &&
                        boost::starts_with(luabind::object_cast<std::string>(key), "_")) {
                     continue;
                  }
               }
               Protocol::LuaObject::Table::Entry *entry_msg = table_msg->add_entries();
               LuaToProtobuf(key, entry_msg->mutable_key(), rootmsg, tables);
               LuaToProtobuf(*i, entry_msg->mutable_value(), rootmsg, tables);
            }

         }
      }
      break;
   default:
      throw std::logic_error(BUILD_STRING("unknown type " << t << " in LuaToProtobuf"));
   }
}

void Convert::ProtobufToLua(Protocol::LuaObject const& msg, luabind::object &obj, Protocol::LuaObject const& rootmsg, std::unordered_map<uint, luabind::object>& tables)
{
   lua_State* L = store_.GetInterpreter();
   switch (msg.type()) {
   case Protocol::LuaObject::NIL:
   case Protocol::LuaObject::FUNCTION:
      obj = luabind::object();
      break;
   case Protocol::LuaObject::BOOLEAN:
      obj = luabind::object(L, msg.boolean_value());
      break;
   case Protocol::LuaObject::NUMBER:
      obj = luabind::object(L, msg.number_value());
      break;
   case Protocol::LuaObject::STRING:
      obj = luabind::object(L, msg.string_value());
      break;
   case Protocol::LuaObject::TABLE: {
         uint i = msg.table_value();
         if (!tables[i].is_valid()) {
            RestoreLuaTableFromProtobuf(i, rootmsg, tables);
         }
         obj = tables[i];
         ASSERT(obj && obj.is_valid());
      }
      break;
   case Protocol::LuaObject::USERDATA: {
      Protocol::Value const& value_msg = msg.userdata_value();
      ProtobufToUserdata(value_msg, obj);
      break;
   }
   default:
      throw std::logic_error(BUILD_STRING("unknown type " << msg.type() << " in LoadLuaValue"));
   }
}

void Convert::RestoreLuaTableFromProtobuf(uint i, Protocol::LuaObject const& rootmsg, std::unordered_map<uint, luabind::object>& tables)
{
   ASSERT(!tables[i].is_valid());
   luabind::object &obj = tables[i];
   Protocol::LuaObject_Table const& msg = rootmsg.tables(i);

   lua_State* L = store_.GetInterpreter();
   obj = luabind::newtable(L);
   for (Protocol::LuaObject::Table::Entry const& entry : msg.entries()) {
      luabind::object key, value;
      ProtobufToLua(entry.key(), key, rootmsg, tables);
      ProtobufToLua(entry.value(), value, rootmsg, tables);
      if (key.is_valid() && value.is_valid()) {
         obj[key] = value;
      }
   }
   ASSERT(tables[i].is_valid());
}


