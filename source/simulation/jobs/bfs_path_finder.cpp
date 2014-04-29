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
#include "om/components/entity_container.ridl.h"
#include "om/components/destination.ridl.h"
#include "dm/map_trace.h"
#include "om/region.h"
#include "csg/color.h"
#include "csg/util.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define BFS_LOG(level)   LOG_CATEGORY(simulation.pathfinder.bfs, level, GetName())


std::vector<std::weak_ptr<BfsPathFinder>> BfsPathFinder::all_pathfinders_;

/*
 * -- SearchOrder
 *
 * The BfsPathFinder uses the searchOrder global table to determine the order to
 * check NavGrid tiles when expanding the search.  At app initialization time,
 * we use a constructor on a static to create the table.  The maximum length
 * of a BFS search is limited by the size of the table, which is
 * MAX_BFS_RADIUS * phys::TILE_SIZE world voxels.  At the time of this writing,
 * MAX_BFS_RADIUS is 64 and phys::TILE_SIZE is 16, which is 1024 voxels, or
 * almost 3/4 of a "game mile".   Far enough!
 *
 * Why bother?  It choosing which tile to explore next O(1) with an EXTREMELY
 * small constants factor.  Most of the time spend in the initial Bfs stages
 * is in aggressively growing the region (but stopping once we find something
 * to us the AStarPathFinder on), so it's important that choosing the next
 * tile be super, super fast.
 *
 */
struct SearchOrderEntry {
   float d;             // The distance, in voxels, when we should explore this tile.
   signed char x, y, z; // The tile address to explore
};

#define MAX_BFS_RADIUS 64 
#define SEARCH_ORDER_SIZE (MAX_BFS_RADIUS * MAX_BFS_RADIUS * MAX_BFS_RADIUS * 8)

SearchOrderEntry searchOrder[SEARCH_ORDER_SIZE];
struct ConstructSearchOrder {
   ConstructSearchOrder() {
      int i = 0;

      // Build the table and sort it by distance.
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

/*
 * -- BfsPathFinder::Create
 *
 * Create a BfsPathFinder.  We use a static method with a private constructor so we can keep track
 * of all BfsPathFinders in existence (for ComputeCounters)...
 *
 */

std::shared_ptr<BfsPathFinder> BfsPathFinder::Create(Simulation& sim, om::EntityPtr entity, std::string const& name, int range)
{
   std::shared_ptr<BfsPathFinder> pathfinder(new BfsPathFinder(sim, entity, name, range));
   all_pathfinders_.push_back(pathfinder);
   return pathfinder;
}


/*
 * -- BfsPathFinder::ComputeCounters
 *
 * Report performance metrics for the BFS pathfinders.  Not sure what's relevant yet, so do nothing.
 *
 */

void BfsPathFinder::ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter)
{
   NOT_YET_IMPLEMENTED();
}


/*
 * -- BfsPathFinder::BfsPathFinder
 *
 * Create a new BfsPathFinder.  `max_range` is the farthest distance the BfsPathFinder should look for
 * a solution, in voxels.
 *
 */

BfsPathFinder::BfsPathFinder(Simulation& sim, om::EntityPtr entity, std::string const& name, int max_range) :
   PathFinder(sim, name),
   entity_(entity),
   search_order_index_(-1),
   running_(false),
   max_travel_distance_(static_cast<float>(max_range))
{
   pathfinder_ = AStarPathFinder::Create(sim, BUILD_STRING(name << "(slave pathfinder)"), entity);
   BFS_LOG(3) << "creating bfs pathfinder";
}


/*
 * -- BfsPathFinder::~BfsPathFinder
 *
 * BfsPathFinder destructor.
 *
 */

BfsPathFinder::~BfsPathFinder()
{
   BFS_LOG(3) << "destroying bfs pathfinder";
}


/*
 * -- BfsPathFinder::SetSource
 *
 * Set the source location to start the search from.
 *
 */

BfsPathFinderPtr BfsPathFinder::SetSource(csg::Point3 const& location)
{
   pathfinder_->SetSource(location);
   return shared_from_this();
}


/*
 * -- BfsPathFinder::SetSolvedCb
 *
 * Register a function to be called when a solution is found.
 *
 */

BfsPathFinderPtr BfsPathFinder::SetSolvedCb(SolvedCb solved_cb)
{
   pathfinder_->SetSolvedCb(solved_cb);
   return shared_from_this();
}


/*
 * -- BfsPathFinder::SetSearchExhaustedCb
 *
 * Register a function to be called when the search distance exceeds the
 * maximum range without finding a solution.
 *
 */

BfsPathFinderPtr BfsPathFinder::SetSearchExhaustedCb(ExhaustedCb exhausted_cb)
{
   NOT_YET_IMPLEMENTED(); // no one yet fires exhausted_cb_
   exhausted_cb_ = exhausted_cb;
   return shared_from_this();
}


/*
 * -- BfsPathFinder::SetFilterFn
 *
 * Register a function to be called to determine if an Entity is a valid
 * destination for the search.  The BfsPathFinder will call `filter_fn` exactly
 * once for each entity it bumps into while expanding.
 *
 */

BfsPathFinderPtr BfsPathFinder::SetFilterFn(FilterFn filter_fn)
{
   filter_fn_ = filter_fn;
   return shared_from_this();
}


/*
 * -- BfsPathFinder::IsIdle
 *
 * Return whether or not there's work to do.
 *
 */

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

