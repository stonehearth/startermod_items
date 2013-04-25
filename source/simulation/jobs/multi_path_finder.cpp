#include "pch.h"
#include <ostream>
#include "metrics.h"
#include "multi_path_finder.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

MultiPathFinder::MultiPathFinder(std::string name) :
   Job(name),
   solved_(0),
   enabled_(true)
{
}

MultiPathFinder::~MultiPathFinder()
{
}

bool MultiPathFinder::IsIdle(int now) const
{
   if (!enabled_) {
      return true;
   }

   auto solution = GetSolution();
   if (!solution) {
      for (const auto& entry : pathfinders_) {
         if (!entry.second->IsIdle(now)) {
            return false;
         }
      }
   }
   return true;
}

void MultiPathFinder::Restart()
{
   for (const auto& entry : pathfinders_) {
      entry.second->Restart();
   }
}

void MultiPathFinder::AddEntity(om::EntityRef e)
{
   PROFILE_BLOCK();

   auto entity = e.lock();

   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         math3d::ipoint3 location = math3d::ipoint3(mob->GetLocation());
         om::EntityId id = entity->GetEntityId();

         auto i = pathfinders_.find(id);
         if (i == pathfinders_.end()) {
            std::ostringstream name;

            name << GetName() << "(entity " << id << ")";
            auto pathfinder = std::make_shared<PathFinder>(name.str(), false);
            for (auto& entry: destinations_) {
               pathfinder->AddDestination(entry.second);
            }
            pathfinders_[id] = pathfinder;
            pathfinder->Start(e, location);
         }
      }
   }
}

void MultiPathFinder::RemoveEntity(om::EntityId id)
{
   PROFILE_BLOCK();

   auto i = pathfinders_.find(id);
   if (i != pathfinders_.end()) {
      if (solved_ == id) {
         LOG(WARNING) << "(" << GetName() << ") removing solution entity from multi-search.  resuming!";
         solved_ = 0;
      }
      pathfinders_.erase(id);
   }
}

void MultiPathFinder::AddDestination(DestinationPtr dst)
{
   PROFILE_BLOCK();

   destinations_[dst->GetEntityId()] = dst;
   for (auto& entry : pathfinders_) {
      entry.second->AddDestination(dst);
   }
}

DestinationPtr MultiPathFinder::GetDestination(om::EntityId id)
{
   PROFILE_BLOCK();

   auto i = destinations_.find(id);
   return (i != destinations_.end()) ? i->second : nullptr;
}

int MultiPathFinder::GetActiveDestinationCount() const
{
   int count = 0;
   for (const auto& entry : destinations_) {
      if (entry.second->IsEnabled()) {
         count++;
      }
   }
   return count;
}

void MultiPathFinder::RemoveDestination(om::EntityId id)
{
   PROFILE_BLOCK();

   auto i = destinations_.find(id);
   if (i != destinations_.end()) {
      destinations_.erase(i);
   }
   for (auto& entry : pathfinders_) {
      entry.second->RemoveDestination(id);
   }
}

void MultiPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   PROFILE_BLOCK();

   for (auto& entry : pathfinders_) {
      entry.second->EncodeDebugShapes(msg);
   }
   for (auto& entry : destinations_) {
      entry.second->EncodeDebugShapes(msg);
   }
}

void MultiPathFinder::Work(int now, const platform::timer &timer)
{
   PROFILE_BLOCK();

   ASSERT(!solved_);
   ASSERT(!pathfinders_.empty());
   
   int cost = INT_MAX;

   PathFinderMap::iterator i = pathfinders_.begin(), end = pathfinders_.end(), closest = end;
   while (i != end) {
      PathFinderPtr p = i->second;

      ASSERT(p->GetState() != PathFinder::SOLVED);
      int c = p->EstimateCostToSolution();
      if (c < cost) {
         closest = i;
         cost = c;
      }
      i++;
   }

   if (closest != end) {
      om::EntityId id = closest->first;
      PathFinderPtr p = closest->second;

      ASSERT(!p->IsIdle(now));
      p->Work(now, timer);
      if (p->GetState() == PathFinder::SOLVED) {
         solved_ = id;
      }
   }
}

void MultiPathFinder::LogProgress(std::ostream& o) const
{
   for (auto& entry : pathfinders_) {
      entry.second->LogProgress(o);
   }
}

PathPtr MultiPathFinder::GetSolution() const
{
   PROFILE_BLOCK();

   if (!solved_) {
      return nullptr;
   }
   auto i = pathfinders_.find(solved_);
   if (i == pathfinders_.end()) {
      solved_ = 0;
      return nullptr;
   }
   auto path = i->second->GetSolution();
   if (!path) {
      solved_ = 0;
      return nullptr;
   }
   return path;
}

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const MultiPathFinder& pf)
{
   return pf.Format(o);
}

std::ostream& MultiPathFinder::Format(std::ostream& o) const
{
   o << "[mpf " << GetName() << " " << pathfinders_.size() 
     << " entities finding " << destinations_.size() << " locations ("
     << GetActiveDestinationCount() << " active)";
   return o;        
}

