#ifndef _RADIANT_OM_SENSOR_LIST_H
#define _RADIANT_OM_SENSOR_LIST_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class SensorList;

class Sensor : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(Sensor, sensor);

   om::EntityRef GetEntity() const { return *entity_; }
   void SetEntity(om::EntityRef e) { entity_ = e; }

   std::string GetName() const { return *name_; }
   void SetName(std::string name) { name_ = name; }

   void SetCube(const csg::Cube3& cube) { cube_ = cube; }
   const csg::Cube3& GetCube() const { return *cube_; }
   const dm::Set<EntityId>& GetContents() const { return contains_; }

   void UpdateIntersection(std::vector<EntityId> intersection);

protected:
   friend SensorList;

   void Construct(om::EntityRef entity, std::string name, const csg::Cube3& cube) {
      entity_ = entity;
      name_ = name;
      cube_ = cube;
   }


private:
   void InitializeRecordFields() override {
      AddRecordField("entity",   entity_);
      AddRecordField("name",     name_);
      AddRecordField("cube",     cube_);
      AddRecordField("contains", contains_);
   }

private:
   dm::Boxed<om::EntityRef>   entity_;
   dm::Boxed<csg::Cube3>      cube_;
   dm::Boxed<std::string>     name_;
   dm::Set<EntityId>          contains_;
};
std::ostream& operator<<(std::ostream& os, const Sensor& o);

typedef std::shared_ptr<Sensor> SensorPtr;

class SensorList : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(SensorList, sensor_list);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   const dm::Map<std::string, SensorPtr>& GetSensors() const { return sensors_; }

   SensorPtr GetSensor(std::string name);
   SensorPtr AddSensor(std::string name, int radius);
   void RemoveSensor(std::string name);

private:
   void InitializeRecordFields() override;

public:
   dm::Map<std::string, SensorPtr>    sensors_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_SENSOR_LIST_H
