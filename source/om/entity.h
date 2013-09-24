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
   virtual ~Entity() { }

   dm::Guard TraceObjectChanges(const char* reason, std::function<void()> fn) const override;

   typedef dm::Map<dm::ObjectType, std::shared_ptr<Object>> ComponentMap;

   const ComponentMap& GetComponents() const { return components_; }

   template <class T> std::shared_ptr<T> AddComponent();
   template <class T> std::shared_ptr<T> GetComponent() const;
   dm::ObjectPtr GetComponent(dm::ObjectType t) const;

   template <class T> std::weak_ptr<T> AddComponentRef() { return AddComponent<T>(); }
   template <class T> std::weak_ptr<T> GetComponentRef() const { return GetComponent<T>(); }
   
   std::weak_ptr<Entity> GetRef() { return shared_from_this(); }
   std::shared_ptr<Entity> GetPtr() { return shared_from_this(); }

   std::string GetDebugText() const { return *debug_text_; }
   void SetDebugText(std::string str) { debug_text_ = str; }

   void SetName(std::string const &mod_name, std::string const& entity_name) {
      mod_name_ = mod_name;
      entity_name_ = entity_name;
   }
   std::string GetModuleName() const { return *mod_name_; }
   std::string GetEntityName() const { return *entity_name_; }

private:
   void InitializeRecordFields() override;
   NO_COPY_CONSTRUCTOR(Entity)

private:
   typedef std::unordered_map<dm::TraceId, std::function<void(dm::ObjectPtr)>> ComponentTraceMap;

private:
   dm::Boxed<std::string>  debug_text_;
   dm::Boxed<std::string>  mod_name_;
   dm::Boxed<std::string>  entity_name_;
   ComponentMap            components_;
};
std::ostream& operator<<(std::ostream& os, const Entity& o);
std::ostream& operator<<(std::ostream& os, EntityPtr o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_H
