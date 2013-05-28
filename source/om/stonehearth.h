#ifndef _RADIANT_OM_STONEHEARTH_H
#define _RADIANT_OM_STONEHEARTH_H

#include "om.h"
#include "dm/dm.h"
#include "csg/region.h"
#include "radiant_json.h"

// This file clearly does not belong in the om.

BEGIN_RADIANT_OM_NAMESPACE

class Stonehearth
{
public:
   // xxx this file has no reason to exist anymore now that lua handles most of this
   static csg::Region3 ComputeStandingRegion(const csg::Region3& r, int height);
   static om::EntityPtr CreateEntityLegacyDIEDIEDIE(dm::Store& store, std::string uri);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_STONEHEARTH_H
