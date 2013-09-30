#ifndef _RADIANT_OM_UNIT_FRAME_H
#define _RADIANT_OM_UNIT_FRAME_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/map.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class UnitInfo : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(UnitInfo, unit_info);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   std::string GetDisplayName() const { return *name_; }
   void SetDisplayName(std::string name) { name_ = name; }

   std::string GetDescription() const { return *description_; }
   void SetDescription(std::string description) { description_ = description; }

   std::string GetFaction() const { return *faction_; }
   void SetFaction(std::string faction) { faction_ = faction; }

   std::string GetIcon() const {return *icon_;}
   void SetIcon(std::string icon) { icon_ = icon; }
 
private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<std::string>  name_;
   dm::Boxed<std::string>  description_;
   dm::Boxed<std::string>  faction_;
   dm::Boxed<std::string>  icon_;

};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_UNIT_FRAME_H
