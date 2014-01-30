#include "pch.h"
#include "path.h"
#include "path_finder.h"
#include "path_finder_src.h"
#include "path_finder_dst.h"
#include "simulation/simulation.h"
#include "lib/lua/script_host.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"
#include "lib/perfmon/store.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

std::vector<std::weak_ptr<PathFinder>> PathFinder::all_pathfinders_;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder, level, GetName())

#if defined(CHECK_HEAPINESS)
#  define VERIFY_HEAPINESS() \
   do { \
      if (!rebuildHeap_) { \
         _Debug_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, _1, _2)); \
      } \
   } while (false);
#else
#  define VERIFY_HEAPINESS()
#endif

std::shared_ptr<PathFinder> PathFinder::Create(Simulation& sim, std::string name, om::EntityPtr entity)
{
   std::shared_ptr<PathFinder> pathfinder(new PathFinder(sim, name, entity));
   all_pathfinders_.push_back(pathfinder);
   return pathfinder;
}

void PathFinder::ComputeCounters(perfmon::Store& store)
{
   int count = 0;
   int running_count = 0;
   int open_count = 0;
   int closed_count = 0;
   int active_count = 0;

   stdutil::ForEachPrune<PathFinder>(all_pathfinders_, [&](std::shared_ptr<PathFinder> pf) {
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
   store.GetCounter("pathfinders.total_count").SetValue(count);
   store.GetCounter("pathfinders.active_count").SetValue(active_count);
   store.GetCounter("pathfinders.running_count").SetValue(running_count);
   store.GetCounter("pathfinders.open_node_count").SetValue(open_count);
   store.GetCounter("pathfinders.closed_node_count").SetValue(closed_count);
}

PathFinder::PathFinder(Simulation& sim, std::string name, om::EntityPtr entity) :
   Job(sim, name),
   rebuildHeap_(false),
   restart_search_(true),
   search_exhausted_(false),
   enabled_(true),
   entity_(entity),
   debug_color_(255, 192, 0, 128)
{
   PF_LOG(3) << "creating pathfinder";
   source_.reset(new PathFinderSrc(*this, entity));
}

PathFinder::~PathFinder()
{
   PF_LOG(3) << "destroying pathfinder";
}

PathFinderPtr PathFinder::SetSource(csg::Point3 const& location)
{
   source_->SetSourceOverride(location);
   RestartSearch("source location changed");
   return shared_from_this();
}

PathFinderPtr PathFinder::SetSolvedCb(luabind::object unsafe_solved_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_solved_cb.interpreter());  
   solved_cb_ = luabind::object(cb_thread, unsafe_solved_cb);
   return shared_from_this();
}

PathFinderPtr PathFinder::SetSearchExhaustedCb(luabind::object unsafe_search_exhuasted_cb)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_search_exhuasted_cb.interpreter());  
   search_exhausted_cb_ = luabind::object(cb_thread, unsafe_search_exhuasted_cb);
   return shared_from_this();
}

PathFinderPtr PathFinder::SetFilterFn(luabind::object unsafe_dst_filter)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_dst_filter.interpreter());  
   dst_filter_ = luabind::object(cb_thread, unsafe_dst_filter);
   RestartSearch("filter function changed");
   return shared_from_this();
}

bool PathFinder::IsIdle() const
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

PathFinderPtr PathFinder::Start()
{
   PF_LOG(5) << "start requested";
   enabled_ = true;
   return shared_from_this();
}

PathFinderPtr PathFinder::Stop()
{
   PF_LOG(5) << "stop requested";
   RestartSearch("pathfinder stopped");
   enabled_ = false;
   return shared_from_this();
}

PathFinderPtr PathFinder::AddDestination(om::EntityRef e)
{
   auto entity = e.lock();
   if (entity) {
      if (dst_filter_ && luabind::type(dst_filter_) == LUA_TFUNCTION) {
         bool ok = false;
         lua_State* L = dst_filter_.interpreter();
         try {
            luabind::object e(L, std::weak_ptr<om::Entity>(entity));
            PF_LOG(5) << "calling lua solution callback";
            ok = lua::ScriptHost::CoerseToBool(dst_filter_(e));
            PF_LOG(5) << "finished calling lua solution callback";
         } catch (std::exception&) {
         }
         if (!ok) {
            return shared_from_this();
         }
      }

      destinations_[entity->GetObjectId()] = std::unique_ptr<PathFinderDst>(new PathFinderDst(*this, e));
      restart_search_ = true;
      solution_ = nullptr;
   }
   return shared_from_this();
}