   // If we've completely run out of nodes to search, return true.  This can happen
   // when the maximum range specified in the constructor exceeds the pre-compiled
   // maximum.
   if (search_order_index_ >= SEARCH_ORDER_SIZE) {
      BFS_LOG(5) << "ran out of search order nodes.  returning IDLE.";
      return true;
   }

   // If our AStarPathFinder has stepped farther than the maximum travel distance, then
   // there must be no solution.
   if (travel_distance_ > max_travel_distance_) {
      BFS_LOG(5) << "travel distance" << travel_distance_ << " exceeds max range of " << max_travel_distance_ << ".  returning IDLE.";
      return true;
   }

   // If we haven't gone too far and there's still stuff to chew on, keep chewing!
   if (!pathfinder_->IsIdle()) {
      BFS_LOG(5) << "pathfinder still has work to do.  returning NOT IDLE.";
      return false;
   }

   // If we've explored farther than the maximum travel distance (plus some padding to
   // account for phys::TILE_SIZE), then there must be no solution.
   float max_explored_distanced = GetMaxExploredDistance();
   if (explored_distance_ > max_explored_distanced) {
      BFS_LOG(5) << "explored distance " << explored_distance_ << " exceeds max range of " << max_explored_distanced << ".  returning IDLE.";
      return true;
   }

   BFS_LOG(5) << "returning not IDLE";
   return false;
}


/*
 * -- BfsPathFinder::Start
 *
 * Start the BfsPathFinder (from scratch!)
 *
 */

BfsPathFinderPtr BfsPathFinder::Start()
{
   BFS_LOG(5) << "start requested";
   pathfinder_->Start();
   running_ = true;
   search_order_index_ = -1;
   explored_distance_ = 0;
   travel_distance_ = 0;
   
   om::EntityContainerPtr container = GetSim().GetRootEntity()->GetComponent<om::EntityContainer>();
   if (container) {
      // Suppose some item is created which is reachable, but inside the explored radius.  We
      // really should manually add that item to the pathfinder, too, right?  To get this 100%
      // correct, we'd need to trace every NavGridTile we've ever visited, which is ABSURDLY
      // expensive.  Almost as good as looking at Entities that come and go from the root object
      // (items placed on the terrain are contained by the root object).  Let's do that.
      entityAddedTrace_ = container->TraceChildren("bfs", dm::PATHFINDER_TRACES)
                           ->OnAdded([this](dm::ObjectId id, om::EntityRef e) {
                                 ConsiderAddedEntity(e.lock());
                              });
   }
   return shared_from_this();
}


/*
 * -- BfsPathFinder::Stop
 *
 * Stop searching. 
 *
 */

BfsPathFinderPtr BfsPathFinder::Stop()
{
   BFS_LOG(5) << "stop requested";
   pathfinder_->Stop();
   running_ = false;
   entityAddedTrace_= nullptr;
   return shared_from_this();
}


/*
 * -- BfsPathFinder::ReconsiderDestination
 *
 * Run an entity through the filter function again,  iff it's been through once before.
 * If the entity hasn't been through the filter, this is a nop.
 *
 */

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


/*
 * -- BfsPathFinder::ConsiderEntity
 *
 * Pass an entity through the filter function, and add it to the AStarPathFinder
 * if it passes.
 *
 */

void BfsPathFinder::ConsiderEntity(om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();
   visited_ids_.insert(id);

   if (filter_fn_(entity)) {
      BFS_LOG(7) << *entity << " passed the filter!  adding to slave pathfinder.";
      pathfinder_->AddDestination(entity);
   } else {
      BFS_LOG(7) << *entity << " failed the filter!!";
   }
}



/*
 * -- BfsPathFinder::ConsiderAddedEntity
 *
 * Consider and entity that was newly added to the terrain.  If that Entity's location
 * is inside the current search radius, manually add it to the pathfinder.
 *
 */

void BfsPathFinder::ConsiderAddedEntity(om::EntityPtr entity)
{
   if (entity) {
      if (pathfinder_->IsSolved()) {
         return;
      }

      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         csg::Point3 src = pathfinder_->GetSourceLocation();
         csg::Point3 dst = mob->GetWorldGridLocation();
         if (src.DistanceTo(dst) <= explored_distance_) {
            BFS_LOG(7) << "considering " << *entity << " newly added to root object";
            ConsiderEntity(entity);
         }
      }
   }
}

