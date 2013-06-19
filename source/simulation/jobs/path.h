#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "math3d.h"
#include "simulation/namespace.h"
#include "om/om.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(const std::vector<math3d::ipoint3>& points, om::EntityRef source, om::DestinationRef destination) :
         points_(points),
         source_(source),
         destination_(destination)
      {
      }

      std::vector<math3d::ipoint3> const& GetPoints() const {
         return points_;
      }

      om::DestinationRef GetDestination() const {
         return destination_;
      }

      om::EntityRef GetEntity() const { return source_; }
      std::ostream& Format(std::ostream& os) const;

   protected:
      std::vector<math3d::ipoint3>  points_;
      om::EntityRef                 source_;
      om::DestinationRef            destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
