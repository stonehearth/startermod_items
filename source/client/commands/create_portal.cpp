#include "pch.h"
#include "create_portal.h"
#include "client/client.h"
#include "client/renderer/renderer.h"
#include "om/selection.h"
#include "om/entity.h"
#include "dm/store.h"
#include "om/stonehearth.h"
#include "om/components/room.h"
#include "om/components/fixture.h"
#include "om/components/wall.h"
#include "om/components/mob.h"
#include "om/components/portal.h"
#include "om/components/render_rig.h"
#include "om/components/entity_container.h"
#include "client/client.h"
#include "client/renderer/render_entity.h"
#include "wait_for_entity_alloc.h"

using namespace radiant;
using namespace radiant::client;


CreatePortal::CreatePortal(PendingCommandPtr cmd) :
   inputHandlerId_(0)
{
   cmd->Complete(JSONNode());
}

CreatePortal::~CreatePortal()
{
   if (inputHandlerId_ != 0) {
      Renderer::GetInstance().RemoveInputEventHandler(inputHandlerId_);
      inputHandlerId_ = 0;
   }
}

void CreatePortal::operator()(void)
{
   ASSERT(inputHandlerId_ == 0);
   EndShadowing();

   auto cb = std::bind(&CreatePortal::OnMouseEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
   inputHandlerId_ = Renderer::GetInstance().SetMouseInputCallback(cb);

   csg::Point2 pt = Renderer::GetInstance().GetMousePosition();
   FindShadowWall(pt.x, pt.y);
}

void CreatePortal::OnMouseEvent(const MouseInputEvent &me, bool &handled, bool &uninstall) 
{
   if (!shadowWall_) {
      FindShadowWall(me.x, me.y);
   } else {
      if (!UpdatePortalPosition(me.x, me.y)) {
         EndShadowing();
      }
   }
   if (me.buttons[0] && portal_) {
      SendCommand();
   }
}

void CreatePortal::FindShadowWall(int windowX, int windowY)
{
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(windowX, windowY, s);
   if (s.HasBlock() && s.HasEntities()) {
      const math3d::ipoint3& pt = s.GetBlock();
      auto entity = Client::GetInstance().GetEntity(s.GetEntities().front());
      if (entity) {
         auto wall = entity->GetComponent<om::Wall>();
         if (wall) {
            BeginShadowing(entity);
            MovePortal(pt);
         }
      }
   }
}

bool CreatePortal::UpdatePortalPosition(int x, int y)
{
   auto remoteWall = remoteWall_.lock();
   if (!remoteWall) {
      return false;
   }

   float d;
   const csg::Region3& rgn = remoteWall->GetComponent<om::Wall>()->GetRegion();

   math3d::ray3 ray = Renderer::GetInstance().GetCameraToViewportRay(x, y);

   // Transform the ray into the coordinate system of the wall
   ray.origin -= shadowWallTransform_.position;
   ray.direction = shadowWallTransform_.orientation.inverse().rotate(ray.direction);
   
   if (!csg::Region3Intersects(rgn, ray, d)) {
      return false;
   }

   csg::Point3 pt = ray.origin + (ray.direction * d);
   MovePortal(pt);

   return true;
}

void CreatePortal::BeginShadowing(om::EntityPtr wall)
{
   // If the mouse isn't over a wall at all, stop shadowing and just
   // bail.
   if (!wall) {
      EndShadowing();
      return;
   }

   // If we've moused over different wall or the current shadow of
   // the wall is gone, we need to end shadowing.
   auto remoteWall = remoteWall_.lock();
   if (!remoteWall || remoteWall->GetObjectId() != wall->GetObjectId()) {
      EndShadowing();
   }

   // If we don't have a shadow wall, go ahead and create one by
   // cloning into the authoring store.
   if (!shadowWall_) {
      dm::CloneMapping mapping;
      shadowWall_ = Client::GetInstance().GetAuthoringStore().AllocObject<om::Entity>();
      wall->CloneDynamicObject(shadowWall_, mapping);      

      // Hide the OG wall...
      remoteWall_ = wall;
      shadowWallTransform_ = wall->GetComponent<om::Mob>()->GetWorldTransform();
      shadowWall_->GetComponent<om::Mob>()->MoveToGridAligned(math3d::ipoint3(shadowWallTransform_.position));

      Renderer::GetInstance().CreateRenderObject(1, shadowWall_);
      auto ro = Renderer::GetInstance().GetRenderObject(wall);
      ro->Show(false);
   }
}

void CreatePortal::EndShadowing()
{
   auto remoteWall = remoteWall_.lock();
   if (remoteWall) {
      auto ro = Renderer::GetInstance().GetRenderObject(remoteWall);
      ro->Show(true);
   }
   remoteWall_.reset();
   shadowWall_.reset();
   portal_.reset();
}

void CreatePortal::MovePortal(const csg::Point3& location)
{
   om::MobPtr portalMob;
   auto wall = shadowWall_->GetComponent<om::Wall>();
   auto wallNormal = wall->GetNormal();
   dm::Store& store = wall->GetStore();

   if (!portal_) {
      float angle;
      if (wallNormal.x == -1 && wallNormal.z == 0) {
         angle = 90;
      } else if (wallNormal.x == 0 && wallNormal.z == 1) {
         angle = 180;
      } else if (wallNormal.x == 1 && wallNormal.z == 0) {
         angle = 270;
      } else {
         angle = 0;
      }
      portal_ = om::Stonehearth::CreateEntity(store, "module://stonehearth/buildings/wooden_door");
      wall->AddFixture(portal_);

      portalMob = portal_->GetComponent<om::Mob>();
      portalMob ->TurnToAngle(angle);
      auto fixture = portal_->GetComponent<om::Fixture>();
      if (fixture) {
         om::EntityPtr item = om::Stonehearth::CreateEntity(store, fixture->GetItemKind());
         fixture->SetItem(item);
      }
      auto portal = portal_->GetComponent<om::Portal>();
      ASSERT(portal);
      portalBounds_ = portal->GetRegion().GetBounds();
   } else {
      portalMob = portal_->GetComponent<om::Mob>();
   }

   portalPosition_ = location;
   int normal, tangent;
   if (wallNormal.x) {
      normal = 0, tangent = 2;
   } else {
      normal = 2, tangent = 0;
   }
   portalPosition_[normal] = 0;
   portalPosition_.y = 0;

   csg::Point3 size = shadowWall_->GetComponent<om::Wall>()->GetSize();
   int minOffset = -portalBounds_.GetMin().x;
   int maxOffset = size[tangent] - portalBounds_.GetMax().x;
   portalPosition_[tangent] = std::max(minOffset, std::min(maxOffset, portalPosition_[tangent]));

   ASSERT(portalPosition_.x == 0 || portalPosition_.z == 0);
   portalMob->MoveToGridAligned(portalPosition_);
   wall->StencilOutPortals();
}

void CreatePortal::SendCommand()
{
   auto remoteWall = remoteWall_.lock();
   if (!remoteWall || !shadowWall_) {
      return;
   }

   auto self = shared_from_this();
   auto shadowWall = shadowWall_;
   int id = Client::GetInstance().CreateCommandResponse([self, remoteWall, shadowWall](const tesseract::protocol::DoActionReply *msg) {
      // It's fine to do nothing here... we just needed to keep the room
      // object alive long enough.
      if (msg) {
         JSONNode node = libjson::parse(msg->json());
         auto i = node.find("entity_id");
         if (i != node.end() && i->type() == JSON_NUMBER) {
            om::EntityId id = i->as_int();
            WaitForEntityAlloc(Client::GetInstance().GetStore(), id, [self, remoteWall, shadowWall]() {
               // Show the remote wall again...
               auto ro = Renderer::GetInstance().GetRenderObject(remoteWall);
               if (ro) {
                  ro->Show(true);
               }

               LOG(WARNING) << "Destroying authoring wall " << shadowWall->GetEntityId();
               if (self.use_count() > 1) {
                  (*self)();
               }
            });
         }
      }
      LOG(WARNING) << "destroying the room.  BOOM!";
   });

   std::vector<om::Selection> args;
   args.push_back(om::Selection(remoteWall->GetComponent<om::Wall>()));
   args.push_back(om::Selection("module://stonehearth/buildings/wooden_door"));
   args.push_back(om::Selection(portalPosition_));
   Client::GetInstance().SendCommand(0, "radiant.commands.create_portal", args, id);

   // Here we go again!
   remoteWall_.reset();
   portal_.reset();
   shadowWall_.reset();
   Renderer::GetInstance().RemoveInputEventHandler(inputHandlerId_);
   inputHandlerId_ = 0;
}
