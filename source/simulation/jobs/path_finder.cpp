#include "pch.h"
#include "metrics.h"
#include "path.h"
#include "path_finder.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/destination.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

// #define CHECK_HEAPINESS

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

PathFinder::PathFinder(std::string name, bool ownsDst) :
   Job(name),
   ownsDst_(ownsDst),
   state_(CONSTRUCTED),
   rebuildHeap_(false)
{
}

PathFinder::~PathFinder()
{
}

bool PathFinder::IsIdle(int now) const
{
   return !IsRunning();
}

void PathFinder::AddDestination(om::DestinationRef d)
{
   PROFILE_BLOCK();

   auto dst = d.lock();
   if (dst) {
      destinations_[dst->GetObjectId()] = dst;
      if (state_ == RUNNING) {
         LOG(WARNING) << "adding destination to a running search.  restarting.";
         solution_ = nullptr;
         state_ = RESTARTING;
      }
   }
}

void PathFinder::RemoveDestination(om::DestinationRef d)
{
   PROFILE_BLOCK();

   auto dst = d.lock();
   if (dst) {
      auto i = destinations_.find(dst->GetObjectId());
      if (i != destinations_.end()) {
         destinations_.erase(i);
         auto solutionDst = solution_->GetDestination().lock();
         if (solutionDst && solutionDst == dst) {
            if (state_ == RUNNING) {
               LOG(WARNING) << "removing destination from a running search.  restarting.";
               solution_ = nullptr;
               state_ = RESTARTING;
            } else if (state_ == SOLVED) {
               LOG(WARNING) << "removing solution destination from multi-search.  resuming!";
               solution_ = nullptr;
               state_ = RESTARTING;
            }
         }
      }
   }
}

void PathFinder::Restart()
{
   Start(entity_, start_);
}

void PathFinder::Start(om::EntityRef entity, const math3d::ipoint3& start)
{
   PROFILE_BLOCK();

   entity_ = entity;
   start_ = start;
   solution_ = nullptr;
   rebuildHeap_ = false;
   cameFrom_.clear();
   closed_.clear();
   open_.clear();
   f_.clear();
   g_.clear();
   h_.clear();
   open_.push_back(start);

   if (IsIdle(0)) {
      LOG(WARNING) << GetName() << " deferring search start because we're idle and can't compute first heuristic.";
      state_ = RESTARTING;
      return;
   }

   LOG(WARNING) << GetName() << " starting search";

   state_ = RUNNING;
   startTime_ = Simulation::GetInstance().GetStore().GetCurrentGenerationId();

   // LOG(WARNING) << "       Starting Search... " << GetName();

   // Add the heurestic cost of the closest destination to the score map
   int h = EstimateCostToDestination(start);
   f_[start] = h;
   h_[start] = h;
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

   ASSERT(!IsIdle(now));

#if 0
   if (state_ == RUNNING) {
      if (!VerifyDestinationModifyTimes()) {
         LOG(WARNING) << "destination in search set has been modified.  restarting.";
         solution_ = nullptr;
         state_ = RESTARTING;
      }
   }
#endif

   if (state_ == RESTARTING) {
      LOG(WARNING) << "restarting search in PathFinder::Work.";
      Start(entity_, start_);
   }
   //LogProgress(LOG(WARNING));

   ASSERT(state_ == RUNNING);

   if (open_.empty()) {
      AbortSearch(EXHAUSTED, "no solution found.");
      return;
   }

   if (rebuildHeap_) {
      RebuildHeap();
   }

   VERIFY_HEAPINESS();
   math3d::ipoint3 current = GetFirstOpen();
   stdutil::UniqueInsert(closed_, current);

   om::DestinationPtr closest;
   if (EstimateCostToDestination(current, closest) == 0) {
      SolveSearch(current, closest);
      return;
   }

   VERIFY_HEAPINESS();

   // Check each neighbor...
   const auto& o = Simulation::GetInstance().GetOctTree();

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
   if (state_ == RESTARTING) {
      LOG(WARNING) << "restarting search in PathFinder::EstimateCostToSolution.";
      Start(entity_, start_);
   }

   if (IsIdle(0) || open_.empty()) {
      return INT_MAX;
   }

   return f_[open_.front()];
}

int PathFinder::EstimateCostToDestination(const math3d::ipoint3 &from) const
{
   om::DestinationPtr ignore;
   ASSERT(!IsIdle(0));
   return EstimateCostToDestination(from, ignore);
}


int PathFinder::EstimateMovementCost(const math3d::ipoint3& from, om::DestinationPtr dst) const
{
   csg::Region3 const& rgn = **dst->GetAdjacent();
   if (rgn.IsEmpty()) {
      return INT_MAX;
   }
   math3d::ipoint3 start = from;
   auto mob = dst->GetEntity().GetComponent<om::Mob>();
   if (mob) {
      start -= mob->GetWorldGridLocation();
   }
   csg::Point3 end = rgn.GetClosestPoint(start);

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

int PathFinder::EstimateCostToDestination(const math3d::ipoint3 &from, om::DestinationPtr& closest) const
{
   int hMin = INT_MAX;

   ASSERT(!IsIdle(0));

   auto i = destinations_.begin();
   while (i != destinations_.end()) {
      auto dst = i->second.lock();
      if (dst) {
         int h = EstimateMovementCost(from, dst);
         //LOG(WARNING) << GetName() << "    sub cost to dst: " << h << "(vs: " << hMin << ")";
         if (h < hMin) {
            closest = dst;
            hMin = h;
         }
         i++;
      } else {
         i = destinations_.erase(i);
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
   reverse(solution.begin(), solution.end());
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

   make_heap(open_.begin(), open_.end(), bind(&PathFinder::CompareEntries, this, std::placeholders::_1, std::placeholders::_2));
   VERIFY_HEAPINESS();
   rebuildHeap_ = false;
}

bool PathFinder::VerifyDestinationModifyTimes()
{
   ASSERT(state_ == RUNNING);
   bool changed = false;

   auto i = destinations_.begin();
   while (i != destinations_.end()) {
      auto dst = i->second.lock();

      if (dst) {
         int modifyTime = dst->GetLastModified();
         if (modifyTime > startTime_) {
            changed = true;
         }
         i++;
      } else {
         i = destinations_.erase(i);
      }
   }

   return !changed;
}

void PathFinder::AbortSearch(State next, std::string reason)
{
   ASSERT(IsRunning());
   state_ = next;
   stopReason_ = reason;  
}

void PathFinder::SolveSearch(const math3d::ipoint3& last, om::DestinationPtr dst)
{
   std::vector<math3d::ipoint3> points;

   ASSERT(state_ == RUNNING);
   VERIFY_HEAPINESS();

   ReconstructPath(points, last);
   solution_ = std::make_shared<Path>(points, entity_, dst);
   solutionTime_ = dst->GetLastModified();
   state_ = SOLVED;

   VERIFY_HEAPINESS();
}

bool PathFinder::IsRunning() const
{
   CheckSolution();
   return state_ == RUNNING || state_ == RESTARTING;
}

PathPtr PathFinder::GetSolution() const
{
   CheckSolution();
   return solution_;
}

void PathFinder::CheckSolution() const
{
   PROFILE_BLOCK();

   // xxx: we shouldn't ahve to do this if we put change trackers on the destinations!!
   if (solution_) {
      auto dst = solution_->GetDestination().lock();
      if (!dst || solutionTime_ != dst->GetLastModified()) {
         solution_ = nullptr;
         state_ = RESTARTING;
      }
   }
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

