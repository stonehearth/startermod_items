#include "radiant.h"
#include "convert.h"
#include "dm/store.h"
#include "protocols/store.pb.h"
#include "lib/lua/bind.h"
#include "lib/typeinfo/dispatcher.h"
#include <EASTL/fixed_vector.h>
#include <boost/algorithm/string/predicate.hpp>

#include "lib/lua/script_host.h"

using namespace radiant;
using namespace radiant::marshall;

#define CONVERT_LOG(level)    LOG_CATEGORY(convert, level, "[" << ((flags_ & marshall::Convert::REMOTE) ? "remote" : "store") << "]")

static std::string ToString(lua_State* L, luabind::object &o) {
   luabind::object tostring = luabind::globals(L)["tostring"];
   return luabind::object_cast<std::string>(tostring(o));
}

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

luabind::object Convert::GetObjectSavedVariables(luabind::object const& obj)
{
   luabind::object saved_variables = obj;

   eastl::fixed_vector<luabind::object, 8, true> visited;

   int t = luabind::type(saved_variables);
   while (t == LUA_TTABLE) {
      for (auto const& i : visited) {
         if (i == saved_variables) {
            CONVERT_LOG(1) << "breaking infinite loop in __saved_variables definition for object.";
            CONVERT_LOG(1) << lua::ScriptHost::LuaToJson(obj.interpreter(), obj).write_formatted();
            ASSERT(false); // ASSERT immediately so we find the bug.  Will be removed in release builds
            return obj; // Return the original object if we pass the ASSERT
         }
      }
      visited.push_back(saved_variables);

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
      luabind::object __get_userdata_type_id = obj["__get_userdata_type_id"];
      if (!__get_userdata_type_id || !__get_userdata_type_id.is_valid()) {
         int top = lua_gettop(L);
         obj.push(L);
         LOG(core.system, 0) << "error: lua object " << luabind::get_class_info(luabind::from_stack(L, top+1)).name << " not registered with dynamic type info in UserdataToProtobuf";
         lua_settop(L, top);
         msg->set_type_id(0);
         return;
      }
      typeId = luabind::object_cast<int>(__get_userdata_type_id());
   } catch (std::exception const& e) {
      LUA_LOG(0) << "wtf? " << e.what();
   }
   if (!typeId) {
      luabind::class_info ci = luabind::object_cast<luabind::class_info>(luabind::globals(L)["class_info"](obj));
      throw std::logic_error(BUILD_STRING("object type " << ci.name << " has no __userdata_type_id field"));
   }

   if (!typeinfo::Dispatcher(store_, flags_).LuaToProto(typeId, obj, msg)) {
	  luabind::class_info ci = luabind::object_cast<luabind::class_info>(luabind::globals(L)["class_info"](obj));
      throw std::logic_error(BUILD_STRING("unknown conversion from object type " << ci.name << " to protobuf"));
   }
}

void Convert::ProtobufToUserdata(Protocol::Value const& msg, luabind::object& obj)
{
   lua_State* L = store_.GetInterpreter();
   typeinfo::TypeId typeId = msg.type_id();

   if (typeId == 0) {
      CONVERT_LOG(3) << "protobuf type_id is 0!  returning nil";
      obj = luabind::object();
      return;
   }
   if (!typeinfo::Dispatcher(store_, flags_).ProtoToLua(typeId, msg, obj)) {
      luabind::class_info ci = luabind::object_cast<luabind::class_info>(luabind::globals(L)["class_info"](obj));
      throw std::logic_error(BUILD_STRING("unknown conversion from object type " << ci.name << " to protobuf"));
   }
}

