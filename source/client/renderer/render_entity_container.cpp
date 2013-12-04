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
      // LOG(WARNING) << "adding child entity to container " << child->GetObjectId();
      children_[key] = Renderer::GetInstance().CreateRenderObject(entity_.GetNode(), child);
   }
}

void RenderEntityContainer::RemoveChild(dm::ObjectId key)
{
   auto i = children_.find(key);
   if (i != children_.end()) {
      LOG(WARNING) << "remmoving child entity to container " << key;
      auto e = i->second.lock();
      if (e) {
         if (e->GetParent() == entity_.GetNode()) {
            e->SetParent(0);
         }
      }
   }
   children_.erase(key);
}

