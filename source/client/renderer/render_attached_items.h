#ifndef _RADIANT_CLIENT_RENDER_ATTACHED_ITEMS_H
#define _RADIANT_CLIENT_RENDER_ATTACHED_ITEMS_H

#include <unordered_map>
#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "dm/dm.h"
#include "dm/map_trace.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderAttachedItems : public RenderComponent {
public:
   RenderAttachedItems(RenderEntity& renderEntity, om::AttachedItemsRef attachedItems);
   ~RenderAttachedItems();

private:
   void AttachItemToModel(std::string const& boneName, om::EntityRef itemRef);
   void DetachItemFromModel(std::string const& boneName);

private:
   RenderEntity&        renderEntity_;
   om::AttachedItemsRef attachedItemsRef_;
   std::unordered_map<std::string, RenderEntityRef> renderEntities_;
   std::shared_ptr<dm::MapTrace<dm::Map<std::string, om::EntityRef>>> trace_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_ATTACHED_ITEMS_H
