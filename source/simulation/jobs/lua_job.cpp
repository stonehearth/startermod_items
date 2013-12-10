#include "pch.h"
#include "lua_job.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;

std::ostream& simulation::operator<<(std::ostream& o, const LuaJob& pf)
{
   return o << "[LuaJob " << pf.GetId() << " " << pf.GetName() << "]";
}

LuaJob::LuaJob(Simulation& sim, std::string const& name, object cb) :
   Job(sim, name),
   finished_(false),
   cb_(cb)
{
}

LuaJob::~LuaJob()
{
}

bool LuaJob::IsIdle() const
{
   return finished_;
}

bool LuaJob::IsFinished() const
{
   return finished_;
}

void LuaJob::Work(const platform::timer &timer)
{
   try {
      finished_ = !call_function<bool>(cb_);
   } catch (std::exception const& e) {
      LUA_LOG(1) << "lua job worker thread error: " << e.what() << ".  killing thread";
      finished_ = true;
   }
}

void LuaJob::LogProgress(std::ostream&) const
{
}

void LuaJob::EncodeDebugShapes(protocol::shapelist *msg) const
{
}

