#include "pch.h"
#include "metrics.h"
#include "path.h"
#include "path_finder.h"
#include "path_finder_src.h"
#include "path_finder_dst.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/destination.h"
#include "om/region.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

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

PathFinder::PathFinder(lua_State* L, std::string name) :
   Job(name),
   rebuildHeap_(false),
   restart_search_(true),
   enabled_(true),
   debug_color_(255, 192, 0, 128)
{
}

PathFinder::~PathFinder()
{
}

void PathFinder::SetSource(om::EntityRef e)
{
   entity_ = e;
   mob_.reset();

   source_.reset(new PathFinderSrc(*this, entity_));

   auto entity = e.lock();
   if (entity) {
      mob_ = entity->GetComponent<om::Mob>();
   }
   RestartSearch();
}

void PathFinder::SetSolvedCb(luabind::object solved_cb)
{
   solved_cb_ = solved_cb;
}

void PathFinder::SetFilterFn(luabind::object dst_filter)
{
   dst_filter_ = dst_filter;
   RestartSearch();
}

bool PathFinder::IsIdle() const
{
   if (!enabled_) {
      return true;
   }

   if (!source_ || source_->IsIdle()) {
      return true;
   }

   if (entity_.expired()) {
      return true;
   }

   if (restart_search_) {
      return false;
   }

   if (IsSearchExhausted()) {
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
      return true;
   }


   if (solution_) {
      return true;
   }
   return false;
}

void PathFinder::Start()
{
   enabled_ = true;
}

void PathFinder::Stop()
{
   enabled_ = false;
}

void PathFinder::AddDestination(om::EntityRef e)
{
   PROFILE_BLOCK();

   auto entity = e.lock();
   if (entity) {
      if (dst_filter_.is_valid() && luabind::type(dst_filter_) == LUA_TFUNCTION) {
         try {
            auto L = dst_filter_.interpreter();
            luabind::object e(L, std::weak_ptr<om::Entity>(entity));
            if (!luabind::call_function<bool>(dst_filter_, e)) {
               return;
            }
         } catch (luabind::error& e) {
            LOG(WARNING) << "luabind::error " << e.what();
            return;
         } catch (std::exception& e) {
            LOG(WARNING) << "std::exception " << e.what();
            return;
         } catch (...) {
            LOG(WARNING) << "unknown error in pathfinder filter cb...";
            return;
         }
      }

      destinations_[entity->GetObjectId()] = std::unique_ptr<PathFinderDst>(new PathFinderDst(*this, e));
      restart_search_ = true;
      solution_ = nullptr;
   }
}

void PathFinder::RemoveDestination(dm::ObjectId id)
{
   PROFILE_BLOCK();

   auto i = destinations_.find(id);
   if (i != destinations_.end()) {
      destinations_.erase(i);
      if (solution_) {
         auto solution_entity = solution_->GetDestination().lock();
         if (solution_entity && solution_entity->GetObjectId() == id) {
            RestartSearch();
         }
      }
   }
}

void PathFinder::Restart()
{
   PROFILE_BLOCK();

   ASSERT(restart_search_);

   solution_ = nullptr;
   rebuildHeap_ = true;
   restart_search_ = false;
   cameFrom_.clear();
   closed_.clear();
   open_.clear();
   f_.clear();
   g_.clear();
   h_.clear();

   source_->InitializeOpenSet(open_);
   for (csg::Point3 const& pt : open_) {
   	int h = EstimateCostToDestination(pt);
	   f_[pt] = h;
	   h_[pt] = h;
   }
}

void PathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   PROFILE_BLOCK();

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
   PROFILE_BLOCK();

   if (restart_search_) {
      Restart();
   }

   if (open_.empty()) {
      return;
   }

   if (rebuildHeap_) {
      RebuildHeap();
   }

   VERIFY_HEAPINESS();
   csg::Point3 current = GetFirstOpen();
   closed_[current] = true;

   PathFinderDst* closest;
   if (EstimateCostToDestination(current, &closest) == 0) {
      // xxx: be careful!  this may end up being re-entrant (for example, removing
      // destinations).
      SolveSearch(current, closest);
      return;
   }

   VERIFY_HEAPINESS();

   // Check each neighbor...
   const auto& o = Simulation::GetInstance().GetOctTree();
   
   // xxx: not correct for reversed search yet...
   auto neighbors = o.ComputeNeighborMovementCost(current);

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


