#include "pch.h"
#include "path.h"
#include "a_star_path_finder.h"
#include "bfs_path_finder.h"
#include "simulation/simulation.h"
#include "lib/lua/script_host.h"
#include "physics/octtree.h"
#include "physics/nav_grid.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"
#include "csg/util.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define BFS_LOG(level)   LOG_CATEGORY(simulation.pathfinder.bfs, level, GetName())

std::vector<std::weak_ptr<BfsPathFinder>> BfsPathFinder::all_pathfinders_;

#define MAX_BFS_RADIUS 64
#define SEARCH_ORDER_SIZE (MAX_BFS_RADIUS * MAX_BFS_RADIUS * MAX_BFS_RADIUS * 8)

struct SearchOrderEntry { float d; signed char x, y, z; } searchOrder[SEARCH_ORDER_SIZE];
struct ConstructSearchOrder {
   ConstructSearchOrder() {
      int i = 0;
      for (int x = -MAX_BFS_RADIUS; x < MAX_BFS_RADIUS; x++) {
         for (int y = -MAX_BFS_RADIUS; y < MAX_BFS_RADIUS; y++) {
            for (int z = -MAX_BFS_RADIUS; z < MAX_BFS_RADIUS; z++) {
               SearchOrderEntry &entry = searchOrder[i++];
               entry.x = x;
               entry.y = y;
               entry.z = z;
               entry.d = std::sqrt(static_cast<float>(x*x + y*y + z*z)) * phys::TILE_SIZE;
            }
         }
      }
      ASSERT(i == SEARCH_ORDER_SIZE);
      std::sort(searchOrder, searchOrder + SEARCH_ORDER_SIZE, [](SearchOrderEntry const& lhs, SearchOrderEntry const& rhs) {
         return lhs.d < rhs.d;
      });
   }
};
static ConstructSearchOrder __init;

std::shared_ptr<BfsPathFinder> BfsPathFinder::Create(Simulation& sim, om::EntityPtr entity, std::string name, int range)
{
   std::shared_ptr<BfsPathFinder> pathfinder(new BfsPathFinder(sim, entity, name, range));
   all_pathfinders_.push_back(pathfinder);
   return pathfinder;
}

void BfsPathFinder::ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter)
{
}

BfsPathFinder::BfsPathFinder(Simulation& sim, om::EntityPtr entity, std::string const& name, int range) :
   PathFinder(sim, name),
   entity_(entity),
   search_order_index_(-1),
   running_(false),
   max_travel_distance_(static_cast<float>(range))
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
   pathfinder_->SetSolvedCb(solved_cb);
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
   if (!running_) {
      BFS_LOG(5) << "not running.  returning IDLE.";
      return true;
   }

   if (pathfinder_->IsSolved()) {
      BFS_LOG(5) << "solved!  returning IDLE.";
      return true;
   }

   if (search_order_index_ >= SEARCH_ORDER_SIZE) {
      BFS_LOG(5) << "ran out of search order nodes.  returning IDLE.";
      return true;
   }

   if (travel_distance_ > max_travel_distance_) {
      BFS_LOG(5) << "travel distance" << travel_distance_ << " exceeds max range of " << max_travel_distance_ << ".  returning IDLE.";
      return true;
   }

   float max_explored_distanced = GetMaxExploredDistance();
   if (explored_distance_ > max_explored_distanced) {
      BFS_LOG(5) << "explored distance " << explored_distance_ << " exceeds max range of " << max_explored_distanced << ".  returning IDLE.";
      return true;
   }

   BFS_LOG(5) << "returning not IDLE";
   return false;
}

BfsPathFinderPtr BfsPathFinder::Start()
{
   BFS_LOG(5) << "start requested";
   pathfinder_->Start();
   running_ = true;
   search_order_index_ = -1;
   explored_distance_ = 0;
   travel_distance_ = 0;
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::Stop()
{
   BFS_LOG(5) << "stop requested";
   pathfinder_->Stop();
   running_ = false;
   return shared_from_this();
}

BfsPathFinderPtr BfsPathFinder::ReconsiderDestination(om::EntityRef e)
{
   if (running_) {
      om::EntityPtr entity = e.lock();
      if (entity) {
         dm::ObjectId id = entity->GetObjectId();

         // we can only *RE*-consider something if it's already been considered.  Otherwise,
         // wait for the BFS to find it organically.
         if (visited_ids_.find(id) != visited_ids_.end()) {
            BFS_LOG(3) << "reconsidering visited destination " << *entity;
            ConsiderEntity(e.lock());
         }
      }
   }
   return shared_from_this();
}

void BfsPathFinder::ConsiderEntity(om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();
   visited_ids_.insert(id);

   static int i = 1;
   BFS_LOG(3) << "   count:" << i++ << " calling filter on entity " << *entity;
   if (filter_fn_(entity)) {
      BFS_LOG(7) << *entity << " passed the filter!  adding to slave pathfinder.";
      pathfinder_->AddDestination(entity);
   }
}

void BfsPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   pathfinder_->EncodeDebugShapes(msg);
}

