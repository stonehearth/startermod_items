#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "simulation/namespace.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(const std::vector<csg::Point3>& points, om::EntityRef source, om::EntityRef destination, csg::Point3 const& poi);

      std::vector<csg::Point3> const& GetPoints() const {
         return points_;
      }

      bool IsEmpty() const { return points_.size() == 0; }
      float GetDistance() const;
      om::EntityRef GetDestination() const { return destination_; }
      om::EntityRef GetSource() const { return source_; }
      csg::Point3 GetStartPoint() const;
      csg::Point3 GetFinishPoint() const;
      csg::Point3 GetDestinationPointOfInterest() const { return poi_; }
      std::ostream& Format(std::ostream& os) const;

   private:
      csg::Point3 GetSourceLocation() const;

   private:
      int                       id_;
      std::vector<csg::Point3>  points_;
      csg::Point3               poi_;
      om::EntityRef             source_;
      om::EntityRef             destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
