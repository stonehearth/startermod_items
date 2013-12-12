#ifndef _RADIANT_SIMULATION_JOBS_LUA_JOB_H
#define _RADIANT_SIMULATION_JOBS_LUA_JOB_H

#include "radiant_macros.h"
#include "lib/lua/bind.h"
#include "job.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class LuaJob: public Job {
   public:
      LuaJob(Simulation& sim, std::string const& name, luabind::object cb);
      virtual ~LuaJob();

   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override;
      void Work(const platform::timer &timer) override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      bool                 finished_;
      luabind::object      cb_;
};

DECLARE_SHARED_POINTER_TYPES(LuaJob)

std::ostream& operator<<(std::ostream& o, const LuaJob& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_LUA_JOB_H

