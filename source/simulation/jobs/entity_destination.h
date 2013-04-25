#ifndef _RADIANT_SIMULATION_JOBS_ENITTY_DESTINATION_H
#define _RADIANT_SIMULATION_JOBS_ENITTY_DESTINATION_H

#include "destination.h"
#include "om/om.h"
#include "om/entity.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class EntityDestination : public Destination
{
   public:
      EntityDestination(om::EntityRef, const PointList& standing);

   public:
      int GetLastModificationTime() const override;
      int EstimateCostToDestination(const math3d::ipoint3& from) override;
      void EncodeDebugShapes(radiant::protocol::shapelist *msg) const override;

   private:
      om::EntityRef        entity_;
      PointList            dst_;
      int                  modified_;
};


END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_ENITTY_DESTINATION_H

