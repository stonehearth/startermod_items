#include "pch.h"
#include "path.h"
#include "core/static_string.h"
#include "a_star_path_finder.h"
#include "path_finder_src.h"
#include "path_finder_dst.h"
#include "simulation/simulation.h"
#include "lib/lua/script_host.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"
#include "csg/util.h"
#include "csg/iterators.h"
#include "movement_helpers.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

static const int DIRECT_PATH_SEARCH_COOLDOWN = 8;

std::vector<std::weak_ptr<AStarPathFinder>> AStarPathFinder::all_pathfinders_;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder.astar, level, GetName())

#if defined(CHECK_HEAPINESS)
#  define VERIFY_HEAPINESS() \
   do { \
      if (!rebuildHeap_) { \
         _Debug_heap(open_.begin(), open_.end(), bind(&AStarPathFinder::CompareEntries, this, _1, _2)); \
      } \
   } while (false);
#else
#  define VERIFY_HEAPINESS()
#endif

AStarPathFinderPtr AStarPathFinder::Create(Simulation& sim, std::string const& name, om::EntityPtr entity)
{
   AStarPathFinderPtr pathfinder(new AStarPathFinder(sim, name, entity));
   all_pathfinders_.push_back(pathfinder);
   return pathfinder;
}

void AStarPathFinder::ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter)
{
   int count = 0;
   int running_count = 0;
   int open_count = 0;
   int closed_count = 0;
   int active_count = 0;

   stdutil::ForEachPrune<AStarPathFinder>(all_pathfinders_, [&](AStarPathFinderPtr pf) {
      count++;
      if (pf->enabled_) {
         active_count++;
         if (!pf->CheckIfIdle()) {
            running_count++;
         }
      }
      open_count += pf->open_.size();
      closed_count += pf->closed_.size();
   });

   addCounter("pathfinders:a_star:total_count", count, "counter");
   addCounter("pathfinders:a_star:active_count", active_count, "counter");
   addCounter("pathfinders:a_star:running_count", running_count, "counter");
   addCounter("pathfinders:a_star:open_node_count", open_count, "counter");
   addCounter("pathfinders:a_star:closed_node_count", closed_count, "counter");
}

AStarPathFinder::AStarPathFinder(Simulation& sim, std::string const& name, om::EntityPtr entity) :
   PathFinder(sim, name),
   rebuildHeap_(false),
   restart_search_(true),
   search_exhausted_(false),
   enabled_(false),
   entity_(entity),
   world_changed_(false),
   direct_path_search_cooldown_(0),
   debug_color_(255, 192, 0, 128),
   _lastIdleCheckResult(nullptr),
   _rebuildOpenHeuristics(false)
{
   PF_LOG(3) << "creating pathfinder";

   auto changed_cb = [this](const char* reason) {
      RestartSearch(reason);
   };

   // Watch for dirty navgrid tiles so we can restart the search if necessary
   source_.reset(new PathFinderSrc(entity, GetName(), changed_cb));
}

AStarPathFinder::~AStarPathFinder()
{
   PF_LOG(3) << "destroying pathfinder";
}

AStarPathFinderPtr AStarPathFinder::SetSource(csg::Point3f const& location)
{
   source_->SetSourceOverride(location);
   RestartSearch("source location changed");
   return shared_from_this();
}

csg::Point3f AStarPathFinder::GetSourceLocation() const
{
   return source_->GetSourceLocation();
}

float AStarPathFinder::GetTravelDistance()
{
   if (open_.empty()) {
      return 0;
   }
   if (rebuildHeap_) {
      RebuildHeap();
   }
   return open_.front().g;
}

