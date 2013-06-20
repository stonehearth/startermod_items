#ifndef _RADIANT_OM_WALL_H
#define _RADIANT_OM_WALL_H

#include "om/om.h"
#include "dm/set.h"
#include "grid_build_order.h"
#include "scaffolding.h"
#include "post.h"

BEGIN_RADIANT_OM_NAMESPACE

class Wall : public GridBuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Wall);

   void Create(PostPtr post1, PostPtr post2, const csg::Point3& normal);
   csg::Point3 GetNormal() const { return *normal_; }
   csg::Point3 GetSize() const { return *size_; }

   void StencilOutPortals();

   void AddFixture(om::EntityRef f);

public: // BuildOrder overrides
   void ConstructBlock(const math3d::ipoint3& block) override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void StartProject(const dm::CloneMapping& mapping) override;
   void Resize(const csg::Region3& roofRegion);

private:
   void InitializeRecordFields() override;
   void ComputeStandingRegion();
   void CommitRegion(const csg::Region3& oldRegion);
   int GetColor(const csg::Point3& loc);

private:
   dm::Boxed<csg::Point3>     normal_;
   dm::Boxed<csg::Point3>     size_; // can probably be optimized out...
   dm::Boxed<PostRef>         post1_;
   dm::Boxed<PostRef>         post2_;
   dm::Set<EntityPtr>         fixtures_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_WALL_H
