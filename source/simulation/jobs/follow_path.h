#ifndef _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H
#define _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

#include "radiant_luabind.h"
#include "job.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class FollowPath : public Job
{
public:
   FollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

public:
   bool IsIdle(int now) const override;
   bool IsFinished() const override { return finished_; }
   void Work(int now, const platform::timer &timer) override;

protected:
   void Stop();
   bool Arrived(om::MobPtr mob);
   bool Obstructed();
   void Report(std::string msg);

protected:
   om::EntityRef        entity_;
   std::shared_ptr<Path>     path_;
   float                speed_;
   int                  pursuing_;
   bool                 finished_;
   int                  lateUpdateTime_;
   float                close_to_distance_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

