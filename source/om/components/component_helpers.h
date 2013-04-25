#ifndef _RADIANT_OM_COMPONENT_HELPERS_H
#define _RADIANT_OM_COMPONENT_HELPERS_H

#include "mob.h"
#include "csg/point.h"

BEGIN_RADIANT_OM_NAMESPACE

static inline csg::Point3 GetEntityGridLocation(Entity& e) {
   return e.GetComponent<Mob>()->GetGridLocation();
}

static inline csg::Point3 GetEntityGridLocation(std::shared_ptr<Entity> e) {
   return GetEntityGridLocation(*e);
}

static inline csg::Point3 GetEntityGridLocation(std::shared_ptr<Component> c) {
   return GetEntityGridLocation(c->GetEntity());
}

static inline csg::Point3 GetEntityWorldGridLocation(Entity& e) {
   return e.GetComponent<Mob>()->GetWorldGridLocation();
}

static inline csg::Point3 GetEntityWorldGridLocation(std::shared_ptr<Entity> e) {
   return GetEntityWorldGridLocation(*e);
}

static inline csg::Point3 GetEntityWorldGridLocation(std::shared_ptr<Component> c) {
   return GetEntityWorldGridLocation(c->GetEntity());
}

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COMPONENT_HELPERS_H
