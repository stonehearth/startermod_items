#ifndef _RADIANT_SIMULATION_JOBS_MULTI_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_MULTI_PATH_FINDER_H

#include <unordered_map>
#include "path_finder.h"
#include "om/om.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class MultiPathFinder : public Job {
   public:
      MultiPathFinder(std::string name);
      virtual ~MultiPathFinder();

      typedef std::unordered_map<om::EntityId, om::EntityRef> DestinationMap;

	   void AddEntity(om::EntityRef actor, luabind::object solved_cb, luabind::object dst_filter);
      void RemoveEntity(om::EntityId actor);

      void AddDestination(om::EntityRef dst);
      void RemoveDestination(dm::ObjectId id);
      const DestinationMap& GetDestinations() const { return destinations_; }

      void SetReverseSearch(bool reversed);

   public: // Job Interface
      bool IsIdle(int now) const override;
      bool IsFinished() const override { return false; }
      void Work(int now, const platform::timer &timer) override;
      void LogProgress(std::ostream&) const override;
      void EncodeDebugShapes(radiant::protocol::shapelist *msg) const override;

   public:
      std::ostream& Format(std::ostream& o) const;

   private:
      typedef std::unordered_map<om::EntityId, PathFinderPtr> PathFinderMap;
      PathFinderMap                    pathfinders_;
      DestinationMap                   destinations_;
      bool                             reversed_search_;
};

std::ostream& operator<<(std::ostream& o, const MultiPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_MULTI_PATH_FINDER_H