PathFinderPtr PathFinder::RemoveDestination(dm::ObjectId id)
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

void PathFinder::Restart()
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
      int h = EstimateCostToDestination(node.pt);
      node.f = h;
      node.g = 0;

      // by default search no more than 4x the distance
      if (h > 0) {
         max_cost_to_destination_ = std::max(max_cost_to_destination_, h * 4);
      }
   }
}

void PathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   if (!IsIdle()) {
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
   }

#if 0
   int maxF = 0;
   for (const auto& pt : open_) {
      auto i = h_.find(pt);
      if (i != h_.end()) {
         maxF = std::max(maxF, i->second);
      }
   }
   for (const auto& pt : open_) {
      if (std::find(best.begin(), best.end(), pt) == best.end()) {
         auto i = h_.find(pt);
         int shade = (int)(255 * ( (maxF - i->second) / (float)maxF ));

         auto coord = msg->add_coords();
         pt.SaveValue(coord);
         csg::Color4(shade, shade, 255, 192).SaveValue(coord->mutable_color());
      }
   }
   for (const auto& entry: closed_) {
      csg::Point3 const& pt = entry.first;
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

void PathFinder::Work(const platform::timer &timer)
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
   PathFinderNode current = GetFirstOpen();
   closed_.insert(current.pt);

   PathFinderDst* closest;
  int h = EstimateCostToDestination(current.pt, &closest);
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
   
   auto neighbors = o.ComputeNeighborMovementCost(entity_.lock(), current.pt);
   PF_LOG(7) << "compute neighbor movment cost from " << current.pt << " returned " << neighbors.size() << " results";
   if (neighbors.size() == 0) {
      //DebugBreak();
   }

   VERIFY_HEAPINESS();
   for (const auto& neighbor : neighbors) {
      const csg::Point3& pt = neighbor.first;
      int cost = neighbor.second;
      VERIFY_HEAPINESS();
      AddEdge(current, pt, cost);
      VERIFY_HEAPINESS();
   }

   VERIFY_HEAPINESS();
}


void PathFinder::AddEdge(const PathFinderNode &current, const csg::Point3 &next, int movementCost)
{
   PF_LOG(10) << "       Adding Edge " << current.pt << " cost:" << next;

   VERIFY_HEAPINESS();

   if (closed_.find(next) == closed_.end()) {
      bool update = false;
      int g = current.g + movementCost;

      uint i, c = open_.size();
      for (i = 0; i < c; i++) {
         if (open_[i].pt == next) {
            break;
         }
      }
      VERIFY_HEAPINESS();

      int h = EstimateCostToDestination(next);
      int f = g + h;

      if (i == c) {
         // not found in the open list.  add a brand new node.
         PF_LOG(10) << "          Adding " << next << " to open set.";
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
            rebuildHeap_ = true; // really?
            PF_LOG(10) << "          Updating costs maps, f:" << f << "  g:" << g;
         }
      }
   }

   VERIFY_HEAPINESS();
}

int PathFinder::EstimateCostToSolution()
{
   if (!enabled_) {
      return INT_MAX;
   }
   if (restart_search_) {
      Restart();
   }
   if (IsIdle()) {
      return INT_MAX;
   }
   return open_.front().f;
}

int PathFinder::EstimateCostToDestination(const csg::Point3 &from) const
{
   ASSERT(!restart_search_);

   return EstimateCostToDestination(from, nullptr);
}

int PathFinder::EstimateCostToDestination(const csg::Point3 &from, PathFinderDst** closest) const
{
   int hMin = INT_MAX;

   ASSERT(!restart_search_);

   auto i = destinations_.begin();
   while (i != destinations_.end()) {
      auto dst = i->second.get();      
      if (!dst->GetEntity()) {
         i = destinations_.erase(i);
      } else {
         i++;

         om::EntityPtr entity = dst->GetEntity();
         if (!entity) {
            continue;
         }
         int h = dst->EstimateMovementCost(from);
         PF_LOG(10) << "    sub cost to dst: " << h << "(vs: " << hMin << ")";
         if (h < hMin) {
            if (closest) {
               *closest = dst;
            }
            hMin = h;
         }
      }
   }
   if (hMin != INT_MAX) {
      float fudge = 1.25f; // to make the search finder at the hMin of accuracy
      hMin ? std::max(1, (int)(hMin * fudge)) : 0;
   }
   PF_LOG(10) << "    EstimateCostToDestination returning " << hMin;
   return hMin;
}

