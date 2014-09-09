#ifndef _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
#define _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H

#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class VerticalPathingRegionTracker : public RegionTracker<om::Region3Boxed>
{
public:
   VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr dst);
   virtual ~VerticalPathingRegionTracker();

   void Initialize() override;   

protected: // RegionTracker implementation
   om::Region3BoxedPtr GetRegion() const override;

private:
   om::VerticalPathingRegionRef      vpr_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
