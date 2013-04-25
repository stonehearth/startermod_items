#include "pch.h"
#include "render_sensor.h"
#include "object_model/sensors/isensor.h"
#include "object_model/icollision_object.h"
#include "Horde3DRadiant.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderSensor::RenderSensor(H3DNode parent, ObjectModel::ISensor *sensor) :
   sensor_(sensor)
{
   std::string name = std::string("Sensor ") + stdutil::ToString(sensor->GetId());
   debugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());
}

RenderSensor::~RenderSensor()
{
}

void RenderSensor::PrepareToRender(int now, float alpha)
{
   auto sphere = sensor_->GetInterface<ObjectModel::ISphereCollisionShape>();
   if (sphere) {
      //LOG(WARNING) << color_ << " vs. " << sensor_->GetDebugColor() << "(" << sensor_->GetId() << ")";
      if (sphere_ != sphere->GetSphereCollisionShape() ||
          color_ != sensor_->GetDebugColor()) {
         sphere_ = sphere->GetSphereCollisionShape();
         UpdateSensor();
      }
   }
}

void RenderSensor::UpdateSensor()
{

   color_ = sensor_->GetDebugColor();
   math3d::point3 p0(0, 0.1f, 0), p1(0, 0.1f, 0);
   math3d::point3 center = sphere_.get_center();
   float r = sphere_.get_radius();

   h3dRadiantClearDebugShape(debugShape_);
   p0.x = (float)(sin(0) * r);
   p0.z = (float)(cos(0) * r);
   for (float t = 0; t < 3.14 * 2; t += 0.1f) {
      p1.x = (float)(sin(t) * r);
      p1.z = (float)(cos(t) * r);
      h3dRadiantAddDebugLine(debugShape_, center + p0, center + p1, color_);
      p0 = p1;
   }
   h3dRadiantCommitDebugShape(debugShape_);
}
