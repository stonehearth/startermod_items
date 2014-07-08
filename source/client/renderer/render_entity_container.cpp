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
   _entity(entity)
{
   // RenderEntities make it into the render tree by virtual of being linked as a child
   // to some other Entity which is a descendant of the Root Node.  This component only
   // does the initial creation of the RenderEntity.  If it gets reparented, the RenderMob
   // class will take care of it.
   _childrenTrace = container->TraceChildren("render", dm::RENDER_TRACES)
                           ->OnAdded([=](dm::ObjectId id, om::EntityRef child) {
                              CreateEntity(child);
                           })
                           ->PushObjectState();

   _attachedItemsTrace = container->TraceAttachedItems("render", dm::RENDER_TRACES)
                           ->OnAdded([=](dm::ObjectId id, om::EntityRef child) {
                              CreateEntity(child);
                           })
                           ->PushObjectState();
}

RenderEntityContainer::~RenderEntityContainer()
{
}

void RenderEntityContainer::CreateEntity(om::EntityRef childRef)
{
   auto child = childRef.lock();
   if (child) {
      EC_LOG(5) << "adding child entity to container " << child->GetObjectId();
      H3DNode parent = _entity.GetOriginNode();
      Renderer::GetInstance().CreateRenderEntity(parent, child);
   }
}
