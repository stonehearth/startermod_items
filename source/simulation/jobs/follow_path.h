#ifndef _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H
#define _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

#include "radiant_luabind.h"
#include "job.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class FollowPath : public Task
{
public:
   FollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object arrived_cb);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

public:
   bool Work(const platform::timer &timer) override;

protected:
   void Stop();
   bool Arrived(om::MobPtr mob);
   bool Obstructed();
   void Report(std::string msg);

protected:
   om::EntityRef           entity_;
   std::shared_ptr<Path>   path_;
   float                   speed_;
   int                     pursuing_;
   float                   close_to_distance_;
   luabind::object         arrived_cb_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

