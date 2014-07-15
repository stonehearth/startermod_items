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
      
      std::shared_ptr<RenderEntity> re = Renderer::GetInstance().GetRenderEntity(child);
      if (!re || re->GetParent() == RenderNode::GetUnparentedNode()) {
         // Make sure we place ourselves under the origin node of the entity.  That
         // represents the exact position of the entity in the world.  GetNode()
         // returns the origin node plus some rendering fix-up offsets to position the
         // entity correctly (e.g. aligning walls and columns to the grid, even when their
         // origin is in the center of a grid tile).
         H3DNode parent = _entity.GetOriginNode();
         Renderer::GetInstance().CreateRenderEntity(parent, child);
      }
   }
}
