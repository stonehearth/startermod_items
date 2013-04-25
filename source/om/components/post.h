#ifndef _RADIANT_OM_POST_H
#define _RADIANT_OM_POST_H

#include "math3d.h"
#include "om/om.h"
#include "om/grid/grid.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/object.h"
#include "dm/store.h"
#include "namespace.h"
#include "component.h"
#include "csg/region.h"
#include "grid_build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class Post : public GridBuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Post);

   Post();
   virtual ~Post();

   void Create(int height);
   void StartProject(const dm::CloneMapping& mapping) override;

   int GetHeight() const { return height_; }

public:
   void ConstructBlock(const math3d::ipoint3& block) override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;

private:
   void InitializeRecordFields() override;
   void ComputeStandingRegion();

private:
   dm::Boxed<int>             height_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_POST_H