void PathFinder::AddEdge(const csg::Point3 &current, const csg::Point3 &next, int movementCost)
{
   // LOG(WARNING) << "       Adding Edge " << neighbor.first << " cost:" << neighbor.second;

   VERIFY_HEAPINESS();

   if (closed_.find(next) == closed_.end()) {
      bool update = false;
      int g = g_[current] + movementCost;

      auto currentg_ = g_.find(next);
      bool found = currentg_ != g_.end();

      VERIFY_HEAPINESS();

      if (!found || currentg_->second > g) {
         VERIFY_HEAPINESS();
         int h = EstimateCostToDestination(next);
         VERIFY_HEAPINESS();

         if (!rebuildHeap_) {
            auto j = f_.find(next);
            rebuildHeap_ = j != f_.end() && j->second != (g + h);
         }
         VERIFY_HEAPINESS();
         cameFrom_[next] = current;
         g_[next] = g;
         f_[next] = g + h;
         h_[next] = h;
         VERIFY_HEAPINESS();

         // LOG(WARNING) << "          Updating costs maps, f:" << (g+h) << "  g:" << g;
      }
      if (!found) {
         // LOG(WARNING) << "          Adding " << next << " to open set.";
         open_.push_back(next);
         if (!rebuildHeap_) {
            push_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, std::placeholders::_1, std::placeholders::_2));
         }
         VERIFY_HEAPINESS();
      }
   }

   VERIFY_HEAPINESS();
}

int PathFinder::EstimateCostToSolution()
{
   if (restart_search_) {
      Restart();
   }
   if (IsIdle()) {
      return INT_MAX;
   }
   return f_[open_.front()];
}

int PathFinder::EstimateCostToDestination(const csg::Point3 &from) const
{
   PROFILE_BLOCK();

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
         // LOG(WARNING) << GetName() << "    sub cost to dst: " << h << "(vs: " << hMin << ")";
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
   //LOG(WARNING) << GetName() << "    EstimateCostToDestination returning " << hMin;
   return hMin;
}

csg::Point3 PathFinder::GetFirstOpen()
{
   PROFILE_BLOCK();
   auto result = open_.front();

   VERIFY_HEAPINESS();
   pop_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, std::placeholders::_1, std::placeholders::_2));
   open_.pop_back();
   VERIFY_HEAPINESS();

   //LOG(WARNING) << GetName() << " open size is " << open_.size();

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
      ReconstructPath(points, open_.front());
   }
}

void PathFinder::LogProgress(std::ostream& o) const
{
   o << GetName() << " " << "(open: " << open_.size() << "  closed: " << closed_.size() << ")";
}

bool PathFinder::CompareEntries(const csg::Point3 &a, const csg::Point3 &b)
{
   ASSERT(stdutil::contains(f_, a) && stdutil::contains(f_, b));
   return f_[a] > f_[b];
}


void PathFinder::RebuildHeap()
{
   PROFILE_BLOCK();

   std::make_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, std::placeholders::_1, std::placeholders::_2));
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
   solution_ = std::make_shared<Path>(points, entity_.lock(), dst->GetEntity(), dst_point_of_interest);
   if (solved_cb_.is_valid()) {
      auto L = solved_cb_.interpreter();
      luabind::object path(L, solution_);
      luabind::call_function<void>(solved_cb_, path);
   }

   VERIFY_HEAPINESS();
}

PathPtr PathFinder::GetSolution() const
{
   return solution_;
}

void PathFinder::RestartSearch()
{
   restart_search_ = true;
   solution_ = nullptr;
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
   return open_.empty();
}

void PathFinder::SetDebugColor(csg::Color4 const& color)
{
   debug_color_ = color;
}
