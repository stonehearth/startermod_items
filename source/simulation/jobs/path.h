#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "math3d.h"
#include "simulation/namespace.h"
#include "destination.h"
#include "om/om.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(const PointList& points, om::EntityRef source, DestinationPtr destination) :
         points_(points),
         source_(source),
         destination_(destination)
      {
      }

      const std::vector<math3d::ipoint3> &GetPoints() const {
         return points_;
      }

      DestinationPtr GetDestination() const {
         return destination_;
      }

      om::EntityRef GetEntity() const { return source_; }
      std::ostream& Format(std::ostream& os) const;

   protected:
      PointList               points_;
      om::EntityRef           source_;
      DestinationPtr          destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
