#ifndef _RADIANT_CLIENT_RENDER_STRUCTURE_H
#define _RADIANT_CLIENT_RENDER_STRUCTURE_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderStructure : public RenderComponent {
   public:
      RenderStructure(const RenderEntity& entity, om::StructurePtr stockpile);
      ~RenderStructure();

   protected:
      void UpdateRegion(const csg::Region3& region);
      void UpdateReserved(const csg::Region3& region);

   protected:
      const RenderEntity&     entity_;
      H3DNode                 regionDebugShape_;
      H3DNode                 reservedDebugShape_;
      H3DNode                 node_;
      dm::Guard               traces_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_STRUCTURE_H
