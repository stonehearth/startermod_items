#ifndef _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H
#define _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H

#include "namespace.h"
#include "dm/dm.h"
#include "om/namespace.h"
#include "csg/csg.h"
#include "csg/ray.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RaycastResult
{
   public:
      class Result {
      public:
         csg::Point3f         intersection;
         csg::Point3f         normal;
         csg::Point3          brick;
         om::EntityRef        entity;
      };

   public:
      RaycastResult(csg::Ray3 const& ray);

      int GetNumResults() const;
      Result GetResult(uint i) const;
      std::vector<Result> const& GetResults() const;
      csg::Ray3 const& GetRay() const;

      void AddResult(csg::Point3f const &intersection, csg::Point3f const& normal, csg::Point3 const& brick, om::EntityRef entity);

   private:
      std::vector<Result>  _results;
      csg::Ray3            _ray;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H
