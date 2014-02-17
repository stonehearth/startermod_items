#ifndef _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H
#define _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

#include "task.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class GotoLocation : public Task
{
public:
   GotoLocation(Simulation& sim, om::EntityRef entity, float speed, const csg::Point3f& location, float close_to_distance, luabind::object arrived_cb);
   GotoLocation(Simulation& sim, om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance, luabind::object arrived_cb);

public:
   bool Work(const platform::timer &timer) override;

public:
   void Stop();

protected:
   void Report(std::string msg);
   bool GotoLocation::GetStandableLocation(std::shared_ptr<om::Entity> const& entity, csg::Point3f& desiredLocation, csg::Point3f& actualLocation);

protected:
   om::EntityRef        entity_;
   om::EntityRef        target_entity_;
   csg::Point3f         target_location_;
   float                speed_;
   bool                 target_is_entity_;
   float                close_to_distance_;
   luabind::object      arrived_cb_;
};

std::ostream& operator<<(std::ostream& os, GotoLocation const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

