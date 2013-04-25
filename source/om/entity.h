#ifndef _RADIANT_OM_ENTITY_H
#define _RADIANT_OM_ENTITY_H

#include <atomic>
#include "om.h"
#include "math3d.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/map.h"
#include "dm/store.h"
#include "all_object_types.h"

BEGIN_RADIANT_OM_NAMESPACE

class Entity : public dm::Record,
               public std::enable_shared_from_this<Entity> {
public:
   DEFINE_OM_OBJECT_TYPE(Entity);
   Entity();
   ~Entity();

   typedef dm::Map<dm::ObjectType, std::shared_ptr<dm::Object>> ComponentMap;

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
   void SetDebugName(std::string str); 

private:
   void InitializeRecordFields() override;

   NO_COPY_CONSTRUCTOR(Entity)

private:
   typedef std::unordered_map<dm::TraceId, std::function<void(dm::ObjectPtr)>> ComponentTraceMap;

   static std::atomic<dm::TraceId>                 nextTraceId_;
   static ComponentTraceMap                        componentTraces_[4];
   static std::vector<std::weak_ptr<dm::Object>>   newComponents_[4];
private:
   dm::Boxed<std::string>  debugname_;
   ComponentMap            components_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_H
