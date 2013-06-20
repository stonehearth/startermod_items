#include "pch.h"
#include "metrics.h"
#include "path.h"
#include "path_finder.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/destination.h"
#include "om/region.h"
#include "simulation/script/script_host.h"

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

PathFinder::PathFinder(std::string name, om::EntityRef e, luabind::object solved_cb, luabind::object dst_filter) :
   Job(name),
   rebuildHeap_(false),
   entity_(e),
   source_(*this, e),
   stopped_(false),
   solved_cb_(solved_cb),
   dst_filter_(dst_filter),
   search_exhausted_(false),
   reversed_search_(false),
   restart_search_(true)
{
}

PathFinder::~PathFinder()
{
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

bool PathFinder::IsIdle(int now) const
{
   bool busy = restart_search_ || (!solution_ && !search_exhausted_);
   return stopped_ || !busy;
}

void PathFinder::AddDestination(om::EntityRef e)
{
   PROFILE_BLOCK();

   auto entity = e.lock();
   if (entity) {
      destinations_[entity->GetObjectId()] = std::unique_ptr<PathFinderEndpoint>(new PathFinderEndpoint(*this, e));
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
   search_exhausted_ = false;
   cameFrom_.clear();
   closed_.clear();
   open_.clear();
   f_.clear();
   g_.clear();
   h_.clear();

   source_.AddAdjacentToOpenSet(open_);
   for (math3d::ipoint3 const& pt : open_) {
   	int h = EstimateCostToDestination(pt);
	   f_[pt] = h;
	   h_[pt] = h;
   }
}

void PathFinder::SetReverseSearch(bool reversed)
{
   if (reversed != reversed_search_) {
      reversed_search_ = reversed;
      RestartSearch();
   }
}

void PathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   PROFILE_BLOCK();

   std::vector<math3d::ipoint3> best;
   math3d::color4 pathColor;
   if (!solution_) {
      RecommendBestPath(best);
      pathColor = math3d::color4(192, 32, 0);
   } else {
      best = solution_->GetPoints();
      pathColor = math3d::color4(32, 192, 0);
   }

   for (const auto& pt : best) {
      pt.SaveValue(msg->add_coords(), pathColor);
   }

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
         pt.SaveValue(msg->add_coords(), math3d::color4(shade, shade, 255, 192));
      }
   }
   for (const auto& pt : closed_) {
      if (std::find(best.begin(), best.end(), pt) == best.end()) {
         pt.SaveValue(msg->add_coords(), math3d::color4(0, 0, 128, 192));
      }
   }
}

void PathFinder::Work(int now, const platform::timer &timer)
{
   PROFILE_BLOCK();

   if (restart_search_) {
      Restart();
   }

   if (open_.empty()) {
      search_exhausted_ = true;
      return;
   }

   if (rebuildHeap_) {
      RebuildHeap();
   }

   VERIFY_HEAPINESS();
   math3d::ipoint3 current = GetFirstOpen();
   stdutil::UniqueInsert(closed_, current);

   om::EntityRef closest;
   if (EstimateCostToDestination(current, closest) == 0) {
      // xxx: be careful!  this may end up being re-entrant (for example, removing
      // destinations).
      SolveSearch(current, closest);
      return;
   }

   VERIFY_HEAPINESS();

   // Check each neighbor...
   const auto& o = Simulation::GetInstance().GetOctTree();
   
   // xxx: not correct for reversed search yet...
   if (reversed_search_) {
      LOG(WARNING) << "neighbor cost in revese search is reversed!";
   }
   auto neighbors = o.ComputeNeighborMovementCost(current);

   VERIFY_HEAPINESS();
   for (const auto& neighbor : neighbors) {
      const math3d::ipoint3& pt = neighbor.first;
      int cost = neighbor.second;
      VERIFY_HEAPINESS();
      AddEdge(current, pt, cost);
      VERIFY_HEAPINESS();
   }

   VERIFY_HEAPINESS();
}


