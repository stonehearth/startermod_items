#include "radiant.h"
#include "om/entity.h"
#include "om/components/room.h"
#include "om/components/unit_info.h"
#include "om/components/entity_container.h"
#include "om/components/build_orders.h"
#include "simulation/simulation.h"
#include "simulation/script/script_host.h"
#include "create_room_cmd.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

// xxx: why is this native???
std::string CreateRoomCmd::operator()(const tesseract::protocol::DoAction& msg) const
{
   assert(false);
   return "";
#if 0
   auto& sim = Simulation::GetInstance();
   csg::Cube3 bounds(msg.args(0).bounds());

   om::EntityPtr room = sim.GetScript().CreateEntity("http://radiant/stonehearth/buildings/room_plan");
   room->AddComponent<om::Room>()->SetInteriorSize(bounds._min, bounds._max);

   LOG(WARNING) << "!!! created room " << room->GetObjectId();
   sim.GetRootEntity()->GetComponent<om::BuildOrders>()->AddBlueprint(room);

   std::ostringstream result;
   result << "{ \"entity_id\": " << room->GetObjectId() << "}";
   return result.str();
#endif
}

