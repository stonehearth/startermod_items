#ifndef _RADIANT_CLIENT_RENDER_SCAFFOLDING_H
#define _RADIANT_CLIENT_RENDER_SCAFFOLDING_H

#include "render_build_order.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderScaffolding : public RenderBuildOrder {
   public:
      RenderScaffolding(const RenderEntity& entity, om::ScaffoldingPtr scaffolding);
      virtual ~RenderScaffolding();

   protected:
      om::ScaffoldingPtr GetScaffolding() const { return scaffolding_.lock(); }
      void UpdateScaffolding();
      void UpdateRegion(const csg::Region3& region) override;
      void UpdateLadder(const csg::Region3& region);

   private:
      H3DNode                 ladderDebugShape_;
      om::ScaffoldingRef       scaffolding_;
      float                    rotation_;
      csg::Point3              normal_;
      H3DNode                  blocksNode_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_SCAFFOLDING_H
