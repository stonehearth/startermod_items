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
   void SetSpeed(float speed);
   bool Work(const platform::timer &timer) override;
   bool Arrived() const;
   void Stop();

protected:
   int CalculateStartIndex(csg::Point3 const& startGridLocation) const;
   int CalculateStopIndex(csg::Point3f const& startLocation, std::vector<csg::Point3> const& points, csg::Point3 const& pointOfInterest, float stopDistance) const;
   bool Obstructed() const;
   void Report(std::string const& msg) const;

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

