#ifndef _RADIANT_OM_UNIT_FRAME_H
#define _RADIANT_OM_UNIT_FRAME_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class UnitInfo : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(UnitInfo);
   void Construct(json::ConstJsonObject const& obj) override;

   std::string GetDisplayName() const { return *name_; }
   void SetDisplayName(std::string name) { name_ = name; }

   std::string GetDescription() const { return *description_; }
   void SetDescription(std::string description) { description_ = description; }

   std::string GetFaction() const { return *faction_; }
   void SetFaction(std::string faction) { faction_ = faction; }

   dm::Guard TraceName(const char* reason, std::function<void()> fn) { return name_.TraceObjectChanges(reason, fn); };
   dm::Guard TraceDescription(const char* reason, std::function<void()> fn) { return description_.TraceObjectChanges(reason, fn); };

private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<std::string>  name_;
   dm::Boxed<std::string>  description_;
   dm::Boxed<std::string>  faction_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_UNIT_FRAME_H
