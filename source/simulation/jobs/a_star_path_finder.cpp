#include "pch.h"
#include "path.h"
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
         if (!pf->IsIdle()) {
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
   if (!enabled_) {
      PF_LOG(5) << "is_idle: yes.  not enabled";
      return true;
   }

   if (!source_ || source_->IsIdle()) {
      PF_LOG(5) << "is_idle: yes.  no source, or source is idle";
      return true;
   }

   if (entity_.expired()) {
      PF_LOG(5) << "is_idle: yes.  no source entity.";
      return true;
   }

   if (restart_search_) {
      PF_LOG(5) << "is_idle: no.  restart pending";
      return false;
   }

   if (IsSearchExhausted()) {
      PF_LOG(5) << "is_idle: yes. search exhausted.";
      return true;
   }

   // xxx: this is redundant with the multi path finder...
   bool all_idle = true;
   for (const auto& entry : destinations_) {
      if (!entry.second->IsIdle()) {
         all_idle = false;
         break;
      }
   }
   if (all_idle) {
      PF_LOG(5) << "is_idle: yes. all destinations idle.";
      return true;
   }


   if (solution_) {
      PF_LOG(5) << "is_idle: yes. already solved!";
      return true;
   }

   PF_LOG(5) << "is_idle: no.  still work to do! ( open:" << open_.size() << " closed:" << closed_.size() << ")";
   return false;
}

