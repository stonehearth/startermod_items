#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "om/components/sensor.ridl.h"
#include "om/components/sensor_list.ridl.h"
#include "dm/map_trace.h"
#include "render_sensor_list.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderSensorList::RenderSensorList(const RenderEntity& entity, om::SensorListPtr sensorList) :
   entity_(entity)
{
   sensorList_ = sensorList;
   H3DNode parent = entity.GetNode();

   renderer_guard_ = Renderer::GetInstance().OnShowDebugShapesChanged([this, parent](bool enabled) {
      if (enabled) {
         om::SensorListPtr sensorList = sensorList_.lock();
         if (sensorList) {
            trace_ = sensorList->TraceSensors("debug rendering", dm::RENDER_TRACES)
               ->OnAdded([this, parent](std::string const& name, om::SensorPtr sensor) {
                  H3DNode s = h3dRadiantAddDebugShapes(parent, name.c_str());
                  h3dRadiantAddDebugBox(s, csg::ToFloat(sensor->GetCube()), csg::Color4(255, 192, 0, 255));
                  h3dRadiantCommitDebugShape(s);
                  shapes_[name] = s;
               })
               ->PushObjectState();
         }
      } else {
         trace_ = nullptr;
         shapes_.clear();
      }
   });
}
