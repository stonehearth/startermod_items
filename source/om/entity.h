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
#include "core/object_counter.h"

BEGIN_RADIANT_OM_NAMESPACE

class Entity : public dm::Record,
               public std::enable_shared_from_this<Entity>,
               public core::ObjectCounter<Entity> {
public:
   DEFINE_OM_OBJECT_TYPE(Entity, entity);

   void Destroy();
 
   typedef dm::Map<dm::CString, om::ComponentPtr, dm::SharedCStringHash, dm::CStringKeyTransform<0>> ComponentMap;
   typedef dm::Map<dm::CString, om::DataStorePtr, dm::SharedCStringHash, dm::CStringKeyTransform<0>> LuaComponentMap;

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
   ComponentPtr AddComponent(const char* name);
   ComponentPtr GetComponent(const char* name) const;

   DataStorePtr GetLuaComponent(const char* name) const;
   void AddLuaComponent(const char* name, DataStorePtr obj);

   template <class T> std::weak_ptr<T> AddComponentRef() { return AddComponent<T>(); }
   template <class T> std::weak_ptr<T> GetComponentRef() const { return GetComponent<T>(); }

   void RemoveComponent(const char* name);
   
   std::weak_ptr<Entity> GetRef() { return shared_from_this(); }
   std::shared_ptr<Entity> GetPtr() { return shared_from_this(); }

   std::string GetDebugText() const { return *debug_text_; }
   void SetDebugText(std::string const& str) { debug_text_ = str; }

   std::string GetUri() const { return *uri_; }
   void SetUri(std::string const& str) { uri_ = str; }

   void SerializeToJson(json::Node& node) const;
   void OnLoadObject(dm::SerializationType r) override;

private:
   template <typename T> void CacheComponent(std::shared_ptr<T> component) {
   }
   template <typename T> std::shared_ptr<T> GetCachedComponent() const {
      return nullptr;
   }
   template <typename T> bool IsCachedComponent() const {
      return false;
   }

   template <> void CacheComponent<Mob>(std::shared_ptr<Mob> component) {
      cached_mob_component_ = component;
   }
   template <> std::shared_ptr<Mob> GetCachedComponent<Mob>() const {
      return cached_mob_component_.lock();
   }
   template <> bool IsCachedComponent<Mob>() const {
      /* we would like to return true here (i.e. always rely on the cached version), but that
       * causes problems on the client.  since no one did an AddComponent on the client, the
       * cache pointer will never get hooked up!  we could trace the container to determine when
       * the mob changes, but that's an extra tracer per entity for no good reason!
       */
      return !cached_mob_component_.expired();
   }

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
std::ostream& operator<<(std::ostream& os, Entity const& o);
std::ostream& operator<<(std::ostream& os, std::shared_ptr<Entity> const& o);

bool IsRootEntity(EntityRef entityRef);
bool IsRootEntity(EntityPtr entityPtr);
EntityPtr GetEntityRoot(EntityPtr entityPtr);
bool IsInWorld(EntityPtr entityPtr);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_H