/*
 * -- BfsPathFinder::EncodeDebugShapes
 *
 * Send debug info over the wire.
 *
 */

void BfsPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   pathfinder_->EncodeDebugShapes(msg);
}


/*
 * -- BfsPathFinder::Work
 *
 * The workhorse.  Gets stuff done!
 *
 */

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


/*
 * -- BfsPathFinder::ExploreNextNode
 *
 * Expands the BFS search region by exactly one NavGridTile.  `search_order_index_`
 * keeps an index into the `searchOrder` static table, which contains the order
 * we should add tiles.
 *
 */

void BfsPathFinder::ExploreNextNode(csg::Point3 const& src_index)
{
   if (search_order_index_ < SEARCH_ORDER_SIZE) {
      SearchOrderEntry const& entry = searchOrder[++search_order_index_];

      explored_distance_ = searchOrder[search_order_index_].d;
      AddTileToSearch(src_index + csg::Point3(entry.x, entry.y, entry.z));
   }
}


/*
 * -- BfsPathFinder::ExpandSearch
 *
 * Expands the BFS search region until the AStarPathFinder finds something it can
 * chew on, the timer runs out, or we simply run out of nodes to explore!
 *
 */

void BfsPathFinder::ExpandSearch(platform::timer const &timer)
{
   csg::Point3 src = pathfinder_->GetSourceLocation();
   csg::Point3 src_index = src / phys::TILE_SIZE;
   float max_explored_distanced = GetMaxExploredDistance();

   // Work once to kick the pathfinder
   while (!timer.expired()) {
      if (pathfinder_->IsSolved()) {
         BFS_LOG(9) << "pathfinder found solution while expanding search";
         return;
      }

      // If we've walked farther than we're allowed to explore, add a new nodes to the
      // bfs search.  For example, if we're currently pursing an indirect path to an
      // entity, we need to make sure all the tiles that are *closer* than the length
      // of that path have been revealed so we can choose the (potentially closer) other
      // entities. 
      if (travel_distance_ > explored_distance_) {
         BFS_LOG(9) << "current travel distance " << travel_distance_ 
                    << " exceeds explored distance " << explored_distance_
                    << ".  exploring more nodes";
         ExploreNextNode(src_index);
         continue;
      }

      // If we've just explored too far and still don't have a solution, bail.
      // We only explore new nodes when the AStarPathfinder is idle or when the
      // path for the active AStarPathfinder exceeds the current explore distance,
      // so if `explored_distance_` is too large, we know there must not be a
      // solution.
      if (explored_distance_ > max_explored_distanced) {
         BFS_LOG(3) << "current explored distance " << explored_distance_ 
            << " exceeds max explored distance " << max_explored_distanced
            << ".  stopping search.";
         return;
      }

      // We may have added a new node since the last time we checked.  If that
      // caused the pathfinder to become active, stop expanding the search!
      if (!pathfinder_->IsIdle()) {
         BFS_LOG(5) << "slave pathfinder is not idle.  stopping search expansion.";
         return;
      }

      // If we just plain ol' run out of nodes, there's no way to expand.
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

/*
 * -- BfsPathFinder::AddTileToSearch
 *
 * Add a single NavGridTile to the search.  Iterates though the Entities in that
 * tile and considers them for adding to the AStarPathFinder.
 *
 */

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


/*
 * -- BfsPathFinder::EstimateCostToSolution
 *
 * Returns the estimated cost (in distance remaining to be traveled) to find a solution or FLT_MAX if
 * no solution exists.  If the AStarPathFinder hasn't found anything to chew on yet, we return
 * the current explored distance.  This most likely under-estimates the actual cost, but
 * has the nice effect of expanding all BfsPathFinders for an Entity at approximately the
 * same rate (and to the distance of other, individual AStarPathFinders).
 *
 */

float BfsPathFinder::EstimateCostToSolution()
{
   if (pathfinder_->IsSolved()) {
      BFS_LOG(3) << "solution found.  estimated cost of solution is 0";
      return 0;
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


/*
 * -- BfsPathFinder::GetProgress
 *
 * Returns a string describing the current bfs progress.
 *
 */

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


/*
 * -- BfsPathFinder::GetSolution
 *
 * Returns a solution, if one exists.
 *
 */

PathPtr BfsPathFinder::GetSolution() const
{
   return pathfinder_->GetSolution();
}

/*
 * -- operator<< for BfsPathFinder
 *
 * Not so interesting, but makes it show up nicely in Decoda
 *
 */

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const BfsPathFinder& pf)
{
   o << "[bfs " << pf.GetName() << "]";
   return o;        
}

/*
 * -- BfsPathFinder::GetSolution
 *
 * Returns the max distance we're allowed to explore.  This is just the max distance
 * we're allowed to travel, plus some fudge factor for the NavGridTile size.
 *
 */

float BfsPathFinder::GetMaxExploredDistance() const
{
   return max_travel_distance_ + phys::TILE_SIZE;
}
