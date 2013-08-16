#include "pch.h"
#include "create_room.h"
#include "client/client.h"
#include "client/renderer/renderer.h"
#include "om/selection.h"
#include "om/entity.h"
#include "dm/store.h"
#include "om/components/room.h"
#include "client/client.h"
#include "wait_for_entity_alloc.h"

using namespace radiant;
using namespace radiant::client;


CreateRoom::CreateRoom(PendingCommandPtr cmd)
{
   cmd->CompleteSuccessObj(JSONNode());
}

CreateRoom::~CreateRoom()
{
   if (selector_) {
      selector_->Deactivate(); // Selectors should have a trace that gets returned on activate! (and dm::Guard should be dm::Guard)
   }
}

void CreateRoom::operator()(void)
{
   csg::Point3 p(0, 0, 0);

   room_ = Client::GetInstance().GetAuthoringStore().AllocObject<om::Entity>();
   room_->SetDebugName(std::string("authored room"));
   room_->AddComponent<om::Room>()->SetInteriorSize(p, p);
   Renderer::GetInstance().CreateRenderObject(1, room_);

   selector_ = std::make_shared<XZRegionSelector>(Client::GetInstance().GetTerrain());
   selector_->RegisterSelectionFn([=](const om::Selection& s) {
      SendCommand(s);
   });

   selector_->RegisterFeedbackFn([=](const math3d::ibounds3& b) {
      ResizeRoom(b._min, b._max);
   });

   selector_->Activate();
}

bool CreateRoom::ResizeRoom(const csg::Point3& p0, const csg::Point3& p1)
{
   room_->AddComponent<om::Room>()->SetInteriorSize(p0, p1);
   return true;
}

void CreateRoom::SendCommand(const om::Selection& s)
{
   std::vector<om::Selection> args;
   args.push_back(s);

   auto room = room_;
   int id = Client::GetInstance().CreateCommandResponse([room](const tesseract::protocol::DoActionReply *msg) {
      // It's fine to do nothing here... we just needed to keep the room
      // object alive long enough.
      if (msg) {
         JSONNode node = libjson::parse(msg->json());
         auto i = node.find("entity_id");
         if (i != node.end() && i->type() == JSON_NUMBER) {
            dm::ObjectId id = i->as_int();
            WaitForEntityAlloc(Client::GetInstance().GetStore(), id, [room]() {
               LOG(WARNING) << "Destroying authoring room " << room->GetObjectId();
            });
         }
      }
      LOG(WARNING) << "destroying the room.  BOOM!";
   });
   Client::GetInstance().SendCommand(0, "create_room", args, id);

   // Here we go again!
   (*this)();
}
