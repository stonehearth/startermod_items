#ifndef _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H
#define _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

#include "lib/lua/bind.h"
#include "task.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class FollowPath : public Task
{
public:
   FollowPath(Simulation& sim, om::EntityRef entity, float speed, std::shared_ptr<Path> path, float stop_distance, luabind::object arrived_cb);
   virtual ~FollowPath();

public:
   bool Work(const platform::timer &timer) override;

public:
   void Stop();

protected:
   int CalculateStopIndex(csg::Point3f const& startLocation, std::vector<csg::Point3> const& points, csg::Point3 const& pointOfInterest, float stopDistance) const;
   bool Arrived(om::MobPtr mob);
   bool Obstructed();
   void Report(std::string const& msg);

protected:
   om::EntityRef           entity_;
   std::shared_ptr<Path>   path_;
   float                   speed_;
   int                     pursuing_;
   int                     stopIndex_;
   float                   stopDistance_;
   luabind::object         arrived_cb_;
};

std::ostream& operator<<(std::ostream& os, FollowPath const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_FOLLOW_PATH_JOB_H