PathFinderNode PathFinder::GetFirstOpen()
{
   auto result = open_.front();

   VERIFY_HEAPINESS();
   pop_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
   open_.pop_back();
   VERIFY_HEAPINESS();

   PF_LOG(10) << " GetFirstOpen returning " << result.pt << ".  " << open_.size() << " points remain in open set";

   return result;
}

void PathFinder::ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const
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

void PathFinder::RecommendBestPath(std::vector<csg::Point3> &points) const
{
   points.clear();
   if (!open_.empty()) {
      if (rebuildHeap_) {
         const_cast<PathFinder*>(this)->RebuildHeap();
      }
      ReconstructPath(points, open_.front().pt);
   }
}

std::string PathFinder::GetProgress() const
{
   std::string ename;
   auto entity = entity_.lock();
   if (entity) {
      ename = BUILD_STRING(*entity);
   }
   return BUILD_STRING(GetName() << " " << ename << " (open: " << open_.size() << "  closed: " << closed_.size() << ")");
}

void PathFinder::RebuildHeap()
{
   std::make_heap(open_.begin(), open_.end(), PathFinderNode::CompareFitness);
   VERIFY_HEAPINESS();
   rebuildHeap_ = false;
}

csg::Point3 PathFinder::GetSourceLocation()
{
   csg::Point3 pt(0, 0, 0);
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         pt = mob->GetWorldGridLocation();
      }
   }
   return pt;
}

void PathFinder::SetSearchExhausted()
{
   if (!search_exhausted_) {
      cameFrom_.clear();
      closed_.clear();
      open_.clear();
      search_exhausted_ = true;
      if (search_exhausted_cb_.is_valid()) {
         PF_LOG(5) << "calling lua search exhausted callback";
         try {
            search_exhausted_cb_();
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(search_exhausted_cb_.interpreter(), e);
         }
         PF_LOG(5) << "finished calling lua search exhausted callback";
      }
   }
}

void PathFinder::SolveSearch(const csg::Point3& last, PathFinderDst* dst)
{
   std::vector<csg::Point3> points;

   VERIFY_HEAPINESS();

   ReconstructPath(points, last);

   csg::Point3 end_point;
   if (!points.empty()) {
      end_point = points.back();
   } else {
      end_point = GetSourceLocation();
   }
   csg::Point3 dst_point_of_interest = dst->GetPointfInterest(end_point);
   PF_LOG(5) << "found solution to destination " << dst->GetEntityId();
   solution_ = std::make_shared<Path>(points, entity_.lock(), dst->GetEntity(), dst_point_of_interest);
   if (solved_cb_.is_valid()) {
      PF_LOG(5) << "calling lua solved callback";
      try {
         solved_cb_(luabind::object(solved_cb_.interpreter(), solution_));
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering solved cb: " << e.what();
      }

      PF_LOG(5) << "finished calling lua solved callback";
   }

   VERIFY_HEAPINESS();
}

PathPtr PathFinder::GetSolution() const
{
   return solution_;
}

PathFinderPtr PathFinder::RestartSearch(const char* reason)
{
   PF_LOG(3) << "requesting search restart: " << reason;
   restart_search_ = true;
   search_exhausted_ = false;
   solution_ = nullptr;
   cameFrom_.clear();
   closed_.clear();
   open_.clear();
   return shared_from_this();
}

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const PathFinder& pf)
{
   return pf.Format(o);
}

std::ostream& PathFinder::Format(std::ostream& o) const
{
   o << "[pf " << GetName() << "]";
   return o;        
}

bool PathFinder::IsSearchExhausted() const
{
   return open_.empty() || search_exhausted_;
}

void PathFinder::SetDebugColor(csg::Color4 const& color)
{
   debug_color_ = color;
}

std::string PathFinder::DescribeProgress()
{
   std::ostringstream progress;
   progress << GetName() << open_.size() << " open nodes. " << closed_.size() << " closed nodes. ";
   if (!open_.empty()) {
      progress << EstimateCostToDestination(GetFirstOpen().pt) << " nodes from destination. ";
   }
   progress << "idle? " << IsIdle();
   return progress.str();
}