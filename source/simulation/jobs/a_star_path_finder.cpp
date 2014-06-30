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

using namespace ::radiant;
using namespace ::radiant::simulation;

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
   debug_color_(255, 192, 0, 128)
{
   PF_LOG(3) << "creating pathfinder";

   auto changed_cb = [this](const char* reason) {
      RestartSearch(reason);
   };
   source_.reset(new PathFinderSrc(entity, GetName(), changed_cb));
}

AStarPathFinder::~AStarPathFinder()
{
   PF_LOG(3) << "destroying pathfinder";
}

AStarPathFinderPtr AStarPathFinder::SetSource(csg::Point3 const& location)
{
   source_->SetSourceOverride(location);
   RestartSearch("source location changed");
   return shared_from_this();
}

csg::Point3 AStarPathFinder::GetSourceLocation() const
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

AStarPathFinderPtr AStarPathFinder::SetSolvedCb(SolvedCb solved_cb)
{
   solved_cb_ = solved_cb;
   return shared_from_this();
}

AStarPathFinderPtr AStarPathFinder::SetSearchExhaustedCb(ExhaustedCb exhausted_cb)
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
      auto changed_cb = [this](const char* reason) {
         RestartSearch(reason);
      };
      destinations_[dstEntity->GetObjectId()] = std::unique_ptr<PathFinderDst>(new PathFinderDst(GetSim(), entity_, dstEntity, GetName(), changed_cb));
      RestartSearch("added destination");
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
   max_cost_to_destination_ = 0;

   source_->InitializeOpenSet(open_);
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
   std::vector<csg::Point3> best;
   csg::Color4 pathColor;
   if (!solution_) {
      RecommendBestPath(best);
      pathColor = csg::Color4(192, 32, 0);
   } else {
      best = solution_->GetPoints();
      pathColor = csg::Color4(32, 192, 0);
   }

   for (const auto& pt : best) {
      auto coord = msg->add_coords();
      pt.SaveValue(coord);
      pathColor.SaveValue(coord->mutable_color());
   }

#if 0
   for (const auto& node: open_) {
      if (std::find(best.begin(), best.end(), node.pt) == best.end()) {
         auto coord = msg->add_coords();
         node.pt.SaveValue(coord);
         csg::Color4(128, 128, 255, 192).SaveValue(coord->mutable_color());
      }
   }
   for (const auto& pt: closed_) {
      if (std::find(best.begin(), best.end(), pt) == best.end()) {
         auto coord = msg->add_coords();
         pt.SaveValue(coord);
         csg::Color4(0, 0, 128, 192).SaveValue(coord->mutable_color());
      }
   }
#endif

   for (const auto& dst : destinations_) {
      dst.second->EncodeDebugShapes(msg, debug_color_);
   }
}

void AStarPathFinder::Work(const platform::timer &timer)
{
   PF_LOG(7) << "entering work function";

   if (restart_search_) {
      Restart();
   }

   if (open_.empty()) {
      PF_LOG(7) << "open set is empty!  returning";
      SetSearchExhausted();
      return;
   }

   if (rebuildHeap_) {
      RebuildHeap();
   }

   VERIFY_HEAPINESS();
   PathFinderNode current = PopClosestOpenNode();
   closed_.insert(current.pt);

   PathFinderDst* closest;
   float h = EstimateCostToDestination(current.pt, &closest);
   if (h == 0) {
      // xxx: be careful!  this may end up being re-entrant (for example, removing
      // destinations).
      SolveSearch(current.pt, closest);
      return;
   } else if (h > max_cost_to_destination_) {
      PF_LOG(3) << "max cost to destination " << max_cost_to_destination_ << " exceeded.  marking search as exhausted.";
      SetSearchExhausted();
   }

   VERIFY_HEAPINESS();

   // Check each neighbor...
   const auto& o = GetSim().GetOctTree();
   
   phys::OctTree::MovementCostVector neighbors = o.ComputeNeighborMovementCost(entity_.lock(), current.pt);
   PF_LOG(7) << "compute neighbor movement cost from " << current.pt << " returned " << neighbors.size() << " results";
   if (neighbors.size() == 0) {
      //DebugBreak();
   }

   VERIFY_HEAPINESS();
   for (const auto& neighbor : neighbors) {
      const csg::Point3& pt = neighbor.first;
      float cost = neighbor.second;
      VERIFY_HEAPINESS();
      AddEdge(current, pt, cost);
      VERIFY_HEAPINESS();
   }

   VERIFY_HEAPINESS();
}


void AStarPathFinder::AddEdge(const PathFinderNode &current, const csg::Point3 &next, float movementCost)
{
   PF_LOG(10) << "       Adding Edge from " << current.pt << " to " << next << " cost:" << movementCost;

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

      float h = EstimateCostToDestination(next);
      float f = g + h;

      if (i == c) {
         // not found in the open list.  add a brand new node.
         PF_LOG(10) << "          Adding " << next << " to open set (f:" << f << " g:" << g << ").";
         cameFrom_[next] = current.pt;
         open_.push_back(PathFinderNode(next, f, g));
         if (!rebuildHeap_) {
            push_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
         }
         VERIFY_HEAPINESS();
      } else {
         // found.  should we update?
         if (g < open_[i].g) {
            cameFrom_[next] = current.pt;
            open_[i].g = g;
            open_[i].f = f;
            rebuildHeap_ = true; // really?  what if it's "good enough" to not validate heap properties?
            PF_LOG(10) << "          (updating) Adding " << next << " to open set (f:" << f << " g:" << g << ").";
         }
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
      i++;

      float h = dst->EstimateMovementCost(from);
      if (h < hMin) {
         closest = dst;
         closestId = entityId;
         hMin = h;
      }
   }
   if (hMin != FLT_MAX) {
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

void AStarPathFinder::ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const
{
   solution.push_back(dst);

   auto i = cameFrom_.find(dst);
   while (i != cameFrom_.end()) {
      solution.push_back(i->second);
      i = cameFrom_.find(i->second);
   }
   // the point on the very from contains the exact location of the starting entity.  we
   // don't want that point in the path, so pull it off before reversing the vectory
   solution.pop_back();
   std::reverse(solution.begin(), solution.end());
}

void AStarPathFinder::RecommendBestPath(std::vector<csg::Point3> &points) const
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

void AStarPathFinder::SolveSearch(const csg::Point3& last, PathFinderDst* dst)
{
   std::vector<csg::Point3> points;

   VERIFY_HEAPINESS();

   ReconstructPath(points, last);

   if (points.empty()) {
      points.push_back(source_->GetSourceLocation());
   }
   csg::Point3 dst_point_of_interest = dst->GetPointOfInterest(points.back());
   PF_LOG(5) << "found solution to destination " << dst->GetEntityId() << " (last point is " << last << ")";
   solution_ = std::make_shared<Path>(points, entity_.lock(), dst->GetEntity(), dst_point_of_interest);
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
   PF_LOG(3) << "requesting search restart: " << reason;
   restart_search_ = true;
   search_exhausted_ = false;
   solution_ = nullptr;
   cameFrom_.clear();
   closed_.clear();
   open_.clear();
   source_->Start();
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