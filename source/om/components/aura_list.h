#ifndef _RADIANT_OM_AURA_LIST_H
#define _RADIANT_OM_AURA_LIST_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class AuraList;

class Aura : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(Aura);

   Aura() : dm::Record() { }
   virtual ~Aura() { }

   std::string GetName() const { return *name_; }

   om::EntityRef GetSource() const { return *source_; }

   void SetExpireTime(int expireTime) { expireTime_ = expireTime; }
   int GetExpireTime() const { return *expireTime_; }

   luabind::object GetMsgHandler() const { return msgHandler_; }
   void SetMsgHandler(luabind::object msgHandler) { msgHandler_ = msgHandler; }

protected:
   friend AuraList;

   void Construct(std::string name, om::EntityRef source) {
      name_ = name;
      source_ = source;
      expireTime_ = 0;
   }

private:
   void InitializeRecordFields() override {
      AddRecordField("name",        name_);
      AddRecordField("source",      source_);
      AddRecordField("expire_time", expireTime_);
   }

private:
   dm::Boxed<om::EntityRef>   source_;
   dm::Boxed<std::string>     name_;
   dm::Boxed<int>             expireTime_;
   luabind::object            msgHandler_;
};

typedef std::shared_ptr<Aura> AuraPtr;

class AuraList : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(AuraList);

   AuraList() { }

   const dm::Set<AuraPtr>& GetAuras() const { return auras_; }

   AuraRef GetAura(std::string name, om::EntityRef source);
   AuraRef CreateAura(std::string name, om::EntityRef source);
   void RemoveAura(AuraPtr aura);

private:
   void InitializeRecordFields() override;

public:
   dm::Set<std::shared_ptr<Aura>>   auras_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_AURA_LIST_H
