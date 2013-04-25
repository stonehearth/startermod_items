#include "pch.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_build_orders.h"
#include "pipeline.h"
#include "om/components/entity_container.h"
#include "Horde3DUtils.h"
#include "client/client.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderBuildOrders::RenderBuildOrders(const RenderEntity& entity, om::BuildOrdersPtr bo) :
   entity_(entity)
{
   node_ = h3dAddGroupNode(entity_.GetNode(), "build order container");

   auto added = std::bind(&RenderBuildOrders::AddChild, this, std::placeholders::_1);
   auto removed = std::bind(&RenderBuildOrders::RemoveChild, this, std::placeholders::_1);

   const auto& blueprints = bo->GetBlueprints();
   guards_ += blueprints.TraceSetChanges("render build order blueprints", added, removed);

   guards_ += Client::GetInstance().TraceShowBuildOrders([=]() { UpdateNodeFlags(); });
   Update(blueprints);
   UpdateNodeFlags();
}

RenderBuildOrders::~RenderBuildOrders()
{
   h3dRemoveNode(node_);
}

void RenderBuildOrders::UpdateNodeFlags()
{
   if (Client::GetInstance().GetShowBuildOrders()) {
      h3dSetNodeFlags(node_, 0, true);
   } else {
      h3dSetNodeFlags(node_, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);
   }
}

void RenderBuildOrders::Update(const om::BuildOrders::BlueprintList& children)
{
   blueprints_.clear();
   for (const auto &entry : children) {
      AddChild(entry);
   }
}

void RenderBuildOrders::AddChild(om::EntityRef b)
{
   om::EntityPtr blueprint = b.lock();
   if (blueprint) {
      om::EntityId id = blueprint->GetEntityId();
      LOG(WARNING) << "adding blueprint entity to bo " << id;
      blueprints_[id] = Renderer::GetInstance().CreateRenderObject(node_, blueprint);
   }
}

void RenderBuildOrders::RemoveChild(om::EntityRef b)
{
   om::EntityPtr blueprint = b.lock();
   if (blueprint) {
      om::EntityId id = blueprint->GetEntityId();
      auto i = blueprints_.find(id);
      if (i != blueprints_.end()) {
         LOG(WARNING) << "remmoving child entity to bo " << id;
         RenderEntityPtr e = i->second.lock();
         if (e->GetParent() == node_) {
            e->SetParent(0);
         }
      }
      blueprints_.erase(id);
   }
}

