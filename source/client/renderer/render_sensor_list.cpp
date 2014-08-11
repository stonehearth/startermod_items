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

RenderSensorList::RenderSensorList(RenderEntity const& entity, om::SensorListPtr sensorList) :
   entity_(entity)
{
   sensorList_ = sensorList;

   renderer_guard_ = Renderer::GetInstance().OnShowDebugShapesChanged([this](bool enabled) {
      if (enabled) {
         om::SensorListPtr sensorList = sensorList_.lock();
         if (sensorList) {
            trace_ = sensorList->TraceSensors("render sensors", dm::RENDER_TRACES)
               ->OnAdded([this](std::string const& name, om::SensorPtr const& sensor) {
                  AddSensorShape(name, sensor);
               })
               ->PushObjectState();
         }
      } else {
         trace_ = nullptr;
         shapes_.clear();
      }
   });
}

void RenderSensorList::AddSensorShape(std::string const& name, om::SensorPtr const& sensor)
{
   // using the origin node because sensors should render without a rotation or render offset
   H3DNode origin = entity_.GetOriginNode();
   H3DNode s = h3dRadiantAddDebugShapes(origin, name.c_str());
   csg::Cube3f renderCube = csg::ToFloat(sensor->GetCube());
   // Shift the renderCube by (-0.5,-0.5,-0.5) to place the entity in the center of the sensor.
   // i.e. The origin voxel of the sensor has integer bounds (0,0,0)-(1,1,1), so shifting the bounds
   // by (-0.5,-0.5,-0.5) centers the cube around the entity's origin at (0,0,0) local coordinates.
   // See sensor_list.cpp for an extended discussion of sensor regions.
   renderCube.Translate(csg::Point3f(-0.5, -0.5, -0.5));
   h3dRadiantAddDebugBox(s, renderCube, csg::Color4(255, 192, 0, 255));
   h3dRadiantCommitDebugShape(s);
   shapes_[name] = s;
}
