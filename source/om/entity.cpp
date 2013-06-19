#include "pch.h"
#include "entity.h"
#include "grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Entity::InitializeRecordFields()
{
   // LOG(WARNING) << "creating entity " << GetObjectId();
   AddRecordField("components",     components_);
   AddRecordField("debugname",      debugname_);
   AddRecordField("resource_uri",   resource_uri_);
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
      components_[T::DmType] = component;

      // Hold onto the new comp
   }
   return component;
}

dm::ObjectPtr Entity::GetComponent(dm::ObjectType t) const
{
   auto i = components_.find(t);
   return i != components_.end() ? i->second : nullptr;
}

template <class T> std::shared_ptr<T> Entity::GetComponent() const
{
   return std::static_pointer_cast<T>(GetComponent(T::DmType));
}

#define OM_OBJECT(Clas, lower) \
   template std::shared_ptr<Clas> Entity::GetComponent() const; \
   template std::shared_ptr<Clas> Entity::AddComponent<Clas>();
OM_ALL_COMPONENTS
#undef OM_OBJECT

