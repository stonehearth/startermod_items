#ifndef _RADIANT_CLIENT_RENDER_BUILD_ORDERS_H
#define _RADIANT_CLIENT_RENDER_BUILD_ORDERS_H

#include <map>
#include "dm/dm.h"
#include "dm/map.h"
#include "om/om.h"
#include "om/components/build_orders.h"
#include "types.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderBuildOrders : public RenderComponent {
public:
   RenderBuildOrders(const RenderEntity& entity, om::BuildOrdersPtr container);
   ~RenderBuildOrders();

private:
   void UpdateNodeFlags();
   void Update(const om::BuildOrders::BlueprintList& children);
   void AddChild(om::EntityRef blueprint);
   void RemoveChild(om::EntityRef blueprint);

private:
   typedef std::unordered_map<dm::ObjectId, std::weak_ptr<RenderEntity>> BuildprintMap;

   const RenderEntity&  entity_;
   H3DNode              node_;
   dm::GuardSet         guards_;
   BuildprintMap        blueprints_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_BUILD_ORDERS_H
