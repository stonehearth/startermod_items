#include "pch.h"
#include "path.h"
#include "a_star_path_finder.h"
#include "bfs_path_finder.h"
#include "simulation/simulation.h"
#include "lib/lua/script_host.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define BFS_LOG(level)   LOG_CATEGORY(simulation.bfs_pathfinder, level, GetName())

std::vector<std::weak_ptr<BfsPathFinder>> BfsPathFinder::all_pathfinders_;

std::shared_ptr<BfsPathFinder> BfsPathFinder::Create(Simulation& sim, std::string name, om::EntityPtr entity)
{
   std::shared_ptr<BfsPathFinder> pathfinder(new BfsPathFinder(sim, name, entity));
   all_pathfinders_.push_back(pathfinder);
   return pathfinder;
}

void BfsPathFinder::ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter)
{
}

BfsPathFinder::BfsPathFinder(Simulation& sim, std::string name, om::EntityPtr entity) :
   PathFinder(sim, name),
   entity_(entity)
{
   pathfinder_ = AStarPathFinder::Create(sim, BUILD_STRING(name << "(slave pathfinder)"), entity);
   BFS_LOG(3) << "creating bfs pathfinder";
}

BfsPathFinder::~BfsPathFinder()
{
   BFS_LOG(3) << "destroying bfs pathfinder";
}

BfsPathFinderPtr BfsPathFinder::SetSource(csg::Point3 const& location)
{
   pathfinder_->SetSource(location);
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::SetSolvedCb(SolvedCb solved_cb)
{
   solved_cb_ = solved_cb;
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::SetSearchExhaustedCb(ExhaustedCb exhausted_cb)
{
   exhausted_cb_ = exhausted_cb;
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::SetFilterFn(FilterFn filter_fn)
{
   filter_fn_ = filter_fn;
   return shared_from_this();
}

bool BfsPathFinder::IsIdle() const
{
   return false;
}

BfsPathFinderPtr BfsPathFinder::Start()
{
   BFS_LOG(5) << "start requested";
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::Stop()
{
   BFS_LOG(5) << "stop requested";
   return shared_from_this();
}

void BfsPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
}

void BfsPathFinder::Work(const platform::timer &timer)
{
   BFS_LOG(7) << "entering work function";
}

int BfsPathFinder::EstimateCostToSolution()
{
   return 0;
}

std::string BfsPathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   return BUILD_STRING(GetName() << " bfs in progress...");
}

PathPtr BfsPathFinder::GetSolution() const
{
   return solution_;
}

BfsPathFinderPtr BfsPathFinder::RestartSearch(const char* reason)
{
   BFS_LOG(3) << "requesting search restart: " << reason;
   return shared_from_this();
}

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const BfsPathFinder& pf)
{
   return pf.Format(o);
}

std::ostream& BfsPathFinder::Format(std::ostream& o) const
{
   o << "[bfs " << GetName() << "]";
   return o;        
}

bool BfsPathFinder::IsSearchExhausted() const
{
   return false;
}

std::string BfsPathFinder::DescribeProgress()
{
   return BUILD_STRING(GetName() << " bfs progress...");
}
