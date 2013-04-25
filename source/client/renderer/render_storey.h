#ifndef _RADIANT_CLIENT_RENDER_STOREY_H
#define _RADIANT_CLIENT_RENDER_STOREY_H

#include <map>
#include "render_aspect.h"
#include "render_grid.h"
#include "object_model/namespace.h"

IN_RADIANT_OBJECT_MODEL_NAMESPACE(
   class Storey;
)

BEGIN_RADIANT_CLIENT_NAMESPACE

class OgreRenderer;
class RenderGrid;
class RenderFloor;

class RenderStorey : public RenderAspect {
   public:
      RenderStorey(H3DNode parent, ObjectModel::Storey* storey);
      virtual ~RenderStorey();

      void PrepareToRender(int now, float distance) override;
      void AddToSelection(om::Selection& sel, const math3d::ray3& ray, const math3d::point3& intersection, const math3d::point3& normal) override;

   protected:
      void UpdateLayer(std::shared_ptr<world::Object> obj, std::string name);

   protected:
      vector<RenderGrid*>    grids_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_STOREY_H
