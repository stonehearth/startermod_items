#ifndef _RADIANT_OM_RENDER_REGION_H
#define _RADIANT_OM_RENDER_REGION_H

#include "math3d.h"
#include "dm/record.h"
#include "dm/set.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "om/om.h"
#include "om/grid/grid.h"
#include "om/all_object_types.h"
#include "om/region.h"
#include "csg/cube.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderRegion : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderRegion, terrain);
   static void RegisterLuaType(struct lua_State* L);
   std::string ToString() const;
   
   csg::Region3& ModifyRegion();
   const csg::Region3& GetRegion() const;
   RegionPtr AllocRegion();
   RegionPtr GetRegionPointer() const;
   void SetRegionPointer(RegionPtr r);

   dm::Boxed<RegionPtr> const& GetBoxedRegion() const;

private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<RegionPtr>    region_;
};

static std::ostream& operator<<(std::ostream& os, const RenderRegion& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_RENDER_REGION_H