void BfsPathFinder::Work(platform::timer const &timer)
{
   BFS_LOG(3) << "entering work function ("
              << " travel distance:" << travel_distance_
              << " explored distance:" << explored_distance_
              << " search order index:" << search_order_index_
              << ")";

   if (running_) {
      ExpandSearch(timer);
      if (!timer.expired() && !pathfinder_->IsIdle()) {
         pathfinder_->Work(timer);
         travel_distance_ = pathfinder_->GetTravelDistance();
         BFS_LOG(9) << "setting travel distance to " << travel_distance_;
      }
   }
   BFS_LOG(3) << "exiting work function";
}

void BfsPathFinder::ExploreNextNode(csg::Point3 const& src_index)
{
   if (search_order_index_ < SEARCH_ORDER_SIZE) {
      SearchOrderEntry const& entry = searchOrder[++search_order_index_];

      explored_distance_ = searchOrder[search_order_index_].d;
      AddTileToSearch(src_index + csg::Point3(entry.x, entry.y, entry.z));
   }
}

void BfsPathFinder::ExpandSearch(platform::timer const &timer)
{
   float max_explored_distanced = GetMaxExploredDistance();

   // Work once to kick the pathfinder
   while (!timer.expired()) {
      if (pathfinder_->IsSolved()) {
         BFS_LOG(9) << "pathfinder found solution while expanding search";
         return;
      }
      csg::Point3 src = pathfinder_->GetSourceLocation();
      csg::Point3 src_index = src / phys::TILE_SIZE;

      if (travel_distance_ > explored_distance_) {
         BFS_LOG(9) << "current travel distance " << travel_distance_ 
                    << " exceeds explored distance " << explored_distance_
                    << ".  exploring more nodes";
         ExploreNextNode(src_index);
         continue;
      }

      if (explored_distance_ > max_explored_distanced) {
         BFS_LOG(3) << "current explored distance " << explored_distance_ 
            << " exceeds max explored distance " << max_explored_distanced
            << ".  stopping search.";
         return;
      }

      if (!pathfinder_->IsIdle()) {
         BFS_LOG(5) << "slave pathfinder is not idle.  stopping search expansion.";
         return;
      }

      if (search_order_index_ >= SEARCH_ORDER_SIZE) {
         BFS_LOG(3) << "ran out of search order nodes while expanding search";
         return;
      }

      // If the pathfinder is still idle even after adding all the tiles, we
      // need to expand the search
      BFS_LOG(7) << "pathfinder is still idle with no tiles left unexplored.  increasing range.";
      ExploreNextNode(src_index);
      BFS_LOG(7) << "explored distance is now " << explored_distance_ << "(max: " << max_explored_distanced << ")";
   }
}

void BfsPathFinder::AddTileToSearch(csg::Point3 const& index)
{
   BFS_LOG(9) << "adding tile " << index << " to search.";

   phys::NavGrid& ng = GetSim().GetOctTree().GetNavGrid();
   ng.ForEachEntityAtIndex(index, [this](om::EntityPtr entity) {
      dm::ObjectId id = entity->GetObjectId();
      if (visited_ids_.find(id) == visited_ids_.end()) {
         ConsiderEntity(entity);
      } else {
         BFS_LOG(5) << "already visited entity " << *entity << ".  ignoring.";
      }
   });
}

float BfsPathFinder::EstimateCostToSolution()
{
   if (pathfinder_->IsSolved()) {
      BFS_LOG(3) << "solution found.  estimated cost of solution is INT_MAX";
      return FLT_MAX;
   }
   if (!pathfinder_->IsIdle()) {
      float cost = pathfinder_->EstimateCostToSolution();
      BFS_LOG(3) << "pathfinder is running.  estimated cost of solution is " << cost;
      return cost;
   }
   if (travel_distance_ > max_travel_distance_) {
      BFS_LOG(3) << "bfs exceeded range " << max_travel_distance_ * phys::TILE_SIZE << ".  estimated max cost is INT_MAX";
      return FLT_MAX;
   }
   BFS_LOG(3) << "bfs still expanding.  estimated max cost is " << explored_distance_;
   return explored_distance_;
}

std::string BfsPathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   float percentExplored = search_order_index_ * 100.0f / SEARCH_ORDER_SIZE;
   return BUILD_STRING(GetName() << "bfs (" << percentExplored << "%): " << pathfinder_->GetProgress());
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

float BfsPathFinder::GetMaxExploredDistance() const
{
   return max_travel_distance_ + phys::TILE_SIZE;
}