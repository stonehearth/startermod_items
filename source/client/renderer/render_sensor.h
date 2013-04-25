#ifndef _RADIANT_CLIENT_RENDER_SENSOR_H
#define _RADIANT_CLIENT_RENDER_SENSOR_H

#include <map>
#include "render_aspect.h"
#include "object_model/namespace.h"
#include "math3d/collision/bounding_sphere.h"

IN_RADIANT_OBJECT_MODEL_NAMESPACE(
   class ISensor;
)

BEGIN_RADIANT_CLIENT_NAMESPACE

class debug_shapes;
class OgreRenderer;

class RenderSensor : public RenderAspect {
   public:
      RenderSensor(H3DNode parent, ObjectModel::ISensor *sensor);
      virtual ~RenderSensor();

      void PrepareToRender(int now, float alpha) override;

   protected:
      void UpdateSensor();

   protected:
      om::SensorPtr       sensor_;
      math3d::bounding_sphere    sphere_;
      math3d::color4             color_;
      H3DNode            debugShape_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_SENSOR_H
