#include "pch.h"
#include "run_toward_point_action.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/carry_block.h"
#include "simulation/script/script_host.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

extern po::variables_map configvm;


static float angle(const math3d::vector3 &v)
{
   return math3d::atan2(v.z, -v.x) - math3d::atan2(-1, 0);
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
   dst_ = object_cast<math3d::point3>(arg1);
}

void RunTowardPoint::Update()
{
   auto mob = self_.lock()->GetComponent<om::Mob>();
   auto location = mob->GetLocation();

   //bool realtime = !configvm["game.noidle"].as<bool>();
   bool realtime = true;

   float maxDistance = realtime ? 0.4f : 10.0f;

   math3d::point3 current = mob->GetLocation();
   math3d::vector3 direction = math3d::vector3(dst_ - current);

   float distance = direction.length();
   if (distance < maxDistance) {
      mob->MoveTo(dst_);
      finished_ = true;
   } else {
      mob->TurnToAngle(angle(direction) * 180 / k_pi);
      mob->MoveTo(current + (direction * (maxDistance / distance)));
   }
}
