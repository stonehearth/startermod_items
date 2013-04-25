#include "pch.h"
#include "metrics.h"
#include "path.h"
#include "path_finder.h"
#include "simulation/simulation.h"
#include "om/entity.h"

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
   if (IsRunning()) {
      for (const auto& entry : destinations_) {
         if (entry.second->IsEnabled()) {
            return false;
         }
      }
   }
   return true;
}

void PathFinder::AddDestination(DestinationPtr dst)
{
   PROFILE_BLOCK();

   destinations_[dst->GetEntityId()] = dst;
   if (state_ == RUNNING) {
      LOG(WARNING) << "adding destination to a running search.  restarting.";
      solution_ = nullptr;
      state_ = RESTARTING;
   }
}

void PathFinder::RemoveDestination(om::EntityId id)
{
   PROFILE_BLOCK();

   auto i = destinations_.find(id);
   if (i != destinations_.end()) {
      if (state_ == RUNNING) {
         LOG(WARNING) << "removing destination from a running search.  restarting.";
         solution_ = nullptr;
         state_ = RESTARTING;
      } else if (state_ == SOLVED) {
         if (solution_->GetDestination() == i->second) {
            LOG(WARNING) << "removing solution destination from multi-search.  resuming!";
            solution_ = nullptr;
            state_ = RESTARTING;
         }
      }
      destinations_.erase(i);
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

   PointList best;
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
   if (ownsDst_) {
      for (auto& entry : destinations_ ) {
         entry.second->EncodeDebugShapes(msg);
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

   DestinationPtr closest;
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
   DestinationPtr ignore;
   ASSERT(!IsIdle(0));
   return EstimateCostToDestination(from, ignore);
}

int PathFinder::EstimateCostToDestination(const math3d::ipoint3 &from, DestinationPtr& closest) const
{
   int hMin = INT_MAX;

   ASSERT(!IsIdle(0));
   for (auto& entry : destinations_) {
      if (entry.second->IsEnabled()) {
start:
         int h = entry.second->EstimateCostToDestination(from);
         //LOG(WARNING) << GetName() << "    sub cost to dst: " << h << "(vs: " << hMin << ")";
         if (h == INT_MAX) {
            DebugBreak();
            goto start;
         }
         if (h < hMin) {
            closest = entry.second;
            hMin = h;
         }
      }
   }
   ASSERT(hMin != INT_MAX);

   // LOG(WARNING) << GetName() << " cost to dst: " << hMin;
   float fudge = 1.25f; // to make the search finder at the cost of accuracyd
   return hMin ? std::max(1, (int)(hMin * fudge)) : 0;
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

void PathFinder::ReconstructPath(PointList &solution, const math3d::ipoint3 &dst) const
{
   solution.push_back(dst);

   auto i = cameFrom_.find(dst);
   while (i != cameFrom_.end()) {
      solution.push_back(i->second);
      i = cameFrom_.find(i->second);
   }
   reverse(solution.begin(), solution.end());
}

void PathFinder::RecommendBestPath(PointList &points) const
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
      auto dst = i->second;

      if (dst->GetEntity().lock()) {
         int modifyTime = dst->GetLastModificationTime();
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

void PathFinder::SolveSearch(const math3d::ipoint3& last, DestinationPtr dst)
{
   PointList points;

   ASSERT(state_ == RUNNING);
   VERIFY_HEAPINESS();

   ReconstructPath(points, last);
   solution_ = std::make_shared<Path>(points, entity_, dst);
   solutionTime_ = dst->GetLastModificationTime();
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

   if (solution_) {
      auto dst = solution_->GetDestination();
      auto entity = dst->GetEntity().lock();
      if (!entity) {
         LOG(WARNING) << "xxx BUG: Track lifetime, then i = destinations_.erase(i); in the destructor...";
         auto i = destinations_.begin();
         while (i != destinations_.end()) {
            if (dst == i->second) {
               destinations_.erase(i);
               break;
            }
            i++;
         }
      }

      if (!entity || !dst->IsEnabled() || solutionTime_ != dst->GetLastModificationTime()) {
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

