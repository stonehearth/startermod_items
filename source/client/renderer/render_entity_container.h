#ifndef _RADIANT_CLIENT_RENDER_ENTITY_CONTAINER_H
#define _RADIANT_CLIENT_RENDER_ENTITY_CONTAINER_H

#include <map>
#include "dm/dm.h"
#include "dm/map.h"
#include "om/om.h"
#include "om/components/entity_container.ridl.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderEntityContainer : public RenderComponent {
public:
   RenderEntityContainer(const RenderEntity& entity, om::EntityContainerPtr container);
   ~RenderEntityContainer();

private:
   void CreateEntity(om::EntityRef child);

private:
   const RenderEntity&  _entity;
   dm::TracePtr         _childrenTrace;
   dm::TracePtr         _attachedItemsTrace;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_ENTITY_CONTAINER_H
