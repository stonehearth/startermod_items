#ifndef _RADIANT_CLIENT_RENDER_STOCKPILE_DESIGNATION_H
#define _RADIANT_CLIENT_RENDER_STOCKPILE_DESIGNATION_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "render_component.h"
#include "horde3d/extensions/stockpile/stockpile_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderStockpileDesignation : public RenderComponent {
   public:
      RenderStockpileDesignation(const RenderEntity& entity, om::StockpileDesignationPtr stockpile);
      ~RenderStockpileDesignation();

   protected:
      void UpdateStockpile(om::StockpileDesignationRef s);
      void UpdateAvailable(om::StockpileDesignationRef s);
      void UpdateReserved(om::StockpileDesignationRef s);

   protected:
      const RenderEntity&     entity_;
      H3DNode                 debugShape_;
      H3DNode                 node_;
      horde3d::StockpileNode* nodeObj_;
      dm::GuardSet              traces_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_STOCKPILE_DESIGNATION_H
