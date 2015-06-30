#include "radiant.h"
#include "item.ridl.h"
#include "mob.ridl.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Item const& o)
{
   return (os << "[Item]");
}

void Item::ConstructObject()
{
   Component::ConstructObject();
   stacks_ = 1;
   max_stacks_ = 1;
}

void Item::LoadFromJson(json::Node const& obj)
{
   int count = obj.get<int>("stacks", *max_stacks_);
   stacks_ = count;
   max_stacks_ = count;

   category_ = obj.get<std::string>("category", *category_);   
}

void Item::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("stacks", GetStacks());
}


Item& Item::SetStacks(int value)
{ 
   if (*stacks_ == value) {
      return *this;
   }

   stacks_ = value;
   lua_State* L = GetStore().GetInterpreter();
   if (L) {
      lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);
      if (scriptHost) {
         luabind::object e(L, GetEntityRef());
         luabind::object evt(L, luabind::newtable(L));
         evt["entity"] = e;
         scriptHost->AsyncTriggerOn(e, "radiant:item:stacks_changed", evt);
         scriptHost->AsyncTrigger("radiant:item:stacks_changed", evt);
      }
   }
   return *this;
}
