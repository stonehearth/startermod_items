#ifndef _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H
#define _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H

#include "csg/color.h"
#include "simulation/namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class DebugColors {
   public:
      static csg::Color4 standingColor;
      static csg::Color4 standingColorDisabled;
      static csg::Color4 destinationColor;
      static csg::Color4 reservedDestinationColor;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H

