#include "radiant.h"
#include "unit_info.ridl.h"
#include "om/entity.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, UnitInfo const& o)
{
   return (os << "[UnitInfo " << o.GetDisplayName() << "]");
}

void UnitInfo::LoadFromJson(json::Node const& obj)
{
   display_name_ = obj.get<std::string>("name", *display_name_);
   description_ = obj.get<std::string>("description", *description_);
   player_id_ = obj.get<std::string>("player_id", *player_id_);
   icon_ = obj.get<std::string>("icon", *icon_);
}

void UnitInfo::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("name", GetDisplayName());
   node.set("description", GetDescription());
   node.set("player_id", GetPlayerId());
   node.set("icon", GetIcon());
}

UnitInfo& UnitInfo::SetPlayerId(std::string value)
{ 
   player_id_ = value;
   lua_State* L = GetStore().GetInterpreter();
   if (L) {
      lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);
      if (scriptHost) {
         luabind::object e(L, GetEntityRef());
         luabind::object evt(L, luabind::newtable(L));
         evt["entity"] = e;
         scriptHost->AsyncTriggerOn(e, "radiant:unit_info:player_id_changed", evt);
         scriptHost->AsyncTrigger("radiant:unit_info:player_id_changed", evt);
      }
   }
   return *this;
}
