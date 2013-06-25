#ifndef _RADIANT_OM_ENTITY_H
#define _RADIANT_OM_ENTITY_H

#include <atomic>
#include "om.h"
#include "math3d.h"
#include "libjson.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/map.h"
#include "dm/store.h"
#include "all_object_types.h"

BEGIN_RADIANT_OM_NAMESPACE

class Entity : public dm::Record,
               public std::enable_shared_from_this<Entity> {
public:
   DEFINE_OM_OBJECT_TYPE(Entity, entity);
   virtual ~Entity() { }

   typedef dm::Map<dm::ObjectType, std::shared_ptr<Object>> ComponentMap;

   const ComponentMap& GetComponents() const { return components_; }

   template <class T> std::shared_ptr<T> AddComponent();
   template <class T> std::shared_ptr<T> GetComponent() const;
   dm::ObjectPtr GetComponent(dm::ObjectType t) const;

   template <class T> std::weak_ptr<T> AddComponentRef() { return AddComponent<T>(); }
   template <class T> std::weak_ptr<T> GetComponentRef() const { return GetComponent<T>(); }
   
   om::EntityId GetEntityId() const { return GetObjectId(); }
   std::weak_ptr<Entity> GetRef() { return shared_from_this(); }
   std::shared_ptr<Entity> GetPtr() { return shared_from_this(); }

   std::string GetDebugName() const { return *debugname_; }
   void SetDebugName(std::string str) { debugname_ = str; }

   std::string GetResourceUri() const { return *resource_uri_; }
   void SetResourceUri(std::string str) { resource_uri_ = str; }

private:
   void InitializeRecordFields() override;

   NO_COPY_CONSTRUCTOR(Entity)

private:
   typedef std::unordered_map<dm::TraceId, std::function<void(dm::ObjectPtr)>> ComponentTraceMap;

private:
   dm::Boxed<std::string>  debugname_;
   dm::Boxed<std::string>  resource_uri_;
   ComponentMap            components_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_H
