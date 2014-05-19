#ifndef _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H

#include "region_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/* 
 * -- DerivedRegionTracker
 *
 * A template for encapsulating a region into a CollisionTracker.
 *
 * The encapsulated class must implement (this should probably be declared as an interface):
 *    Region3BoxedPtr const& GetRegion();
 *    DeepRegionGuardPtr TraceRegion(const char* reason, int category);
 *
 * See collision_tracker.h for more details.
 */

template <class T, TrackerType OT>
class DerivedRegionTracker : public RegionTracker
{
public:
   DerivedRegionTracker(NavGrid& ng, om::EntityPtr entity, std::shared_ptr<T> regionProviderPtr);
   virtual ~DerivedRegionTracker() { }

   void Initialize() override;
   TrackerType GetType() const override;

protected:
   om::Region3BoxedPtr GetRegion() const override;

private:
   NO_COPY_CONSTRUCTOR(DerivedRegionTracker)

private:
   std::weak_ptr<T> regionProviderRef_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
