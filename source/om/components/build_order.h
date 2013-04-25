#ifndef _RADIANT_OM_BUILD_ORDER_H
#define _RADIANT_OM_BUILD_ORDER_H

#include "om/region.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class BuildOrder : public Component
{
public: // Simulation Methods
   virtual std::string GetMaterial() { return "wood"; }
   virtual bool ShouldTearDown() { return false; }
   virtual bool NeedsMoreWork() = 0;
   virtual const RegionPtr GetConstructionRegion() const = 0;
   virtual bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) = 0;
   virtual void ConstructBlock(const math3d::ipoint3& block) = 0;
   virtual void StartProject(const dm::CloneMapping& mapping) = 0;
   virtual void CompleteTo(int percent) = 0;
public: // Creation Methods
   virtual void UpdateShape() { }

public: // Tracking Methods
   virtual dm::Guard TraceRegion(const char* reason, std::function<void(const csg::Region3&)> cb) const = 0;
   virtual dm::Guard TraceReserved(const char* reason, std::function<void(const csg::Region3&)> cb) const = 0;
   virtual const csg::Region3& GetRegion() const = 0;
   virtual const csg::Region3& GetReservedRegion() const = 0;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_BUILD_ORDER_H
