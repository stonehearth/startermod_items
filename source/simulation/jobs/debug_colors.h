#ifndef _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H
#define _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H

#include "math3d/common/color.h"
#include "simulation/namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class DebugColors {
   public:
      static math3d::color4 standingColor;
      static math3d::color4 standingColorDisabled;
      static math3d::color4 destinationColor;
      static math3d::color4 reservedDestinationColor;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_DEBUG_COLORS_H

