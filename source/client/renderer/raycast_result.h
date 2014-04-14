#ifndef _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H
#define _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H

#include "namespace.h"
#include "dm/dm.h"
#include "csg/csg.h"
#include "csg/ray.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RaycastResult
{
   public:
      RaycastResult();
      ~RaycastResult();

      int numResults() const;
      bool isValid() const;
      
      const csg::Point3f intersectionOf(uint i) const;
      const csg::Point3f normalOf(uint i) const;
      dm::ObjectId objectIdOf(uint i) const;
      const csg::Point3 brickOf(uint i) const; 
      bool isValidBrick(uint i) const;

      const csg::Ray3 ray() const;

      void addIntersection(const csg::Point3f& point, const csg::Point3f& normal, dm::ObjectId objId);
      void setRay(const csg::Ray3& ray);

   private:
      std::vector<csg::Point3f> _intersections;
      std::vector<csg::Point3f> _normals;
      std::vector<dm::ObjectId> _objIds;
      std::vector<csg::Point3>  _bricks;
      csg::Ray3                 _ray;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RAYCAST_RESULT_H