void PathFinder::AddEdge(const math3d::ipoint3 &current, const math3d::ipoint3 &next, int movementCost)
{
   // LOG(WARNING) << "       Adding Edge " << neighbor.first << " cost:" << neighbor.second;

   VERIFY_HEAPINESS();

   if (!stdutil::contains(closed_, next)) {
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
   if (open_.empty()) {
      return INT_MAX;
   }
   return f_[open_.front()];
}

int PathFinder::EstimateCostToDestination(const math3d::ipoint3 &from) const
{
   ASSERT(!restart_search_);

   om::EntityRef ignore;
   return EstimateCostToDestination(from, ignore);
}

int PathFinder::EstimateCostToDestination(const math3d::ipoint3 &from, om::EntityRef& closest) const
{
   int hMin = INT_MAX;

   ASSERT(!restart_search_);

   auto i = destinations_.begin();
   while (i != destinations_.end()) {
      auto dst = i->second.get();      
      if (!dst->GetEntity()) {
         i = destinations_.erase(i);
      } else {
         int h = dst->EstimateMovementCost(from);
         //LOG(WARNING) << GetName() << "    sub cost to dst: " << h << "(vs: " << hMin << ")";
         if (h < hMin) {
            closest = dst->GetEntity();
            hMin = h;
         }
         i++;
      }
   }
   if (hMin != INT_MAX) {
      float fudge = 1.25f; // to make the search finder at the hMin of accuracy
      hMin ? std::max(1, (int)(hMin * fudge)) : 0;
   }
   return hMin;
}

math3d::ipoint3 PathFinder::GetFirstOpen()
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

void PathFinder::ReconstructPath(std::vector<math3d::ipoint3> &solution, const math3d::ipoint3 &dst) const
{
   solution.push_back(dst);

   auto i = cameFrom_.find(dst);
   while (i != cameFrom_.end()) {
      solution.push_back(i->second);
      i = cameFrom_.find(i->second);
   }
   if (!reversed_search_) {
      std::reverse(solution.begin(), solution.end());
   }
}

void PathFinder::RecommendBestPath(std::vector<math3d::ipoint3> &points) const
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

bool PathFinder::CompareEntries(const math3d::ipoint3 &a, const math3d::ipoint3 &b)
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

void PathFinder::SolveSearch(const math3d::ipoint3& last, om::EntityRef dst)
{
   std::vector<math3d::ipoint3> points;

   VERIFY_HEAPINESS();

   ReconstructPath(points, last);
   solution_ = std::make_shared<Path>(points, entity_, dst);
   if (solved_cb_.is_valid()) {
      auto L = solved_cb_.interpreter();
      luabind::object path(L, solution_);
      ScriptHost::GetInstance().Call(solved_cb_, path);
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


PathFinderEndpoint::PathFinderEndpoint(PathFinder &pf, om::EntityRef e) :
   pf_(pf),
   entity_(e)
{
   auto entity = e.lock();
   if (entity) {
      auto restart_fn = [=]() {
         pf_.RestartSearch();
      };

      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         guards_ += mob->GetBoxedTransform().TraceValue(
            "pathfinder entity mob trace",
            [=](math3d::transform const&) {
               restart_fn();
            });
      }
      auto dst = entity->GetComponent<om::Destination>();
      if (dst) {
         guards_ += dst->GetAdjacent()->Trace(
            "pathfinder destination trace",
            [=](csg::Region3 const&) {
               restart_fn();
            });
      }
   }
}

void PathFinderEndpoint::AddAdjacentToOpenSet(std::vector<math3d::ipoint3>& open)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         math3d::ipoint3 origin;
         origin = mob->GetWorldGridLocation();

         auto destination = entity->GetComponent<om::Destination>();
         if (destination) {
            // xxx - should open_ be a region?  wow, that would be complicated!
            om::RegionPtr adjacent = destination->GetAdjacent();
            csg::Region3 const& region = **adjacent;
            for (csg::Cube3 const& cube : region) {
               for (csg::Point3 const& pt : cube) {
                  open.push_back(origin + pt);
               }
            }
         } else {
            open.push_back(origin);
         }
      }
   }
}

int PathFinderEndpoint::EstimateMovementCost(const math3d::ipoint3& from) const
{
   auto entity = GetEntity();

   if (!entity) {
      return INT_MAX;
   }

   // Translate the point to the local coordinate system
   math3d::ipoint3 start = from;
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      start -= mob->GetWorldGridLocation();
   }

   csg::Point3 end;
   om::DestinationPtr dst = entity->GetComponent<om::Destination>();
   if (dst) {
      csg::Region3 const& rgn = **dst->GetAdjacent();
      if (rgn.IsEmpty()) {
         return INT_MAX;
      }
      end = rgn.GetClosestPoint(start);
   } else {
      csg::Region3 adjacent;
      adjacent += csg::Point3(-1, 0,  0);
      adjacent += csg::Point3( 1, 0,  0);
      adjacent += csg::Point3( 0, 0, -1);
      adjacent += csg::Point3( 0, 0, -1);
      end = adjacent.GetClosestPoint(start);
   }
   return EstimateMovementCost(start, end);

   return INT_MAX;
}

int PathFinderEndpoint::EstimateMovementCost(csg::Point3 const& start, csg::Point3 const& end) const
{
   static int COST_SCALE = 10;
   int cost = 0;

   // it's fairly expensive to climb.
   cost += COST_SCALE * std::max(end.y - start.y, 0) * 2;

   // falling is super cheap.
   cost += std::max(start.y - end.y, 0);

   // diagonals need to be more expensive than cardinal directions
   int xCost = abs(end.x - start.x);
   int zCost = abs(end.z - start.z);
   int diagCost = std::min(xCost, zCost);
   int horzCost = std::max(xCost, zCost) - diagCost;

   cost += (int)((horzCost + diagCost * 1.414) * COST_SCALE);

   return cost;
}

