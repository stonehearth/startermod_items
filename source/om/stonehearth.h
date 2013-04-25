#ifndef _RADIANT_OM_STONEHEARTH_H
#define _RADIANT_OM_STONEHEARTH_H

#include "om.h"
#include "dm/dm.h"
#include "csg/region.h"


// This file clearly does not belong in the om.

BEGIN_RADIANT_OM_NAMESPACE

class Stonehearth
{
public:
   static om::EntityPtr CreateEntity(dm::Store& store, std::string kind);
   static csg::Region3 ComputeStandingRegion(const csg::Region3& r, int height);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_STONEHEARTH_H
