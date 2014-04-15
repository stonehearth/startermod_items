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
   current_range_(-1),
   max_range_((range / phys::TILE_SIZE) + 1)
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
   if (current_range_ <= max_range_) {
      return false;
   }
   if (pathfinder_->IsSolved()) {
      return true;
   }
   return true;
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
   csg::Color4 boxColor(128, 128, 0, 192);
   for (auto const& cube : visited_) {
      protocol::box* box = msg->add_box();
      csg::Cube3f shape = csg::ToFloat(cube.Scaled(phys::TILE_SIZE));
      shape.min.SaveValue(box->mutable_minimum());
      shape.max.SaveValue(box->mutable_maximum());
      boxColor.SaveValue(box->mutable_color());
   }

   pathfinder_->EncodeDebugShapes(msg);
}

void BfsPathFinder::Work(platform::timer const &timer)
{
   BFS_LOG(7) << "entering work function "
              << "(unexplored area:" << unexplored_.GetArea() 
              << ", explored area:" << explored_.GetArea() << ")";

   // make sure the bounds of the search is at least as far out as the farthest
   // block we've already tried to find.
   csg::Point3 src = pathfinder_->GetSourceLocation();
   csg::Point3 farthest = pathfinder_->GetFarthestSearchPoint();
   ExpandVoxelRange(src, (int)std::ceil(src.DistanceTo(farthest)));

   // if there are any more blocks in the unexplored space, go ahead and add
   // their destinations now
   ExpandSearch(src, timer);

   if (!timer.expired() && !pathfinder_->IsIdle()) {
      ASSERT(!pathfinder_->IsIdle());
      pathfinder_->Work(timer);
      BFS_LOG(8) << pathfinder_->DescribeProgress();
   }
}

void BfsPathFinder::ExpandVoxelRange(csg::Point3 const& src, int range)
{
   ExpandTileRange(src / phys::TILE_SIZE, range / phys::TILE_SIZE);
}

void BfsPathFinder::ExpandTileRange(csg::Point3 const& src_index, int range)
{
   if (range > current_range_ && current_range_ <= max_range_) {
      BFS_LOG(3) << "increasing range to " << range << " tiles";
      current_range_ = range;

      // create a big cube that covers every block in the range
      csg::Cube3 bounds(csg::Point3(-range, -range, -range),
                        csg::Point3(range + 1, range + 1, range + 1));

      // offset it by the entity source, in tile coordinates.
      bounds.Translate(src_index);
      
      // remove the explored region and add it to the fringe.
      unexplored_ += csg::Region3(bounds) - explored_;
   }
}

void BfsPathFinder::ExpandSearch(csg::Point3 const& src, platform::timer const &timer)
{
   csg::Point3 src_index = src / phys::TILE_SIZE;

   while (!timer.expired()) {
      // Work once to kick the pathfinder 
      if (pathfinder_->IsSolved()) {
         BFS_LOG(9) << "pathfinder found solution while expanding search";
         break;
      }

      if (!pathfinder_->IsIdle()) {
         BFS_LOG(9) << "slave pathfinder is not idle.  stopping search expansion.";
         break;
      }

      // if there are more regions we could add to the search, go ahead and
      // add them now.
      if (!unexplored_.IsEmpty()) {
         csg::Point3 index = unexplored_.GetClosestPoint(src_index);
         BFS_LOG(7) << index << " is closest tile index in unexplored to " << src_index;
         AddTileToSearch(index);
         continue;
      }

      ASSERT(unexplored_.IsEmpty());

      // If the pathfinder is still idle even after adding all the tiles, we
      // need to expand the search
      BFS_LOG(7) << "pathfinder is still idle with no tiles left unexplored.  increasing range.";
      BFS_LOG(7) << "(slave pathfinder state: " << pathfinder_->DescribeProgress() << ")";
      ExpandTileRange(src_index, current_range_ + 1);
   }
}

void BfsPathFinder::AddTileToSearch(csg::Point3 const& index)
{
   ASSERT(unexplored_.Contains(index));
   ASSERT(!explored_.Contains(index));

   unexplored_ -= index;
   explored_.AddUnique(index);

   BFS_LOG(9) << "adding tile " << index << " to search.";

   bool visited = false;
   phys::NavGrid& ng = GetSim().GetOctTree().GetNavGrid();
   ng.ForEachEntityAtIndex(index, [this, &visited](om::EntityPtr entity) {
      visited = true;
      if (filter_fn_(entity)) {
         pathfinder_->AddDestination(entity);
      }
   });
#if defined(ENCODE_VISITED_NODES)
   if (visited) {
      visited_.push_back(csg::Cube3(index, index + csg::Point3::one));
   }
#endif
}

int BfsPathFinder::EstimateCostToSolution()
{
   if (pathfinder_->IsSolved()) {
      return INT_MAX;
   }
   if (!pathfinder_->IsIdle()) {
      return pathfinder_->EstimateCostToSolution();
   }
   if (unexplored_.IsEmpty() && current_range_ > max_range_) {
      return INT_MAX;
   }
   return 0;
}

std::string BfsPathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   return BUILD_STRING(GetName() << "bfs: " << pathfinder_->GetProgress());
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
