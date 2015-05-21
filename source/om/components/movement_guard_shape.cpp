#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "movement_guard_shape.ridl.h"
#include "lib/lua/bind.h"
#include "region_common.h"
#include "physics/physics_util.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, MovementGuardShape const& o)
{
   return (os << "[MovementGuardShape]");
}

void MovementGuardShape::LoadFromJson(json::Node const& obj)
{
   region_ = LoadRegion(obj, GetStore());
}

void MovementGuardShape::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::Region3fBoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }
}

void MovementGuardShape::SetGuardCb(luabind::object const& unsafe_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_cb.interpreter());  
   _guardCb = luabind::object(cb_thread, unsafe_cb);
}

bool MovementGuardShape::CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& pt)
{
   om::Region3fBoxedPtr const& region = GetRegion();
   if (!region) {
      return true;
   }

   csg::Point3f location = csg::ToFloat(pt);
   csg::Region3f r = phys::LocalToWorld(region->Get(), GetEntityPtr());
   if (!r.Contains(location)) {
      return true;
   }
   if (luabind::type(_guardCb) == LUA_TNIL) {
      return false;
   }

   bool canPass = false;
   try {
      luabind::object result = _guardCb(om::EntityRef(entity), location);
      if (luabind::type(result) == LUA_TBOOLEAN) {
         canPass = luabind::object_cast<bool>(result);
      }
   } catch (std::exception const& e) {
      lua::ScriptHost::ReportCStackException(_guardCb.interpreter(), e);
   }
   return canPass;
}
