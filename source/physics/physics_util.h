#ifndef _RADIANT_PHYSICS_UTIL_H
#define _RADIANT_PHYSICS_UTIL_H

#include "namespace.h"
#include "om/om.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

template <typename Shape> Shape LocalToWorld(Shape const& shape, om::EntityPtr entity);
template <typename Shape> Shape WorldToLocal(Shape const& shape, om::EntityPtr entity);

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_UTIL_H
