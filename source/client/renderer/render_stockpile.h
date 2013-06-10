#ifndef _RADIANT_CLIENT_RENDER_STOCKPILE_H
#define _RADIANT_CLIENT_RENDER_STOCKPILE_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "render_component.h"
#include "horde3d/extensions/stockpile/stockpile_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderStockpile : public RenderComponent {
   public:
      RenderStockpile(const RenderEntity& entity, om::LuaComponentPtr stockpile);
      ~RenderStockpile();

   protected:
      void UpdateStockpile(JSONNode const& data);

   protected:
      const RenderEntity&        entity_;
      H3DNode                    debugShape_;
      H3DNode                    node_;
      horde3d::StockpileNode*    nodeObj_;
      dm::GuardSet               traces_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_STOCKPILE_H
