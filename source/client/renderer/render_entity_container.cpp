#include "pch.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_entity_container.h"
#include "pipeline.h"
#include "om/components/entity_container.h"
#include "Horde3DUtils.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderEntityContainer::RenderEntityContainer(const RenderEntity& entity, om::EntityContainerPtr container) :
   entity_(entity)
{
   auto added = std::bind(&RenderEntityContainer::AddChild, this, std::placeholders::_1, std::placeholders::_2);
   auto removed = std::bind(&RenderEntityContainer::RemoveChild, this, std::placeholders::_1);

   const auto& children = container->GetChildren();
   tracer_ += children.TraceMapChanges("render entity children", added, removed);
   Update(children);
}

RenderEntityContainer::~RenderEntityContainer()
{
}

void RenderEntityContainer::Update(const om::EntityContainer::Container& children)
{
   children_.clear();
   for (const auto &entry : children) {
      AddChild(entry.first, entry.second);
   }
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