void Convert::LuaToProtobuf(luabind::object const &from, Protocol::LuaObject* msg, Protocol::LuaObject* rootmsg, std::vector<luabind::object>& tables)
{
   if (luabind::type(from) == LUA_TTABLE) {
      // A Lua table is an instance of a class if it has member variable called __type == "object"
      luabind::object objectType = from["__type"];
      bool isInstance = luabind::type(objectType) == LUA_TSTRING && 
                        luabind::object_cast<std::string>(objectType) == "object";
      if (isInstance) {
         // It's impossible to reasonably convert an instance of a class to a serializable form.  There's
         // all these random instance variables like __type and __class that we should (or shouldn't?)
         // serialize, etc.  If the instance has its state locked away in a __saved_variables member, go
         // ahead and convert that (since they know what they're doing), but otherwise convert the whole
         // thing to a nil.  Also assert.  This is a logical error on the part of the programmer!!
         if (luabind::type(from["__saved_variables"]) == LUA_TNIL) {
            msg->set_type(Protocol::LuaObject::NIL);

            CONVERT_LOG(1) << "cannot convert class instance with no delegated __saved_variables to protobuf!";
            CONVERT_LOG(1) << lua::ScriptHost::LuaToJson(from.interpreter(), from).write_formatted();
            msg->set_type(Protocol::LuaObject::NIL);
            return;
         }
      }
   }

   luabind::object obj = GetObjectSavedVariables(from);
   int t = luabind::type(obj);

   switch (t) {
   case LUA_TSTRING:
      msg->set_type(Protocol::LuaObject::STRING);
      msg->set_string_value(luabind::object_cast<std::string>(obj));
      CONVERT_LOG(7) << "lua LUA_TSTRING -> " << msg->string_value();
      break;
   case LUA_TNUMBER:
      msg->set_type(Protocol::LuaObject::NUMBER);
      msg->set_number_value(luabind::object_cast<double>(obj));
      CONVERT_LOG(7) << "lua LUA_TNUMBER -> " << msg->number_value();
      break;
   case LUA_TBOOLEAN:
      msg->set_type(Protocol::LuaObject::BOOLEAN);
      msg->set_boolean_value(luabind::object_cast<bool>(obj));
      CONVERT_LOG(7) << "lua LUA_TBOOLEAN -> " << msg->boolean_value();
      break;
   case LUA_TFUNCTION:
      msg->set_type(Protocol::LuaObject::FUNCTION);
      CONVERT_LOG(7) << "lua LUA_TFUNCTION -> nil";
      break;
   case LUA_TNIL:
      msg->set_type(Protocol::LuaObject::NIL);
      CONVERT_LOG(7) << "lua LUA_TNIL -> nil";
      break;
   case LUA_TUSERDATA:
      {
         msg->set_type(Protocol::LuaObject::USERDATA);
         Protocol::Value *userdata_msg = msg->mutable_userdata_value();
         UserdataToProtobuf(obj, userdata_msg);
         CONVERT_LOG(7) << "lua LUA_TNIL -> (see above)";
      }
      break;
   case LUA_TTABLE:
      {
         uint i = SaveLuaTableToProtobuf(obj, rootmsg, tables);
         msg->set_type(Protocol::LuaObject::TABLE);
         msg->set_table_value(i);
         CONVERT_LOG(7) << "lua LUA_TTABLE -> (table index " << i << ")";
      }
      break;
   default:
      throw std::logic_error(BUILD_STRING("unknown type " << t << " in LuaToProtobuf"));
   }
}

void Convert::ProtobufToLua(Protocol::LuaObject const& msg, luabind::object &obj, Protocol::LuaObject const& rootmsg, std::unordered_map<uint, luabind::object>& tables)
{
   log::Indent indent;

   lua_State* L = store_.GetInterpreter();
   switch (msg.type()) {
   case Protocol::LuaObject::NIL:
      obj = luabind::object();
      CONVERT_LOG(7) << "protobuf NIL -> nil";
      break;
   case Protocol::LuaObject::FUNCTION:
      obj = luabind::object();
      CONVERT_LOG(7) << "protobuf FUNCTION -> nil";
      break;
   case Protocol::LuaObject::BOOLEAN:
      obj = luabind::object(L, msg.boolean_value());
      CONVERT_LOG(7) << "protobuf BOOLEAN -> " << ToString(L, obj);
      break;
   case Protocol::LuaObject::NUMBER:
      obj = luabind::object(L, msg.number_value());
      CONVERT_LOG(7) << "protobuf NUMBER -> " << ToString(L, obj);
      break;
   case Protocol::LuaObject::STRING:
      obj = luabind::object(L, msg.string_value());
      CONVERT_LOG(7) << "protobuf STRING -> " << ToString(L, obj);
      break;
   case Protocol::LuaObject::TABLE: {
         uint i = msg.table_value();
         if (!tables[i].is_valid()) {
            RestoreLuaTableFromProtobuf(i, rootmsg, tables);
         }
         obj = tables[i];
         ASSERT(obj && obj.is_valid());
         CONVERT_LOG(7) << "protobuf TABLE -> " << ToString(L, obj);
      }
      break;
   case Protocol::LuaObject::USERDATA: {
      Protocol::Value const& value_msg = msg.userdata_value();
      ProtobufToUserdata(value_msg, obj);
      CONVERT_LOG(7) << "protobuf USERDATA -> " << ToString(L, obj);
      break;
   }
   default:
      throw std::logic_error(BUILD_STRING("unknown type " << msg.type() << " in LoadLuaValue"));
   }
}

