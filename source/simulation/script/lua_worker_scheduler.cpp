#include "pch.h"
#include "math3d.h"
#include "lua_mob.h"
#include "lua_worker_scheduler.h"
#include "luabind/operator.hpp"
#include "simulation/simulation.h"
#include "simulation/scheduler/worker_scheduler.h"
#include "simulation/scheduler/building_scheduler.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;

std::ostream& operator<<(std::ostream& out, const LuaWorkerScheduler& a)
{
   out << "WorkerScheduler(" << a.GetDisplayName() << ")";
   return out;
}

void LuaWorkerScheduler::RegisterType(lua_State* L)
{
   module(L) [
      class_<LuaWorkerScheduler>("RadiantWorkerScheduler")
         .def(constructor<>())
         .def(constructor<int>())
         .def(tostring(self))
         .def("add_worker", &LuaWorkerScheduler::AddWorker)
         .def("remove_worker", &LuaWorkerScheduler::RemoveWorker)
   ];

   // luabind does not support vararg bindings, so just use the basic lua.
   //lua_register(L, "RadiantCreateWorkerScheduler", LuaWorkerScheduler::CreateLuaWorkerScheduler);
}

LuaWorkerScheduler::LuaWorkerScheduler() :
   type_("worker")
{
   scheduler_ = Simulation::GetInstance().GetWorkerScheduler();
}

LuaWorkerScheduler::LuaWorkerScheduler(int id) :
   type_("building ")
{
   scheduler_ = Simulation::GetInstance().GetBuildingScehduler(id);
   type_ += stdutil::ToString(id);
}

void LuaWorkerScheduler::AddWorker(LuaMob* actor)
{
   scheduler_->AddWorker(actor->GetObject());
}

void LuaWorkerScheduler::RemoveWorker(LuaMob* actor)
{
   scheduler_->RemoveWorker(actor->GetObject());
}
