#ifndef _RADIANT_SIMULATION_JOBS_REGION_DESTINATION_H
#define _RADIANT_SIMULATION_JOBS_REGION_DESTINATION_H

#include "destination.h"
#include "om/om.h"
#include "om/entity.h"
#include "dm/boxed.h"
#include "csg/region.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RegionDestination : public Destination
{
   public:
      RegionDestination(om::EntityRef, const om::RegionPtr region);

   public:
      int GetLastModificationTime() const override;
      int EstimateCostToDestination(const math3d::ipoint3& from) override;
      void EncodeDebugShapes(radiant::protocol::shapelist *msg) const override;
      bool IsEnabled() const override;

   private:
      const csg::Region3& GetRegion() const;

   private:
      const om::RegionPtr  region_;
};


END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_REGION_DESTINATION_H

