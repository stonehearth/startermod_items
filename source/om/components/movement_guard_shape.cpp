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
   //_guardCb = luabind::object(cb_thread, unsafe_cb);
   if (!_frc) {
      // xxx: There's currently no way tor this filter to get invalidated!  The
      // proper fix probably involves getting FRC's to listen on something and
      // automatically do the invalidate themselves, but we have no way to
      // listen from cpp. =(
      int flags = simulation::FilterResultCache::INVALIDATE_ON_PLAYER_ID_CHANGED;
      _frc = std::make_shared<simulation::FilterResultCache>(flags);
   }
   _frc ->SetLuaFilterFn(unsafe_cb);
}

bool MovementGuardShape::CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& pt)
{
   om::Region3fBoxedPtr const& region = GetRegion();
   if (!region) {
      return true;
   }

   if (!_frc) {
      return true;
   }

   csg::Point3f location = csg::ToFloat(pt);
   csg::Region3f r = phys::LocalToWorld(region->Get(), GetEntityPtr());
   if (!r.Contains(location)) {
      return true;
   }

   return _frc->ConsiderEntity(std::static_pointer_cast<om::Entity>(entity));
}
