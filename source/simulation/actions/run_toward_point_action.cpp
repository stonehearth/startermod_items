#include "pch.h"
#include "run_toward_point_action.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

static float angle(const csg::Point3f &v)
{
   return (float)(atan2(v.z, -v.x) - atan2(-1, 0));
}

RunTowardPoint::RunTowardPoint()
{
}

void RunTowardPoint::Create(lua_State* L)
{
   using namespace luabind;

   int nargs = lua_gettop(L);
   object arg0(from_stack(L, -2));
   object arg1(from_stack(L, -1));

   self_ = object_cast<om::EntityRef>(arg0);
   dst_ = object_cast<csg::Point3f>(arg1);
}

void RunTowardPoint::Update()
{
   auto mob = self_.lock()->GetComponent<om::Mob>();
   auto location = mob->GetLocation();

   float maxDistance = 0.4f;

   csg::Point3f current = mob->GetLocation();
   csg::Point3f direction = csg::Point3f(dst_ - current);

   float distance = direction.Length();
   if (distance < maxDistance) {
      mob->MoveTo(dst_);
      finished_ = true;
   } else {
      mob->TurnTo(angle(direction) * 180 / csg::k_pi);
      mob->MoveTo(current + (direction * (maxDistance / distance)));
   }
}
