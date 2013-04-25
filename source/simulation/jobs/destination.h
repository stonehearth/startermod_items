#ifndef _RADIANT_SIMULATION_JOBS_DESTINATION_H
#define _RADIANT_SIMULATION_JOBS_DESTINATION_H

#include "math3d.h"
#include "namespace.h"
#include "radiant.pb.h"
#include "om/om.h"

/*
 * xxx: There's a big flaw in the multipathfinder.
 *
 * Destination objects are shared among all pathfinders.  This has
 * two big benefits:
 *
 * 1) Path finding is much faster when there are many actors in the
 *    same multipathfinder, since we only have to recompute destination
 *    standing positions once for all actors.
 * 2) The destination object can be the place where we stick "reserved"
 *    spaces (i.e. locations handed out by previous solutions to this
 *    destination).
 *
 * If the pickup, dropoff, or capabilties of all the actors isn't
 * identical, this obviously won't work!  
 */

BEGIN_RADIANT_SIMULATION_NAMESPACE

typedef std::vector<math3d::ipoint3> PointList;

class Destination 
{
public:
   Destination(om::EntityRef e);
   virtual ~Destination() { }

   om::EntityId GetEntityId() { return entityId_; }
   om::EntityRef GetEntity() const { return entity_; }

   virtual void SetEnabled(bool enabled) { enabled_ = enabled; }
   virtual bool IsEnabled() const { return enabled_; }
   virtual int GetLastModificationTime() const = 0;
   virtual int EstimateCostToDestination(const math3d::ipoint3& from) = 0;
   virtual void EncodeDebugShapes(radiant::protocol::shapelist *msg) const = 0;

private:
   bool           enabled_;
   om::EntityRef  entity_;
   om::EntityId   entityId_;
};

typedef std::weak_ptr<Destination> DestinationRef;
typedef std::shared_ptr<Destination> DestinationPtr;

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_DESTINATION_H