uint Convert::SaveLuaTableToProtobuf(luabind::object const& obj, Protocol::LuaObject* rootmsg, std::vector<luabind::object>& tables)
{
   uint i;
   for (i = 0; i < tables.size(); i++) {
      if (obj == tables[i]) {
         return i;
      }
   }
   tables.push_back(obj);

   CONVERT_LOG(7) << "saving lua LUA_TTABLE -> { (table index " << i << ")";
   {
      lua_State* L = obj.interpreter();

      Protocol::LuaObject_Table* table_msg = rootmsg->add_tables();
      for (luabind::iterator i(obj), end; i != end; ++i) {
         luabind::object key = i.key();
            
         if (flags_ & Convert::REMOTE) {
            // private variables don't get marshalled remotely.
            if (luabind::type(key) == LUA_TSTRING &&
                  boost::starts_with(luabind::object_cast<std::string>(key), "_")) {
               continue;
            }
         }
         if (luabind::type(key) == LUA_TUSERDATA) {
            CONVERT_LOG(1) << "cannot store table key with type LUA_TUSERDATA";
            continue;
         }
         Protocol::LuaObject::Table::Entry *entry_msg = table_msg->add_entries();
         CONVERT_LOG(7) << "converting table key \"" << ToString(L, key);
         LuaToProtobuf(key, entry_msg->mutable_key(), rootmsg, tables);
         CONVERT_LOG(7) << "converting table value \"" << ToString(L, luabind::object(*i));
         LuaToProtobuf(*i, entry_msg->mutable_value(), rootmsg, tables);
      }
   }
   CONVERT_LOG(7) << "}";
   return i;
}

void Convert::RestoreLuaTableFromProtobuf(uint i, Protocol::LuaObject const& rootmsg, std::unordered_map<uint, luabind::object>& tables)
{
   ASSERT(!tables[i].is_valid());
   luabind::object &obj = tables[i];
   Protocol::LuaObject_Table const& msg = rootmsg.tables(i);

   CONVERT_LOG(7) << "restoring protobuf TABLE -> {";
   {
      log::Indent indent;

      lua_State* L = store_.GetInterpreter();
      obj = luabind::newtable(L);
      for (Protocol::LuaObject::Table::Entry const& entry : msg.entries()) {
         luabind::object key;
         ProtobufToLua(entry.key(), key, rootmsg, tables);
         if (luabind::type(key) == LUA_TUSERDATA) {
            CONVERT_LOG(1) << "ignoring table key with type LUA_TUSERDATA";
            continue;
         }
         if (luabind::type(key) == LUA_TNIL) {
            CONVERT_LOG(1) << "ignoring table key with type LUA_TNIL";
            continue;
         }

         luabind::object value;
         ProtobufToLua(entry.value(), value, rootmsg, tables);
         if (key.is_valid() && value.is_valid()) {
            luabind::object tostring = luabind::globals(L)["tostring"];
            volatile std::string strval = luabind::object_cast<std::string>(tostring(key));
            obj[key] = value;
            CONVERT_LOG(7) << "setting \"" << ToString(L, key) << "\" = " << ToString(L, value);
         }
      }
      ASSERT(tables[i].is_valid());
   }
   CONVERT_LOG(7) << "}";
}

