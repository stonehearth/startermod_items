#ifndef _RADIANT_CLIENT_RENDER_SENSOR_LIST_H
#define _RADIANT_CLIENT_RENDER_SENSOR_LIST_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderSensorList : public RenderComponent {
   public:
      RenderSensorList(const RenderEntity& entity, om::SensorListPtr sensorList);

   private:
      RenderEntity const&           entity_;
      om::SensorListRef             sensorList_;
      dm::TracePtr                  trace_;
      core::Guard                   renderer_guard_;
      std::unordered_map<std::string, SharedNode>  shapes_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_SENSOR_LIST_H
