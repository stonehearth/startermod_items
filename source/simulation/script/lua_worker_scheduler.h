#ifndef _RADIANT_SIMULATION_LUA_WORKER_SCHEDULER_H
#define _RADIANT_SIMULATION_LUA_WORKER_SCHEDULER_H

#include "namespace.h"
#include "luabind/luabind.hpp"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Scheduler;

class LuaWorkerScheduler
{
public:
   LuaWorkerScheduler();
   LuaWorkerScheduler(int objectId);

   static void RegisterType(lua_State* L);
   std::string GetDisplayName() const { return type_; }

private:
   void AddWorker(LuaMob* actor);
   void RemoveWorker(LuaMob* actor);

private:
   Scheduler      *scheduler_;
   std::string    type_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_WORKER_SCHEDULER_H