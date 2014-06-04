#include "pch.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_entity_container.h"
#include "pipeline.h"
#include "dm/map_trace.h"
#include "om/components/entity_container.ridl.h"
#include "Horde3DUtils.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define EC_LOG(level)      LOG(renderer.entity_container, level)

RenderEntityContainer::RenderEntityContainer(const RenderEntity& entity, om::EntityContainerPtr container) :
   entity_(entity)
{
   trace_ = container->TraceChildren("render", dm::RENDER_TRACES)
                           ->OnAdded([=](dm::ObjectId id, om::EntityRef child) {
                              AddChild(id, child);
                           })
                           ->OnRemoved([=](dm::ObjectId id) {
                              RemoveChild(id);
                           })
                           ->PushObjectState();
}

RenderEntityContainer::~RenderEntityContainer()
{
}

void RenderEntityContainer::AddChild(dm::ObjectId key, om::EntityRef childRef)
{
   auto child = childRef.lock();
   if (child) {
      EC_LOG(5) << "adding child entity to container " << child->GetObjectId();
      // Make sure we place ourselves under the origin node of the entity.  That
      // represents the exact position of the entity in the world.  GetNode()
      // returns the origin node plus some rendering fix-up offsets to position the
      // entity correctly (e.g. aligning walls and columns to the grid, even when their
      // origin is in the center of a grid tile).
      H3DNode parent = entity_.GetOriginNode();
      children_[key] = Renderer::GetInstance().CreateRenderObject(parent, child);
   }
}

void RenderEntityContainer::RemoveChild(dm::ObjectId key)
{
   auto i = children_.find(key);
   if (i != children_.end()) {
      EC_LOG(5) << "remmoving child entity to container " << key;
      auto e = i->second.lock();
      if (e) {
         if (e->GetParent() == entity_.GetNode()) {
            e->SetParent(0);
         }
      }
   }
   children_.erase(key);
}

