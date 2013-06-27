#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "math3d.h"
#include "simulation/namespace.h"
#include "om/om.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(const std::vector<math3d::ipoint3>& points, om::EntityRef source, om::EntityRef destination, math3d::ipoint3 const& start, math3d::ipoint3 const& finish) :
         points_(points),
         source_(source),
         destination_(destination),
         start_pt_(start),
         finish_pt_(finish)
      {
      }

      std::vector<math3d::ipoint3> const& GetPoints() const {
         return points_;
      }

      om::EntityRef GetDestination() const { return destination_; }
      om::EntityRef GetSource() const { return source_; }
      math3d::ipoint3 GetStartPoint() const { return start_pt_; }
      math3d::ipoint3 GetFinishPoint() const { return finish_pt_; }
      std::ostream& Format(std::ostream& os) const;

   protected:
      std::vector<math3d::ipoint3>  points_;
      math3d::ipoint3               start_pt_;
      math3d::ipoint3               finish_pt_;
      om::EntityRef                 source_;
      om::EntityRef                 destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
