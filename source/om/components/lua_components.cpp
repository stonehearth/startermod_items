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

void LuaComponents::ExtendObject(json::ConstJsonObject const& obj) 
{
}

DataBindingPtr LuaComponents::GetLuaComponent(std::string name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? nullptr : i->second;
}

DataBindingPtr LuaComponents::AddLuaComponent(std::string name)
{
   ASSERT(lua_components_.find(name) == lua_components_.end());

   auto component = GetStore().AllocObject<DataBinding>();
   lua_components_[name] = component;
   return component;
}

