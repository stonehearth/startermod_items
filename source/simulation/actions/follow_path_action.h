#ifndef _RADIANT_SIMULATION_FOLLOW_PATH_ACTION_H
#define _RADIANT_SIMULATION_FOLLOW_PATH_ACTION_H

#include "action.h"
#include "simulation/jobs/path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class FollowPathAction {
public:
   FollowPathAction(om::EntityRef entity, std::shared_ptr<Path> path);

public:
   void Create(lua_State* L) override;
   void Start() override;
   void Stop() override;
   void Update() override;

protected:
   bool FollowPath();
   bool Arrived();
   bool Obstructed();

protected:
   om::EntityRef        self_;
   std::shared_ptr<Path>     path_;
   int                  pursuing_;
   om::MobPtr           mob_;
   std::string          movingEffect_;
};


END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_FOLLOW_PATH_ACTION_H

