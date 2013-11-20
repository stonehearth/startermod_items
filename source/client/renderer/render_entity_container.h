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
   void Update(const om::EntityContainer::Container& children);
   void AddChild(dm::ObjectId key, om::EntityRef child);
   void RemoveChild(dm::ObjectId key);

private:
   typedef std::unordered_map<dm::ObjectId, std::weak_ptr<RenderEntity>> EntityContainerMap;

   const RenderEntity&  entity_;
   core::Guard              tracer_;
   EntityContainerMap   children_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_ENTITY_CONTAINER_H
