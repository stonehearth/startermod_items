#include "radiant.h"
#include "entity.h"
#include "om/all_components.h"
#include "lib/lua/bind.h"
#include "om/components/data_store.ridl.h"

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

   os << "(" << o.GetObjectId();
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
   if (o.GetStore().GetName() != "game") {
      os << " (" << o.GetStore().GetName() << ")";
   }
   os << ")";
   return os;
}

void Entity::Destroy()
{
   // Our component datastores are manually allocated, so we manually destroy them, too.  (All
   // others go through 'sim'.
   for (auto& entry : lua_components_) {
      entry.second->CallLuaDestructor();
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
   for (auto const& entry : GetLuaComponents()) {
      node.set(entry.first, entry.second->GetStoreAddress());
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


ComponentPtr Entity::AddComponent(const char* name)
{
   ComponentPtr obj = GetComponent(name);

   if (!obj) {
#define OM_OBJECT(Clas, lower)  \
      else if (strcmp(name, #lower) == 0) { \
         obj = AddComponent<Clas>(); \
      } 

      if (false) {
      } OM_ALL_COMPONENTS
#undef OM_OBJECT
   }
   return obj;
}


ComponentPtr Entity::GetComponent(const char* name) const
{
   if (strcmp(name, Mob::GetClassNameLower()) == 0 && 
         IsCachedComponent<Mob>()) { // xxx: this can go once IsCachedComponent returns true uncondiionally

      // Make sure we go through the cache mechanism, even when using the const char* version
      // of ::GetComponent.  This is a perf-loss for non-Mob components, but a HUGE per win
      // for Mobs, which are extremely extremely commonly queried for.
      return GetComponent<Mob>();
   }
   return components_.Get(name, ComponentPtr());
}

void Entity::AddLuaComponent(const char* name, DataStorePtr obj)
{
   // client side lua will use set_lua_component_data to construct temporary objects...
   // ASSERT(!lua_components_.Contains(name));

   // xxx: need to fire a trace here...
   lua_components_.Add(name, obj);
}

DataStorePtr Entity::GetLuaComponent(const char* name) const
{
   auto i = lua_components_.find(name);
   if (i != lua_components_.end()) {
      return i->second;
   }
   return nullptr;
}

template <class T> std::shared_ptr<T> Entity::GetComponent() const
{
   if (IsCachedComponent<T>()) {
      return GetCachedComponent<T>();
   }
   return std::static_pointer_cast<T>(GetComponent(T::GetClassNameLower()));
}

void Entity::RemoveComponent(const char* name)
{
   components_.Remove(name);

   auto i = lua_components_.find(name);
   if (i != lua_components_.end()) {
      i->second->CallLuaDestructor();
      lua_components_.Remove(name);
   }
}

void Entity::OnLoadObject(dm::SerializationType r)
{
   CacheComponent<Mob>(std::static_pointer_cast<Mob>(components_.Get(Mob::GetClassNameLower(), ComponentPtr())));
}

#define OM_OBJECT(Clas, lower) \
   template std::shared_ptr<Clas> Entity::GetComponent() const; \
   template std::shared_ptr<Clas> Entity::AddComponent<Clas>();
OM_ALL_COMPONENTS
#undef OM_OBJECT

bool radiant::om::IsRootEntity(EntityRef entityRef)
{
   om::EntityPtr entity = entityRef.lock();
   if (entity) {
      return IsRootEntity(entity);
   } else {
      return false;
   }
}

bool radiant::om::IsRootEntity(EntityPtr entity)
{
   if (entity->IsValid()) {
      return entity->GetObjectId() == 1;
   } else {
      return false;
   }
}

EntityPtr radiant::om::GetEntityRoot(EntityPtr entity)
{
   EntityPtr parent;
   MobPtr mob;

   while (true) {
      parent = nullptr;
      mob = entity->GetComponent<Mob>();
      if (mob) {
         parent = mob->GetParent().lock();
      }
      if (!parent) {
         return entity;
      }
      entity = parent;
   }
}

bool radiant::om::IsInWorld(EntityPtr entity)
{
   EntityPtr entityRoot = GetEntityRoot(entity);
   return IsRootEntity(entityRoot);
}


