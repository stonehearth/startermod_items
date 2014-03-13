#include "pch.h"
#include "entity.h"
#include "om/all_components.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define E_LOG(level)    LOG_CATEGORY(om.entity, level, *this)

std::ostream& ::radiant::om::operator<<(std::ostream& os, Entity const& o)
{
   // ug! luabind!!
   if (&o == nullptr) {
      return (os << "(invalid entity reference)");
   }

   std::string debug_text = o.GetDebugText();

   os << "(Entity " << o.GetObjectId();
   std::string annotation;
   UnitInfoPtr unit_info = o.GetComponent<UnitInfo>();
   if (unit_info) {
      annotation = unit_info->GetDisplayName();
   }
   if (annotation.empty()) {
      annotation = o.GetUri();
   }
   if (!annotation.empty()) {
      os << " " << annotation;
   }
   if (!debug_text.empty()) {
      os << " " << debug_text;
   }
   os << ")";
   return os;
}

Entity::~Entity()
{
}

void Entity::Destroy()
{
   for (const auto& entry : lua_components_.GetContents()) {
      luabind::object obj = entry.second;
      lua_State* L = lua::ScriptHost::GetCallbackThread(obj.interpreter());
      try {
         luabind::object destroy = obj["destroy"];
         if (destroy) {
            E_LOG(3) << "destroying component " << entry.first;
            luabind::object cb(L, destroy);
            cb(obj);
         }
      } catch (std::exception const& e) {
         E_LOG(1) << "error destroying component '" << entry.first << "':" << e.what();
      }
   }
}

void Entity::InitializeRecordFields()
{
   AddRecordField("components",      components_);
   AddRecordField("lua_components",  lua_components_);
   AddRecordField("debug_text",      debug_text_);
   AddRecordField("uri",             uri_);
   E_LOG(3) << "creating entity " << GetObjectId();
}

void Entity::SerializeToJson(json::Node& node) const
{
   Record::SerializeToJson(node);

   std::string debug_text = GetDebugText();
   std::string uri = GetUri();

   if (!uri.empty()) {
      node.set("uri", uri);
   }
   if (!debug_text.empty()) {
      node.set("debug_text", debug_text);
   }
   for (auto const& entry : GetComponents()) {
      node.set(entry.first, entry.second->GetStoreAddress());
   }

   lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(GetStore().GetInterpreter());
   for (auto const& entry : GetLuaComponents()) {
      node.set(entry.first, scriptHost->LuaToJson(entry.second));
   }
}

template <class T> std::shared_ptr<T> Entity::AddComponent()
{
   std::shared_ptr<T> component = GetComponent<T>();
   if (!component) {
      component = GetStore().AllocObject<T>();
      // xxx - this is potentially dangerous if the lifetime of the component exceeds
      // that of the owning entity.  we can defend against this better if we return
      // weak references from GetComponent, etc.
      component->SetEntity(shared_from_this()); 
      components_.Add(component->GetObjectClassNameLower(), component);
      CacheComponent<T>(component);
   }
   return component;
}


ComponentPtr Entity::AddComponent(std::string const& name)
{
   ComponentPtr obj = GetComponent(name);

   if (!obj) {
#define OM_OBJECT(Clas, lower)  \
      else if (name == #lower) { \
         obj = AddComponent<Clas>(); \
      } 

      if (false) {
      } OM_ALL_COMPONENTS
#undef OM_OBJECT
   }
   return obj;
}


ComponentPtr Entity::GetComponent(std::string const& name) const
{
   return components_.Get(name, nullptr);
}

void Entity::AddLuaComponent(std::string const& name, luabind::object component)
{
   // client side lua will use set_lua_component_data to construct temporary objects...
   // ASSERT(!lua_components_.Contains(name));

   // xxx: need to fire a trace here...
   lua_components_.Add(name, component);
}

luabind::object Entity::GetLuaComponent(std::string const& name) const
{
   auto i = lua_components_.find(name);
   if (i != lua_components_.end()) {
      return i->second;
   }
   return luabind::object();
}

template <class T> std::shared_ptr<T> Entity::GetComponent() const
{
   std::shared_ptr<T> component = GetCachedComponent<T>();
   if (!component) {
      component = std::static_pointer_cast<T>(GetComponent(T::GetClassNameLower()));
   }
   return component;
}

void Entity::RemoveComponent(std::string const& name)
{
   components_.Remove(name);
   lua_components_.Remove(name);
}

#define OM_OBJECT(Clas, lower) \
   template std::shared_ptr<Clas> Entity::GetComponent() const; \
   template std::shared_ptr<Clas> Entity::AddComponent<Clas>();
OM_ALL_COMPONENTS
#undef OM_OBJECT


