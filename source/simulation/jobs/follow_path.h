#ifndef _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H
#define _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

#include "lib/lua/bind.h"
#include "task.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class FollowPath : public Task
{
public:
   FollowPath(Simulation& sim, om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object arrived_cb);
   virtual ~FollowPath();

public:
   bool Work(const platform::timer &timer) override;

public:
   void Stop();

protected:
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

std::ostream& operator<<(std::ostream& os, FollowPath const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

