#ifndef _RADIANT_OM_FIXTURE_H
#define _RADIANT_OM_FIXTURE_H

#include "build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class Fixture : public BuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Fixture, fixture);
   void ExtendObject(json::ConstJsonObject const& obj) override;

public: // Simulation Methods
   bool NeedsMoreWork() override;
   const RegionPtr GetConstructionRegion() const override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void ConstructBlock(const math3d::ipoint3& block) override;
   void StartProject(const dm::CloneMapping& mapping) override;
   void CompleteTo(int percent) override;

public: // Tracking Methods
   dm::Guard TraceRegion(const char* reason, std::function<void(const csg::Region3&)> cb) const override;
   dm::Guard TraceReserved(const char* reason, std::function<void(const csg::Region3&)> cb) const override;
   const csg::Region3& GetRegion() const override;
   const csg::Region3& GetReservedRegion() const override;

public:
   void SetItem(EntityRef item);
   EntityRef GetItem() const { return *item_; }
   std::string GetItemKind() { return *kind_; }
   void SetItemKind(std::string kind) { kind_ = kind; }

private:
   void InitializeRecordFields() override;

protected:
   dm::Boxed<EntityPtr>    item_;
   dm::Boxed<bool>         reserved_;
   dm::Boxed<std::string>  kind_;
   dm::Boxed<RegionPtr>    standingRegion_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_FIXTURE_H
