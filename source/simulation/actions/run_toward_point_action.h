#ifndef _RADIANT_SIMULATION_RUN_TOWARD_POINT_ACTION_H
#define _RADIANT_SIMULATION_RUN_TOWARD_POINT_ACTION_H

#include "action.h"
#include "simulation/jobs/path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RunTowardPoint : public Action {
public:
   RunTowardPoint();

public:
   void Create(lua_State* L) override;
   void Update() override;

protected:
   om::EntityRef        self_;
   csg::Point3f       dst_;
};


END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_RUN_TOWARD_POINT_ACTION_H

