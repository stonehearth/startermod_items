#ifndef _RADIANT_PHYSICS_UTIL_H
#define _RADIANT_PHYSICS_UTIL_H

#include "namespace.h"
#include "om/om.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

template <typename Shape> Shape LocalToWorld(Shape const& shape, om::EntityPtr entity);
template <typename Shape> Shape WorldToLocal(Shape const& shape, om::EntityPtr entity);

// returns whether a voxel with a center at the specified coordinate is aligned to the terrain
bool IsTerrainAligned(float voxelCenter);

float GetTerrainAlignmentOffset(float coordinate);

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_UTIL_H
