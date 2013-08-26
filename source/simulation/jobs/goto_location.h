#ifndef _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H
#define _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

#include "job.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class GotoLocation : public Task
{
public:
   GotoLocation(om::EntityRef entity, float speed, const csg::Point3f& location, float close_to_distance, luabind::object arrived_cb);
   GotoLocation(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance, luabind::object arrived_cb);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

public:
   bool Work(const platform::timer &timer) override;

protected:
   void Report(std::string msg);
   void Stop();

protected:
   om::EntityRef        entity_;
   om::EntityRef        target_entity_;
   csg::Point3f       target_location_;
   float                speed_;
   bool                 target_is_entity_;
   float                close_to_distance_;
   luabind::object      arrived_cb_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

