#ifndef _RADIANT_PHYSICS_DESTINATION_TRACKER_H
#define _RADIANT_PHYSICS_DESTINATION_TRACKER_H

#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class DestinationTracker : public RegionTracker<om::Region3fBoxed>
{
public:
   DestinationTracker(NavGrid& ng, om::EntityPtr entity, om::DestinationPtr dst);
   virtual ~DestinationTracker();

   void Initialize() override;   

protected: // RegionTracker implementation
   om::Region3fBoxedPtr GetRegion() const override;

private:
   om::DestinationRef      dst_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_DESTINATION_TRACKER_H
