#include "pch.h"
#include "lua_components.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

// xxx: move this into a helper or header or somewhere.  EVERYONE needs to
// do this.  actually, we can't we just inherit it in the luabind registration
// stuff???
template <class T>
std::string ToJsonUri(std::weak_ptr<T> o, luabind::object state)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << "\"/object/" << obj->GetObjectId() << "\"";
   } else {
      output << "null";
   }
   return output.str();
}

LuaComponent::LuaComponent() :
   dm::Object(),
   cached_json_valid_(false)
{
}

void LuaComponent::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;
   module(L) [
      class_<LuaComponent, LuaComponentRef>("LuaComponent")
         .def(tostring(self))
         .def("__tojson", &ToJsonUri<LuaComponent>)
         .def("mark_changed", &LuaComponent::MarkChanged)
   ];
}

std::string LuaComponent::ToString() const
{
   std::ostringstream o;
   o << "[LuaComponent " << GetObjectId() << "]";
   return o.str();
}

void LuaComponent::SetLuaObject(std::string const& name, luabind::object obj)
{
   name_ = name;
   obj_ = obj;
}

void LuaComponent::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   // xxx: this isn't going to work for save and load.  we need to serialize something
   // which can then be unserialized (probably via eval!!), which means it will have
   // class names in it, which means it won't be valid json! 
   //
   // this is idea for remoting, though, so let's do it.  SaveValue/LoadValue should
   // probably be renamed to something which indicates they're for remoting (or
   // removed entirely from the object and moved elsewhere!!!)
   std::string json = ToJson().write();
   dm::SaveImpl<std::string>::SaveValue(store, msg, json);
}

void LuaComponent::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   cached_json_ = libjson::parse(json);
   cached_json_valid_ = true;
}

JSONNode LuaComponent::ToJson() const
{
   using namespace luabind;
   if (!cached_json_valid_) {
      cached_json_ = JSONNode();
      
      object tojson = obj_["__tojson"];
      if (luabind::type(tojson) != LUA_TNIL) {
         std::string json = call_function<std::string>(tojson, obj_);
         if (!libjson::is_valid(json)) {
            // xxx: actually, throw an exception
            return JSONNode();
         }
         cached_json_ = libjson::parse(json);
      }
      // xxx: skip this until we have legitimate change tracking
      // cached_json_valid_ = true; 
   }
   return cached_json_;
}

void LuaComponents::ExtendObject(json::ConstJsonObject const& obj) 
{
}

luabind::scope LuaComponents::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   LuaComponent::RegisterLuaType(L);
   return
      class_<LuaComponents, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("get_lua_component",               &om::LuaComponents::GetLuaComponent)
         .def("add_lua_component",               &om::LuaComponents::AddLuaComponent)
      ;
}

std::string LuaComponents::ToString() const
{
   std::ostringstream os;
   os << "(LuaComponents id:" << GetObjectId() << ")";
   return os.str();
}

LuaComponentPtr LuaComponents::GetLuaComponent(std::string name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? nullptr : i->second;
}

LuaComponentPtr LuaComponents::AddLuaComponent(std::string name)
{
   ASSERT(lua_components_.find(name) == lua_components_.end());

   auto component = GetStore().AllocObject<LuaComponent>();
   lua_components_[name] = component;
   return component;
}

