#include "pch.h"
#include "entity.h"
#include "om/components/unit_info.ridl.h"

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

void Entity::ConstructObject()
{
}

void Entity::Destroy()
{
   for (const auto& entry : components_.GetContents()) {
      if (entry.second->GetObjectType() == DataStoreObjectType) {
         om::DataStorePtr ds = std::static_pointer_cast<DataStore>(entry.second);
         luabind::object controller = ds->GetController().GetLuaObject();
         if (controller) {
            lua_State* L = lua::ScriptHost::GetCallbackThread(controller.interpreter());
            try {
               luabind::object destroy = controller["destroy"];
               if (destroy) {
                  E_LOG(3) << "destroying component " << entry.first;
                  luabind::object cb(L, destroy);
                  cb(controller);
               }
            } catch (std::exception const& e) {
               E_LOG(1) << "error destroying component '" << entry.first << "':" << e.what();
            }
         }
      }
   }
}

void Entity::InitializeRecordFields()
{
   AddRecordField("components",  components_);
   AddRecordField("debug_text",  debug_text_);
   AddRecordField("uri",         uri_);
   E_LOG(3) << "creating entity " << GetObjectId();
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

dm::ObjectPtr Entity::GetComponent(std::string const& name) const
{
   return components_.Get(name, nullptr);
}

void Entity::AddComponent(std::string const& name, DataStorePtr component)
{
   ASSERT(!components_.Contains(name));
   components_.Add(name, component);
}

template <class T> std::shared_ptr<T> Entity::GetComponent() const
{
   std::shared_ptr<T> component = GetCachedComponent<T>();
   if (!component) {
      component = std::static_pointer_cast<T>(GetComponent(T::GetClassNameLower()));
   }
   return component;
}

#define OM_OBJECT(Clas, lower) \
   template std::shared_ptr<Clas> Entity::GetComponent() const; \
   template std::shared_ptr<Clas> Entity::AddComponent<Clas>();
OM_ALL_COMPONENTS
#undef OM_OBJECT


