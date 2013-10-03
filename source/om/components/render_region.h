#ifndef _RADIANT_OM_RENDER_REGION_H
#define _RADIANT_OM_RENDER_REGION_H

#include "dm/record.h"
#include "dm/set.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "om/om.h"
#include "om/object_enums.h"
#include "om/region.h"
#include "csg/cube.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderRegion : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderRegion, terrain);
   
   dm::Boxed<BoxedRegion3Ptr> const& GetRegion() const { return region_; }
   void SetRegion(BoxedRegion3Ptr r);

   dm::Guard TraceRenderRegion(const char* reason, std::function<void(csg::Region3 const&)> cb) const;

private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<BoxedRegion3Ptr>    region_;
};

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_RENDER_REGION_H
