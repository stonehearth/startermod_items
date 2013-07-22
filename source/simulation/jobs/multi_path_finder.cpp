#include "pch.h"
#include <ostream>
#include "metrics.h"
#include "multi_path_finder.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/destination.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

MultiPathFinder::MultiPathFinder(std::string name) :
   Job(name),
   reversed_search_(false),
   enabled_(true)
{
}

MultiPathFinder::~MultiPathFinder()
{
}

void MultiPathFinder::SetEnabled(bool enabled)
{
   enabled_ = enabled;
}

bool MultiPathFinder::IsIdle() const
{
   if (!enabled_) {
      return true;
   }

   // xxx: don't run if any of these have a solution!
   for (const auto& entry : pathfinders_) {
      if (!entry.second->IsIdle()) {
         return false;
      }
   }
   return true;
}

void MultiPathFinder::AddEntity(om::EntityRef e, luabind::object solved_cb, luabind::object dst_filter)
{
   PROFILE_BLOCK();

   auto entity = e.lock();
   if (entity) {
      om::EntityId id = entity->GetEntityId();

      auto i = pathfinders_.find(id);
      if (i == pathfinders_.end()) {
         std::ostringstream name;

         name << GetName() << "(entity " << id << ")";
         auto pathfinder = std::make_shared<PathFinder>(name.str(), e, solved_cb, dst_filter);
         for (auto& entry: destinations_) {
            pathfinder->AddDestination(entry.second);
         }
         pathfinder->SetReverseSearch(reversed_search_);
         pathfinders_[id] = pathfinder;
      } else {
         LOG(WARNING) << "adding duplicate entity " << entity->GetObjectId() << " to path finder " << GetName() << ".  ignoring.";
      }
   }
}

void MultiPathFinder::RemoveEntity(om::EntityId id)
{
   PROFILE_BLOCK();

   auto i = pathfinders_.find(id);
   if (i != pathfinders_.end()) {
      pathfinders_.erase(id);
   }
}

void MultiPathFinder::AddDestination(om::EntityRef d)
{
   PROFILE_BLOCK();

   auto dst = d.lock();
   if (dst) {
      destinations_[dst->GetObjectId()] = dst;
      for (auto& entry : pathfinders_) {
         entry.second->AddDestination(dst);
      }
   }
}

void MultiPathFinder::RemoveDestination(dm::ObjectId id)
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

void MultiPathFinder::SetReverseSearch(bool reversed)
{
   reversed_search_ = reversed;
   for (auto const& entry : pathfinders_) {
      entry.second->SetReverseSearch(reversed);
   }
}

void MultiPathFinder::EncodeDebugShapes(radiant::protocol::shapelist *msg) const
{
   PROFILE_BLOCK();

   for (auto& entry : pathfinders_) {
      entry.second->EncodeDebugShapes(msg);
   }
}

void MultiPathFinder::Work(const platform::timer &timer)
{
   PROFILE_BLOCK();

   ASSERT(!pathfinders_.empty());
   
   int cost = INT_MAX;

   PathFinderMap::iterator i = pathfinders_.begin(), end = pathfinders_.end(), closest = end;
   while (i != end) {
      PathFinderPtr p = i->second;

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

      ASSERT(!p->IsIdle());
      p->Work(timer);
   }
}

void MultiPathFinder::LogProgress(std::ostream& o) const
{
   for (auto& entry : pathfinders_) {
      entry.second->LogProgress(o);
   }
}

std::ostream& ::radiant::simulation::operator<<(std::ostream& o, const MultiPathFinder& pf)
{
   return pf.Format(o);
}

std::ostream& MultiPathFinder::Format(std::ostream& o) const
{
   o << "[mpf " << GetName() << " " << pathfinders_.size() 
     << " entities finding " << destinations_.size() << " locations";
   return o;        
}

