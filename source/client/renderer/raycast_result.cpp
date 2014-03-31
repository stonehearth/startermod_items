#include "pch.h"
#include "raycast_result.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "lib/perfmon/perfmon.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define R_LOG(level)      LOG(renderer.renderer, level)

RaycastResult::RaycastResult()
{
}

bool RaycastResult::isValid() const
{
   return numResults() > 0;
}

int RaycastResult::numResults() const
{
   return _intersections.size();
}

const csg::Point3f RaycastResult::intersectionOf(int i) const
{
   return _intersections[i];
}

void RaycastResult::addIntersection(const csg::Point3f& p, const csg::Point3f& normal, dm::ObjectId objId)
{
   _intersections.push_back(p);
   _normals.push_back(normal);
   _objIds.push_back(objId);

   csg::Point3 brick;
   // Calculate brick location!
   for (int i = 0; i < 3; i++) {
      // The brick origin is at the center of mass.  Adding 0.5f to the
      // coordinate and flooring it should return a brick coordinate.
      brick[i] = (int)std::floor(p[i] + 0.5f);

      // We want to choose the brick that the mouse is currently over.  The
      // intersection point is actually a point on the surface.  So to get the
      // brick, we need to move in the opposite direction of the normal
      if (fabs(normal[i]) > csg::k_epsilon) {
         brick[i] += normal[i] > 0 ? -1 : 1;
      }
   }
   _bricks.push_back(brick);
}

const csg::Point3f RaycastResult::normalOf(int i) const
{
   return _normals[i];
}

dm::ObjectId RaycastResult::objectIdOf(int i) const
{
   return _objIds[i];
}

const csg::Ray3 RaycastResult::ray() const
{
   return _ray;
}

void RaycastResult::setRay(const csg::Ray3& ray)
{
   _ray = ray;
}

const csg::Point3 RaycastResult::brickOf(int i) const
{
   return _bricks[i];
}

RaycastResult::~RaycastResult()
{

}

