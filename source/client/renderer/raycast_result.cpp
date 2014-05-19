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

RaycastResult::RaycastResult(csg::Ray3 const& ray) :
   _ray(ray)
{
}

int RaycastResult::GetNumResults() const
{
   return _results.size();
}

RaycastResult::Result RaycastResult::GetResult(uint i) const
{
   if (i >= _results.size()) {
      throw std::logic_error(BUILD_STRING("requesting raycast result out-of-bounds " << i));
   }
   return _results[i];
}

std::vector<RaycastResult::Result> const& RaycastResult::GetResults() const
{
   return _results;
}

csg::Ray3 const& RaycastResult::GetRay() const
{
   return _ray;
}

void RaycastResult::AddResult(csg::Point3f const &intersection, csg::Point3f const& normal, csg::Point3 const& brick, om::EntityRef entity)
{
   Result r;
   r.intersection = intersection;
   r.normal = normal;
   r.brick = brick;
   r.entity = entity;
   _results.emplace_back(r);
}

#if 0
bool RaycastResult::isValidBrick(uint i) const
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
#endif