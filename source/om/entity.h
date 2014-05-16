#ifndef _RADIANT_OM_ENTITY_H
#define _RADIANT_OM_ENTITY_H

#include <atomic>
#include "om.h"
#include "om/object_enums.h"
#include "om/macros.h"
#include "libjson.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/map.h"
#include "dm/store.h"

BEGIN_RADIANT_OM_NAMESPACE

class Entity : public dm::Record,
               public std::enable_shared_from_this<Entity> {
public:
   DEFINE_OM_OBJECT_TYPE(Entity, entity);

   void Destroy();

   typedef dm::Map<std::string, ComponentPtr>   ComponentMap;
   typedef dm::Map<std::string, DataStorePtr>   LuaComponentMap;

   const ComponentMap& GetComponents() const { return components_; }
   const LuaComponentMap& GetLuaComponents() const { return lua_components_; }

   std::shared_ptr<dm::MapTrace<ComponentMap>> TraceComponents(const char* reason, int category)
   {
      return components_.TraceChanges(reason, category);
   }
   std::shared_ptr<dm::MapTrace<LuaComponentMap>> TraceLuaComponents(const char* reason, int category)
   {
      return lua_components_.TraceChanges(reason, category);
   }

   template <class T> std::shared_ptr<T> AddComponent();
   template <class T> std::shared_ptr<T> GetComponent() const;
   ComponentPtr AddComponent(std::string const& name);
   ComponentPtr GetComponent(std::string const& name) const;

   DataStorePtr GetLuaComponent(std::string const& name) const;
   void AddLuaComponent(std::string const& name, DataStorePtr obj);

   template <class T> std::weak_ptr<T> AddComponentRef() { return AddComponent<T>(); }
   template <class T> std::weak_ptr<T> GetComponentRef() const { return GetComponent<T>(); }

   void RemoveComponent(std::string const& name);
   
   std::weak_ptr<Entity> GetRef() { return shared_from_this(); }
   std::shared_ptr<Entity> GetPtr() { return shared_from_this(); }

   std::string GetDebugText() const { return *debug_text_; }
   void SetDebugText(std::string str) { debug_text_ = str; }

   std::string GetUri() const { return *uri_; }
   void SetUri(std::string str) { uri_ = str; }

   void SerializeToJson(json::Node& node) const;

private:
   template <typename T> void CacheComponent(std::shared_ptr<T> component) { }
   template <typename T> std::shared_ptr<T> GetCachedComponent() const { return nullptr; }

   template <> void CacheComponent(std::shared_ptr<Mob> component) { cached_mob_component_ = component; }
   template <> std::shared_ptr<Mob> GetCachedComponent() const { return cached_mob_component_.lock(); }

private:
   void InitializeRecordFields() override;
   NO_COPY_CONSTRUCTOR(Entity)

private:
   dm::Boxed<std::string>  debug_text_;
   dm::Boxed<std::string>  uri_;
   ComponentMap            components_;
   LuaComponentMap         lua_components_;

private:
   std::weak_ptr<Mob>      cached_mob_component_;
};
std::ostream& operator<<(std::ostream& os, const Entity& o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_H
