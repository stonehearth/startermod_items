#include "pch.h"
#include "renderer.h"
#include "render_attached_items.h"
#include "client/client.h"
#include "om/entity.h"
#include "om/components/attached_items.ridl.h"
#include "dm/record_trace.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define CB_LOG(level)      LOG(renderer.attached_items, level)

RenderAttachedItems::RenderAttachedItems(RenderEntity& renderEntity, om::AttachedItemsRef attachedItemsRef) :
   renderEntity_(renderEntity),
   attachedItemsRef_(attachedItemsRef)
{
   om::AttachedItemsPtr attachedItems = attachedItemsRef_.lock();
   if (!attachedItems) {
      return;
   }

   trace_ = attachedItems->TraceItems("render", dm::RENDER_TRACES)
      ->OnAdded([this](std::string const& boneName, om::EntityRef item) {
         DetachItemFromModel(boneName);
         AttachItemToModel(boneName, item);
      })
      ->OnRemoved([this](std::string const& boneName) {
         DetachItemFromModel(boneName);
      })
      ->PushObjectState();
}

RenderAttachedItems::~RenderAttachedItems()
{
}

void RenderAttachedItems::AttachItemToModel(std::string const& boneName, om::EntityRef itemRef)
{
   H3DNode boneNode = renderEntity_.GetSkeleton().GetSceneNode(boneName);
   if (!boneNode) {
      CB_LOG(5) << "bone " << boneName << " not found";
      return;
   }

   om::EntityPtr item = itemRef.lock();
   if (item) {
      CB_LOG(5) << "attaching item " << *item << " to bone " << boneName;
      RenderEntityPtr renderEntity = Renderer::GetInstance().CreateRenderObject(boneNode, item);
      renderEntities_[boneName] = renderEntity;
   } else {
      CB_LOG(5) << "invalid item to attach to bone " << boneName;
      renderEntities_.erase(boneName);
   }
}

void RenderAttachedItems::DetachItemFromModel(std::string const& boneName)
{
   H3DNode boneNode = renderEntity_.GetSkeleton().GetSceneNode(boneName);
   if (!boneNode) {
      CB_LOG(5) << "bone " << boneName << " not found";
      return;
   }

   auto iterator = renderEntities_.find(boneName);

   if (iterator != renderEntities_.end()) {
      RenderEntityPtr oldRenderEntity = iterator->second.lock();
            
      if (oldRenderEntity && oldRenderEntity->GetParent() == boneNode) {
         CB_LOG(5) << "detaching item " << *oldRenderEntity->GetEntity() << " from bone " << boneName;
         oldRenderEntity->SetParent(0);
      } else {
         CB_LOG(5) << "old item " << *oldRenderEntity->GetEntity() << " already detached from bone " << boneName;
      }

      renderEntities_.erase(boneName);
   } else {
      CB_LOG(5) << "nothing to detach from bone " << boneName;
   }
}
