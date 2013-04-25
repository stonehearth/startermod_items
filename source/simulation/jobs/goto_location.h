#ifndef _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H
#define _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

#include "job.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class GotoLocation : public Job
{
public:
   GotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance);
   GotoLocation(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

public:
   bool IsIdle(int now) const override;
   bool IsFinished() const override { return finished_; }
   void Work(int now, const platform::timer &timer) override;

protected:
   void Report(std::string msg);
   void Stop();

protected:
   om::EntityRef        entity_;
   om::EntityRef        target_entity_;
   math3d::point3       target_location_;
   float                speed_;
   bool                 target_is_entity_;
   bool                 finished_;
   int                  lateUpdateTime_;
   float                close_to_distance_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_GOTO_LOCATION_JOB_H

