#include "pch.h"
#include "raycast_result.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "lib/perfmon/perfmon.h"
#include "client/client.h"
#include "client/renderer/renderer.h"

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
   if (i >= _intersections.size()) {
      throw std::logic_error(BUILD_STRING("invalid index " << i << "in RaycastResult::intersectionOf"));
   }
   return _intersections[i];
}

void RaycastResult::addIntersection(const csg::Point3f& p, const csg::Point3f& normal, dm::ObjectId objId)
{
   _intersections.push_back(p);
   _normals.push_back(normal);
   _objIds.push_back(objId);
}

const csg::Point3f RaycastResult::normalOf(int i) const
{
   if (i >= _normals.size()) {
      throw std::logic_error(BUILD_STRING("invalid index " << i << "in RaycastResult::normalOf"));
   }
   return _normals[i];
}

dm::ObjectId RaycastResult::objectIdOf(int i) const
{
   if (i >= _objIds.size()) {
      throw std::logic_error(BUILD_STRING("invalid index " << i << "in RaycastResult::objectIdOf"));
   }
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
   if (i >= _objIds.size() || i >= _normals.size() || i >= _intersections.size()) {
      throw std::logic_error(BUILD_STRING("invalid index " << i << "in RaycastResult::brickOf"));
   }
   csg::Matrix4 nodeTransform = Renderer::GetInstance().GetTransformForObject(_objIds[i]);
   nodeTransform.affine_inverse();
   csg::Point3f normal = nodeTransform.rotate(_normals[i]);
   csg::Point3f intersection = nodeTransform.transform(_intersections[i]);

   csg::Point3 brick;
   // Calculate brick location!
   for (int j = 0; j < 3; j++) {
      // The brick origin is at the center of mass.  Adding 0.5f to the
      // coordinate and flooring it should return a brick coordinate.
      brick[j] = (int)std::floor(intersection[j] + 0.5f);

      // We want to choose the brick that the mouse is currently over.  The
      // intersection point is actually a point on the surface.  So to get the
      // brick, we need to move in the opposite direction of the normal
      if (fabs(normal[j]) > csg::k_epsilon) {
         brick[j] += normal[j] > 0 ? -1 : 1;
      }
   }
   return brick;
}

bool RaycastResult::isValidBrick(int i) const
{
   if (i >= _objIds.size()) {
      return false;
   }
   om::EntityPtr terrain = Client::GetInstance().GetEntity(_objIds[i]);
   if (!terrain) {
      return false;
   }
   return terrain->GetComponent<om::Terrain>() != nullptr;
}

RaycastResult::~RaycastResult()
{

}