AStarPathFinderPtr AStarPathFinder::Start()
{
   PF_LOG(5) << "start requested";
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
   if (dstEntity) {
      if (om::IsInWorld(dstEntity)) {
         auto changed_cb = [this](PathFinderDst const& dst, const char* reason) {
            OnPathFinderDstChanged(dst, reason);
         };
         PathFinderDst* dst = new PathFinderDst(GetSim(), *this, entity_, dstEntity, GetName(), changed_cb);
         destinations_[dstEntity->GetObjectId()] = std::unique_ptr<PathFinderDst>(dst);
         OnPathFinderDstChanged(*dst, "added destination");
      } else {
         PF_LOG(3) << "cannot add " << dstEntity << " to pathfinder because it is not in the world.";
      }
   }
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::RemoveDestination(dm::ObjectId id)
{
   auto i = destinations_.find(id);
   if (i != destinations_.end()) {
      destinations_.erase(i);
      if (solution_) {
         auto solution_entity = solution_->GetDestination().lock();
         if (solution_entity && solution_entity->GetObjectId() == id) {
            RestartSearch("destination removed");
         }
      }
   }
   return shared_from_this();
}

void AStarPathFinder::Restart()
{
   PF_LOG(5) << "restarting search";

   ASSERT(restart_search_);
   ASSERT(!solution_);

   cameFrom_.clear();
   closed_.clear();
   open_.clear();

   rebuildHeap_ = true;
   restart_search_ = false;

   world_changed_ = false;
   watching_tiles_.clear();
   EnableWorldWatcher(true);

   max_cost_to_destination_ = GetSim().GetOctTree().GetMovementCost(csg::Point3::zero, csg::Point3::unitX) * 64;   

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
      closed_.insert(current.pt);
      WatchWorldPoint(current.pt);

      PathFinderDst* closest = nullptr;
      float h = EstimateCostToDestination(current.pt, &closest);

      if (h == 0) {
         // xxx: be careful!  this may end up being re-entrant (for example, removing
         // destinations).
         ASSERT(closest);

         std::vector<csg::Point3f> solution;
         ReconstructPath(solution, current.pt);
         SolveSearch(solution, *closest);
         return;
      } 

      if (closest && FindDirectPathToDestination(current.pt, *closest)) {
         // Found a solution!  Bail.
         return;
      }

      if (current.g + h > max_cost_to_destination_) {
         PF_LOG(3) << "max cost to destination " << max_cost_to_destination_ << " exceeded.  marking search as exhausted.";
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
   VERIFY_HEAPINESS();

   if (closed_.find(next) == closed_.end()) {
      bool update = false;
      float g = current.g + movementCost;

      uint i, c = open_.size();
      for (i = 0; i < c; i++) {
         if (open_[i].pt == next) {
            break;
         }
      }
      VERIFY_HEAPINESS();

      if (i == c) {
         float h = EstimateCostToDestination(next);
         float f = g + h;

         // not found in the open list.  add a brand new node.
         PF_LOG(9) << "          Adding " << next << " to open set (f:" << f << " g:" << g << ").";
         cameFrom_[next] = current.pt;
         open_.push_back(PathFinderNode(next, f, g));
         if (!rebuildHeap_) {
            push_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
         }
         VERIFY_HEAPINESS();
      } else {
         // found.  should we update?
         if (g < open_[i].g) {
            PathFinderNode &node = open_[i];

            float old_h = node.f - node.g;
            float old_g = node.g;
            float f = g + old_h;

            cameFrom_[next] = current.pt;
            node.g = g;
            node.f = f;
            rebuildHeap_ = true; // really?  what if it's "good enough" to not validate heap properties?
            PF_LOG(9) << "          Updated " << next << " in open set (f:" << f << " g:" << g << "old_g: " << old_g << ")";
         } else {
            PF_LOG(9) << "          Ignoring update to " << next << " to open set (g:" << g << " old_g:" << open_[i].g << ")";
         }
      }
   } else {
      PF_LOG(9) << "       Ignoring edge in closed set from " << current.pt << " to " << next << " cost:" << movementCost;
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

   if (IsIdle()) {
      return FLT_MAX;
   }
   return open_.front().f;
}

float AStarPathFinder::EstimateCostToDestination(const csg::Point3 &from) const
{
   ASSERT(!restart_search_);

   return EstimateCostToDestination(from, nullptr);
}

float AStarPathFinder::EstimateCostToDestination(const csg::Point3 &from, PathFinderDst** result) const
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

   PF_LOG(10) << " PopClosestOpenNode returning " << result.pt << ".  " << open_.size() << " points remain in open set";

   return result;
}

void AStarPathFinder::ReconstructPath(std::vector<csg::Point3f> &solution, const csg::Point3 &dst) const
{
   solution.push_back(csg::ToFloat(dst));

   auto i = cameFrom_.find(dst);
   while (i != cameFrom_.end()) {
      solution.push_back(csg::ToFloat(i->second));
      i = cameFrom_.find(i->second);
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
      ReconstructPath(points, open_.front().pt);
   }
}

std::string AStarPathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   return BUILD_STRING(GetName() << " " << ename << " (open: " << open_.size() << "  closed: " << closed_.size() << ")");
}

void AStarPathFinder::RebuildHeap()
{
   std::make_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
   VERIFY_HEAPINESS();
   rebuildHeap_ = false;
}

void AStarPathFinder::SetSearchExhausted()
{
   if (world_changed_) {
      PF_LOG(5) << "restarting search after exhaustion: world changed during search";
      RestartSearch("world changed during search");
      return;
   }

   if (!search_exhausted_) {
      cameFrom_.clear();
      closed_.clear();
      open_.clear();
      search_exhausted_ = true;
      if (exhausted_cb_) {
         PF_LOG(5) << "calling lua search exhausted callback";
         exhausted_cb_();
         PF_LOG(5) << "finished calling lua search exhausted callback";
      }
   }
}

void AStarPathFinder::SolveSearch(std::vector<csg::Point3f>& solution, PathFinderDst& dst)
{
   if (solution.empty()) {
      solution.push_back(source_->GetSourceLocation());
   }
   csg::Point3f dst_point_of_interest = dst.GetPointOfInterest(solution.back());
   PF_LOG(5) << "found solution to destination " << dst.GetEntityId() << " (last point is " << solution.back() << ")";
   solution_ = std::make_shared<Path>(solution, entity_.lock(), dst.GetEntity(), dst_point_of_interest);
   if (solved_cb_) {
      PF_LOG(5) << "calling lua solved callback";
      solved_cb_(solution_);
      PF_LOG(5) << "finished calling lua solved callback";
   }

   VERIFY_HEAPINESS();
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
      cameFrom_.clear();
      closed_.clear();
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

std::string AStarPathFinder::DescribeProgress()
{
   std::ostringstream progress;
   progress << GetName() << open_.size() << " open nodes. " << closed_.size() << " closed nodes. ";
   if (!open_.empty()) {
      progress << EstimateCostToDestination(open_.front().pt) << " nodes from destination. ";
   }
   progress << "idle? " << IsIdle();
   return progress.str();
}

void AStarPathFinder::WatchWorldRegion(csg::Region3f const& region)
{
   if (!navgrid_guard_.Empty()) {
      csg::Cube3 bounds = csg::ToInt(region.GetBounds());
      csg::Cube3 chunks = csg::GetChunkIndex<phys::TILE_SIZE>(bounds);
      for (csg::Point3 const& cursor : chunks) {
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

bool AStarPathFinder::FindDirectPathToDestination(csg::Point3 const& start, PathFinderDst &dst)
{   
   if (direct_path_search_cooldown_ > 0) {
      return false;
   }
   direct_path_search_cooldown_ = DIRECT_PATH_SEARCH_COOLDOWN;

   MovementHelper mh(LOG_LEVEL(simulation.pathfinder.astar));
   om::EntityPtr entity = entity_.lock();

   csg::Point3f from = csg::ToFloat(start);
   csg::Region3f const& dstRegion = dst.GetWorldSpaceAdjacentRegion();
   csg::Point3f to = dstRegion.GetClosestPoint(from);

   if (!mh.GetPathPoints(GetSim(), entity, false, from, to, _directPathCandiate)) {
      return false;
   }

   std::vector<csg::Point3f> solution;
   ReconstructPath(solution, start);
   solution.insert(solution.end(), _directPathCandiate.begin(), _directPathCandiate.end());
   SolveSearch(solution, dst);
   return true;
}

void AStarPathFinder::OnPathFinderDstChanged(PathFinderDst const& dst, const char* reason)
{
   if (false) {
      // the dst contains a closed node
      RestartSearch(reason);
      return;
   }
   _rebuildOpenHeuristics = true;
}

void AStarPathFinder::RebuildOpenHeuristics()
{
   // The value of h for all search points has changed!  That doesn't
   // affect g, though.  Go through the whole openset and re-compute
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