AStarPathFinderPtr AStarPathFinder::SetSolvedCb(SolvedCb const& solved_cb)
{
   solved_cb_ = solved_cb;
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::SetSearchExhaustedCb(ExhaustedCb const& exhausted_cb)
{
   exhausted_cb_ = exhausted_cb;
   return shared_from_this();
}

bool AStarPathFinder::IsIdle() const
{
   bool idle = CheckIfIdle();
   if (_lastIdleCheckResult) {
      PF_LOG(5) << _lastIdleCheckResult;
   }
   return idle;
}

bool AStarPathFinder::CheckIfIdle() const
{
   if (!enabled_) {
      _lastIdleCheckResult = "yes. not enabled";
      return true;
   }

   if (!source_ || source_->IsIdle()) {
      _lastIdleCheckResult = "yes. no source, or source is idle";
      return true;
   }

   if (entity_.expired()) {
      _lastIdleCheckResult = "yes. no source entity.";
      return true;
   }

   if (restart_search_) {
      _lastIdleCheckResult = "no. restart pending";
      return false;
   }

   if (IsSearchExhausted()) {
      _lastIdleCheckResult = "yes. search exhausted.";
      return true;
   }

   bool all_idle = true;
   for (const auto& entry : destinations_) {
      if (!entry.second->IsIdle()) {
         all_idle = false;
         break;
      }
   }
   if (all_idle) {
      _lastIdleCheckResult = "yes. all destinations idle.";
      return true;
   }


   if (solution_) {
      _lastIdleCheckResult = "yes. already solved!";
      return true;
   }

   _lastIdleCheckResult = "no. still work to do!";
   return false;
}

AStarPathFinderPtr AStarPathFinder::Start()
{
   PF_LOG(5) << "start requested";
   ASSERT(solved_cb_);
   enabled_ = true;
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::Stop()
{
   PF_LOG(5) << "stop requested";
   RestartSearch("pathfinder stopped");
   enabled_ = false;
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::AddDestination(om::EntityRef e)
{
   auto dstEntity = e.lock();
   if (!dstEntity) {
      PF_LOG(3) << "cannot add destination to pathfinder.  invalid entity.";
      return shared_from_this();
   }
   
   if (!om::IsInWorld(dstEntity)) {
      PF_LOG(3) << "cannot add " << *dstEntity << " to pathfinder.  not in the world.";
      return shared_from_this();
   }

   dm::ObjectId id = dstEntity->GetObjectId();
   if (stdutil::contains(destinations_, id)) {
      PF_LOG(3) << *dstEntity << " destination is already in pathfinder.  ignoring add request.";
      return shared_from_this();
   }

   PF_LOG(3) << "adding destination " << *dstEntity << ".";
   auto changed_cb = [this](PathFinderDst const& dst, const char* reason) {
      OnPathFinderDstChanged(dst, reason);
   };
   PathFinderDst* dst = new PathFinderDst(GetSim(), *this, entity_, dstEntity, GetName(), changed_cb);
   destinations_[id] = std::unique_ptr<PathFinderDst>(dst);
   OnPathFinderDstChanged(*dst, "added destination");
   if (LOG_IS_ENABLED(simulation.pathfinder.astar, 9)) {
      PF_LOG(9) << "destination list:";
      for (auto const& entry : destinations_) {
         PathFinderDst* dst = entry.second.get();
         PF_LOG(9) << "  destination " << entry.first << ":" << *dst->GetEntity();
      }
      PF_LOG(9) << "end destination list";
   }
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::RemoveDestination(dm::ObjectId id)
{
   PF_LOG(3) << "removing destination " << id << ".";
   auto i = destinations_.find(id);
   if (i != destinations_.end()) {
      destinations_.erase(i);
      if (solution_) {
         auto solution_entity = solution_->GetDestination().lock();
         if (solution_entity && solution_entity->GetObjectId() == id) {
            RestartSearch("destination removed");
         }
      } else {
         PF_LOG(3) << "recomputing open heuristics on destination removal";
         _rebuildOpenHeuristics = true;
      }
   }
   if (LOG_IS_ENABLED(simulation.pathfinder.astar, 9)) {
      PF_LOG(9) << "destination list:";
      for (auto const& entry : destinations_) {
         PathFinderDst* dst = entry.second.get();
         PF_LOG(9) << "  destination " << entry.first << ":" << *dst->GetEntity();
      }
      PF_LOG(9) << "end destination list";
   }
   return shared_from_this();
}

void AStarPathFinder::Restart()
{
   PF_LOG(5) << "restarting search";

   ASSERT(restart_search_);
   ASSERT(!solution_);

   _solutionNodes.clear();
   closed_.clear();
   closedBounds_ = csg::Cube3::zero;
   open_.clear();

   rebuildHeap_ = true;
   restart_search_ = false;

   world_changed_ = false;
   watching_tiles_.clear();
   EnableWorldWatcher(true);

   max_cost_to_destination_ = GetSim().GetOctTree().GetDistanceCost(csg::Point3::zero, csg::Point3::unitX) * 64;   

   source_->Start(open_);
   for (auto const& entry : destinations_) {
      entry.second->Start();
   }

   for (PathFinderNode& node : open_) {
      float h = EstimateCostToDestination(node.pt);
      node.f = h;
      node.g = 0;

      // by default search no more than 4x the distance
      if (h > 0) {
         max_cost_to_destination_ = std::max(max_cost_to_destination_, h * 4.0f);
      }
   }
}

void AStarPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   csg::Color4 pathColor;
   if (!solution_) {
      pathColor = csg::Color4(192, 32, 0);
   } else {
      pathColor = csg::Color4(32, 192, 0);
   }

   float min_h = FLT_MAX;
   float max_h = FLT_MIN;
   std::unordered_map<csg::Point3, float, csg::Point3::Hash> costs;
   for (const auto& node: open_) {
      float h = EstimateCostToDestination(node.pt);
      min_h = std::min(min_h, h);
      max_h = std::max(max_h, h);
      costs[node.pt] = h;
   }
   for (const auto& node: open_) {
      auto coord = msg->add_coords();
      int c = static_cast<int>(255 * costs[node.pt] / (max_h - min_h));

      node.pt.SaveValue(coord);
      csg::Color4(c, c, c, 255).SaveValue(coord->mutable_color());
   }

   for (const auto& pt: closed_) {
      auto coord = msg->add_coords();
      pt.SaveValue(coord);
      csg::Color4(0, 0, 64, 192).SaveValue(coord->mutable_color());
   }

   for (const auto& dst : destinations_) {
      dst.second->EncodeDebugShapes(msg, debug_color_);
   }
}

void AStarPathFinder::Work(const platform::timer &timer)
{
   MEASURE_JOB_TIME();

   for (int i = 0; i < 8 && !timer.expired(); i++) {
      PF_LOG(7) << "entering work function";

      if (restart_search_) {
         Restart();
      }

      if (open_.empty()) {
         PF_LOG(7) << "open set is empty!  returning";
         SetSearchExhausted();
         return;
      }

      if (_rebuildOpenHeuristics) {
         RebuildOpenHeuristics();
      }

      if (rebuildHeap_) {
         RebuildHeap();
      }

      VERIFY_HEAPINESS();
      PathFinderNode current = PopClosestOpenNode();
      if (closed_.empty()) {
         closedBounds_ = csg::Cube3(current.pt);
      } else {
         closedBounds_.Grow(current.pt);
      }
      closed_.insert(current.pt);
      WatchWorldPoint(current.pt);

      float mod = 1.0f + GetMaxMovementModifier(current.pt);

      PathFinderDst* closest = nullptr;
      float h = EstimateCostToDestination(current.pt, &closest, mod);

      if (h == 0) {
         // xxx: be careful!  this may end up being re-entrant (for example, removing
         // destinations).
         ASSERT(closest);

         std::vector<csg::Point3f> solution;
         ReconstructPath(solution, current);
         if (SolveSearch(solution, closest)) {
            return;
         }
         if (restart_search_) {
            PF_LOG(3) << "restarting search after failed attempt to dispatch solution!";
            Restart();
         }
      } 
      ASSERT(!restart_search_);

      // If there are no movement modifiers nearby (mod == 1.0), then we're clear to just
      // try to direct path-find to our destination.
      if (mod == 1.0 && closest && FindDirectPathToDestination(current, closest)) {
         // Found a solution!  Bail.
         return;
      }
      if (restart_search_) {
         PF_LOG(3) << "restarting search after failed attempt to dispatch direct path solution!";
         Restart();
      }
      ASSERT(!restart_search_);

      if (current.g + h > max_cost_to_destination_) {
         PF_LOG(3) << "max cost to destination " << max_cost_to_destination_ << " exceeded. marking search as exhausted.";
         SetSearchExhausted();
         return;
      }

      VERIFY_HEAPINESS();

      // Check each neighbor...
      const auto& o = GetSim().GetOctTree();
   
      VERIFY_HEAPINESS();

      o.ComputeNeighborMovementCost(entity_.lock(), current.pt, [this, &current](csg::Point3 const& to, float cost) {
         VERIFY_HEAPINESS();
         AddEdge(current, to, cost);
         VERIFY_HEAPINESS();
      });

      direct_path_search_cooldown_--;
   }
   VERIFY_HEAPINESS();
}

void AStarPathFinder::AddEdge(const PathFinderNode &current, const csg::Point3 &next, float movementCost)
{
   if (closed_.find(next) != closed_.end()) {
      PF_LOG(9) << "       Ignoring edge in closed set from " << current.pt << " to " << next << " cost:" << movementCost;
      return;
   }

   VERIFY_HEAPINESS();

   bool update = false;
   float g = current.g + movementCost;

   uint i;
   uint c = open_.size();
   for (i = 0; i < c; i++) {
      if (open_[i].pt == next) {
         break;
      }
   }

   if (i == c) {
      // not found in the open list. add a brand new node.
      float h = EstimateCostToDestination(next);
      float f = g + h;
      _solutionNodes.push_back(new PathFinderSolutionNode(next, current.solutionNode));
      PF_LOG(9) << "          Adding " << next << " to open set (f:" << f << " g:" << g << ").";
      open_.push_back(PathFinderNode(next, f, g, _solutionNodes.back()));
      if (!rebuildHeap_) {
         push_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
      }
   } else {
      // found. should we update?
      if (g < open_[i].g) {
         PathFinderNode &node = open_[i];

         float old_h = node.f - node.g;
         float old_g = node.g;
         float f = g + old_h;

         node.solutionNode->prev = current.solutionNode;
         node.g = g;
         node.f = f;
         rebuildHeap_ = true; // really?  what if it's "good enough" to not validate heap properties?
         PF_LOG(9) << "          Updated " << next << " in open set (f:" << f << " g:" << g << "old_g: " << old_g << ")";
      } else {
         PF_LOG(9) << "          Ignoring update to " << next << " to open set (g:" << g << " old_g:" << open_[i].g << ")";
      }
   }

   VERIFY_HEAPINESS();
}

float AStarPathFinder::EstimateCostToSolution()
{
   if (!enabled_) {
      return FLT_MAX;
   }
   if (restart_search_) {
      Restart();
   }
   if (_rebuildOpenHeuristics) {
      RebuildOpenHeuristics();
   }

   if (CheckIfIdle()) {
      return FLT_MAX;
   }
   return open_.front().f;
}

float AStarPathFinder::EstimateCostToDestination(const csg::Point3 &from) const
{
   ASSERT(!restart_search_);

   return EstimateCostToDestination(from, nullptr);
}

float AStarPathFinder::EstimateCostToDestination(const csg::Point3 &from, PathFinderDst** result, float maxMoveModifier) const
{
   float hMin = FLT_MAX;
   dm::ObjectId closestId = -1;
   PathFinderDst* closest = nullptr;

   ASSERT(!restart_search_);

   auto i = destinations_.begin();
   while (i != destinations_.end()) {
      dm::ObjectId entityId = i->first;
      PathFinderDst* dst = i->second.get();

      om::EntityPtr entity = dst->GetEntity();
      if (!entity) {
         i = destinations_.erase(i);
         continue;
      }
      ++i;

      float h = dst->EstimateMovementCost(csg::ToFloat(from));
      if (h < hMin) {
         closest = dst;
         closestId = entityId;
         hMin = h;
      }
   }
   if (hMin != FLT_MAX) {
      hMin = csg::Sqrt(hMin);
      hMin *= 1.25f;       // prefer faster over optimal...
      hMin /= maxMoveModifier;
   }
   PF_LOG(10) << "    EstimateCostToDestination returning (" << closestId << ") : " << hMin;
   if (result) {
      *result = closest;
   }
   return hMin;
}

PathFinderNode AStarPathFinder::PopClosestOpenNode()
{
   auto result = open_.front();

   for (auto const& entry : open_) {
      PF_LOG(10) << " open node " << entry.pt << " f:" << entry.f << " g:" << entry.g;
   }

   VERIFY_HEAPINESS();
   pop_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
   open_.pop_back();
   VERIFY_HEAPINESS();

   PF_LOG(10) << " PopClosestOpenNode returning " << result.pt << ". " << open_.size() << " points remain in open set";

   return result;
}

void AStarPathFinder::ReconstructPath(std::vector<csg::Point3f> &solution, const PathFinderNode &dst) const
{
   auto n = dst.solutionNode;

   while (n) {
      solution.push_back(csg::ToFloat(n->pt));
      n = n->prev;
   }

   // following convention, we include the origin point in the path
   // follow path can decide to skip this point so that we don't run to the origin of the current block first
   std::reverse(solution.begin(), solution.end());
}

void AStarPathFinder::RecommendBestPath(std::vector<csg::Point3f> &points) const
{
   points.clear();
   if (!open_.empty()) {
      if (rebuildHeap_) {
         const_cast<AStarPathFinder*>(this)->RebuildHeap();
      }
      ReconstructPath(points, open_.front());
   }
}

std::string AStarPathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   return BUILD_STRING("astar " << std::left << std::setw(40) << GetName() << "(" <<
                       "entity:" << std::setw(32) << ename << " " << 
                       "id:" << std::setw(3) << GetId() << " " << 
                       "src:" << source_->GetSourceLocation() << " " << 
                       "#dst:" << destinations_.size() << " " << 
                       "open:" << open_.size() << " " << 
                       "closed:" << closed_.size() << " " <<
                       "idle:\"" << std::setw(16) << (_lastIdleCheckResult ? _lastIdleCheckResult : "not checked") << "\" " <<
                       ")");
}

void AStarPathFinder::RebuildHeap()
{
   std::make_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
   VERIFY_HEAPINESS();
   rebuildHeap_ = false;
}

void AStarPathFinder::SetSearchExhausted()
{
   PF_LOG(7) << "trying to market search as exhausted!";

   if (world_changed_) {
      PF_LOG(5) << "restarting search after exhaustion: world changed during search";
      RestartSearch("world changed during search");
      return;
   }

   if (!search_exhausted_) {
      _solutionNodes.clear();
      closed_.clear();
      closedBounds_ = csg::Cube3::zero;
      open_.clear();
      search_exhausted_ = true;
      if (exhausted_cb_) {
         PF_LOG(5) << "calling lua search exhausted callback";
         exhausted_cb_();
         PF_LOG(5) << "finished calling lua search exhausted callback";
      }
   }
}

bool AStarPathFinder::SolveSearch(std::vector<csg::Point3f>& solution, PathFinderDst*& dst)
{
   int dstId = dst->GetEntityId();

   if (solution.empty()) {
      solution.push_back(source_->GetSourceLocation());
   }
   csg::Point3f dst_point_of_interest = dst->GetPointOfInterest(solution.back());
   PF_LOG(5) << "found solution to destination " << dst->GetEntityId() << " (last point is " << solution.back() << ")";
   PathPtr path = std::make_shared<Path>(solution, entity_.lock(), dst->GetEntity(), dst_point_of_interest);

   PF_LOG(5) << "calling lua solved callback";
   bool solved = solved_cb_(path);
   PF_LOG(5) << "finished calling lua solved callback";

   if (solved) {
      solution_  = path;
   } else {
      PF_LOG(5) << "solved cb said no bueno.  removing destination and continuing search.";
      RemoveDestination(dstId);
      dst = nullptr;
   }

   VERIFY_HEAPINESS();

   return solution_ != nullptr;
}

PathPtr AStarPathFinder::GetSolution() const
{
   return solution_;
}

AStarPathFinderPtr AStarPathFinder::RestartSearch(const char* reason)
{
   if (!restart_search_) {
      PF_LOG(3) << "requesting search restart: " << reason;
      restart_search_ = true;
      search_exhausted_ = false;
      solution_ = nullptr;
      _solutionNodes.clear();
      closed_.clear();
      closedBounds_ = csg::Cube3::zero;
      open_.clear();

      world_changed_ = false;
      watching_tiles_.clear();
      direct_path_search_cooldown_ = 0;

      EnableWorldWatcher(false);
   }   
   return shared_from_this();
}

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const AStarPathFinder& pf)
{
   return pf.Format(o);
}

std::ostream& AStarPathFinder::Format(std::ostream& o) const
{
   o << "[pf " << GetName() << "]";
   return o;        
}

bool AStarPathFinder::IsSearchExhausted() const
{
   return !restart_search_ && (open_.empty() || search_exhausted_);
}

bool AStarPathFinder::IsSolved() const
{
   return !restart_search_ && (solution_ != nullptr);
}

void AStarPathFinder::SetDebugColor(csg::Color4 const& color)
{
   debug_color_ = color;
}

void AStarPathFinder::WatchWorldRegion(csg::Region3f const& region)
{
   if (!navgrid_guard_.Empty()) {
      csg::Cube3 bounds = csg::ToInt(region.GetBounds());
      csg::Cube3 chunks = csg::GetChunkIndex<phys::TILE_SIZE>(bounds);
      for (csg::Point3 const& cursor : csg::EachPoint(chunks)) {
         WatchTile(cursor);
      }
   }
}

void AStarPathFinder::WatchWorldPoint(csg::Point3 const& pt)
{
   if (!navgrid_guard_.Empty()) {
      csg::Point3 index = csg::GetChunkIndex<phys::TILE_SIZE>(pt);
      WatchTile(index);
   }
}

void AStarPathFinder::WatchTile(csg::Point3 const& index)
{
   watching_tiles_.insert(index);
}

void AStarPathFinder::OnTileDirty(csg::Point3 const& index)
{
   if (watching_tiles_.find(index) != watching_tiles_.end()) {
      world_changed_ = true;
      EnableWorldWatcher(false);

      if (search_exhausted_) {
         RestartSearch("world changed after exhausted search");
      }
   }
}

void AStarPathFinder::EnableWorldWatcher(bool enabled)
{
   if (enabled) {
      world_changed_ = false;
      navgrid_guard_ = GetSim().GetOctTree().GetNavGrid().NotifyTileDirty([this](csg::Point3 const& pt) {
         OnTileDirty(pt);
      });
   } else {
      navgrid_guard_.Clear();
   }
}


// Find the max movement modifier in the region just around the supplied point.  We can use this
// to compute a better pathing heuristic, for example.
float AStarPathFinder::GetMaxMovementModifier(csg::Point3 const& wgl) const
{
   phys::OctTree& octTree = GetSim().GetOctTree();
   phys::NavGrid& ng = octTree.GetNavGrid();
   csg::Point3 index[4];
   csg::Point3 offset;
   csg::GetChunkIndex<phys::TILE_SIZE>(wgl, index[0], offset);

   int xOffset = offset.x >= phys::TILE_SIZE / 2 ? 1 : -1;
   int zOffset = offset.z >= phys::TILE_SIZE / 2 ? 1 : -1;

   csg::GetChunkIndex<phys::TILE_SIZE>(wgl + csg::Point3(0, 0, zOffset * phys::TILE_SIZE), index[1], offset);
   csg::GetChunkIndex<phys::TILE_SIZE>(wgl + csg::Point3(xOffset * phys::TILE_SIZE, 0, zOffset * phys::TILE_SIZE), index[2], offset);
   csg::GetChunkIndex<phys::TILE_SIZE>(wgl + csg::Point3(xOffset * phys::TILE_SIZE, 0, 0), index[3], offset);

   float maxmodifier = 0;
   for (int i = 0; i < 4; i++) {
      maxmodifier = std::max(maxmodifier, ng.GridTile(index[i]).GetMaxMovementModifier());
   }
   return maxmodifier;
} 


bool AStarPathFinder::FindDirectPathToDestination(PathFinderNode &start, PathFinderDst*& dst)
{   
   ASSERT(dst);

   if (direct_path_search_cooldown_ > 0) {
      return false;
   }
   direct_path_search_cooldown_ = DIRECT_PATH_SEARCH_COOLDOWN;

   MovementHelper mh(LOG_LEVEL(simulation.pathfinder.astar));
   om::EntityPtr entity = entity_.lock();

   csg::Point3f from = csg::ToFloat(start.pt);
   csg::Region3f const& dstRegion = dst->GetWorldSpaceAdjacentRegion();
   csg::Point3f to = dstRegion.GetClosestPoint(from);

   if (!mh.GetPathPoints(GetSim(), entity, false, from, to, _directPathCandiate)) {
      return false;
   }

   std::vector<csg::Point3f> solution;
   ReconstructPath(solution, start);
   
   auto begin = _directPathCandiate.begin();
   auto end = _directPathCandiate.end();

   if (!solution.empty() && begin != end && solution.back() == *begin) {
      // The point at the back of the solution is the same as the point at
      // the front of the direct path.  Skip the duplicate.
      ++begin;
   }
   solution.insert(solution.end(), begin, end);

   return SolveSearch(solution, dst);
}

void AStarPathFinder::OnPathFinderDstChanged(PathFinderDst const& dst, const char* reason)
{
   PF_LOG(7) << "path finder dst changed.  (worldRegion:" << dst.GetWorldSpaceAdjacentRegion().GetBounds() << " closedBounds:" << closedBounds_ << ")";

   if (!closed_.empty()) {
      if (dst.GetWorldSpaceAdjacentRegion().Intersects(csg::ToFloat(closedBounds_))) {
         // Too close.  Just restart
         RestartSearch("added dst inside closed bounding box");
         return;
      }
   }
   _rebuildOpenHeuristics = true;
}

void AStarPathFinder::RebuildOpenHeuristics()
{
   // The value of h for all search points has changed!  That doesn't
   // affect g, though. Go through the whole openset and re-compute
   // f based on the existing g and the new h.
   for (PathFinderNode& node : open_) {
      float h = EstimateCostToDestination(node.pt);      
      node.f = node.g + h;
   }
   _rebuildOpenHeuristics = false;

   // Changing all those f's around probably destroyed the heap property
   // of the open vector, so rebuild it.
   rebuildHeap_ = true;
}

om::EntityRef AStarPathFinder::GetEntity() const
{
   return entity_;
}
