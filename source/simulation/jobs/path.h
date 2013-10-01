#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "simulation/namespace.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(const std::vector<csg::Point3>& points, om::EntityRef source, om::EntityRef destination, csg::Point3 const& start, csg::Point3 const& finish) :
         points_(points),
         source_(source),
         destination_(destination),
         start_pt_(start),
         finish_pt_(finish)
      {
      }

      std::vector<csg::Point3> const& GetPoints() const {
         return points_;
      }

      om::EntityRef GetDestination() const { return destination_; }
      om::EntityRef GetSource() const { return source_; }
      csg::Point3 GetStartPoint() const { return points_.empty() ? csg::Point3(0, 0, 0) : points_.front(); }
      csg::Point3 GetFinishPoint() const { return points_.empty() ? csg::Point3(0, 0, 0) : points_.back(); }
      csg::Point3 GetSourcePointOfInterest() const { return start_pt_; }
      csg::Point3 GetDestinationPointOfInterest() const { return finish_pt_; }
      std::ostream& Format(std::ostream& os) const;

   protected:
      std::vector<csg::Point3>  points_;
      csg::Point3               start_pt_;
      csg::Point3               finish_pt_;
      om::EntityRef                 source_;
      om::EntityRef                 destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
