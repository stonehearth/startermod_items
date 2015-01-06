#ifndef _RADIANT_SIMULATION_JOBS_PATH_H
#define _RADIANT_SIMULATION_JOBS_PATH_H

#include "simulation/namespace.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path {
   public:
      Path(std::vector<csg::Point3f> const& points, om::EntityRef source, om::EntityRef destination, csg::Point3f const& poi);

      std::vector<csg::Point3f> const& GetPoints() const {
         return points_;
      }

      std::vector<csg::Point3f> const& GetPrunedPoints();

      bool IsEmpty() const { return points_.size() == 0; }
      double GetDistance() const;
      om::EntityRef GetDestination() const { return destination_; }
      om::EntityRef GetSource() const { return source_; }
      csg::Point3f GetStartPoint() const;
      csg::Point3f GetFinishPoint() const;
      csg::Point3f GetDestinationPointOfInterest() const { return poi_; }

      std::ostream& Format(std::ostream& os) const;

      static PathPtr CombinePaths(std::vector<PathPtr> const& paths);
      static PathPtr CreatePathSubset(PathPtr path, int startIndex, int finishIndex);

   private:
      csg::Point3f GetSourceLocation() const;

   private:
      int                       id_;
      std::vector<csg::Point3f> points_;
      std::vector<csg::Point3f> prunedPoints_;
      csg::Point3f              poi_;
      om::EntityRef             source_;
      om::EntityRef             destination_;
};

typedef std::weak_ptr<Path> PathRef;
typedef std::shared_ptr<Path> PathPtr;

std::ostream& operator<<(std::ostream& os, const Path& in);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_H
