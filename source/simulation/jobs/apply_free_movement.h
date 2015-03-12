#ifndef _RADIANT_SIMULATION_APPLY_FREE_MOTION_TASK_H
#define _RADIANT_SIMULATION_APPLY_FREE_MOTION_TASK_H

#include "lib/lua/bind.h"
#include "task.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class ApplyFreeMotionTask : public Task
{
public:
   ApplyFreeMotionTask(om::MobRef mob);
   virtual ~ApplyFreeMotionTask();

public:
   bool Work(const platform::timer &timer) override;

public:
   void Stop();

protected:
   om::MobRef           mob_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_APPLY_FREE_MOTION_TASK_H

