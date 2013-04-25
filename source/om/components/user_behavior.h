#ifndef _RADIANT_OM_USER_BEHAVIOR_H
#define _RADIANT_OM_USER_BEHAVIOR_H

#include <luabind/luabind.hpp>
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "store.pb.h"

BEGIN_RADIANT_OM_NAMESPACE

class UserBehavior : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(UserBehavior);
   UserBehavior() : dm::Record() { }

public:
   std::string GetScriptName() const;
   std::string GetConfigString() const;
   luabind::object GetScriptInstance() const;

   void SetScriptName(std::string script);
   void SetConfigString(std::string config);
   void SetScriptInstance(luabind::object instance);

private:
   NO_COPY_CONSTRUCTOR(UserBehavior)

   void InitializeRecordFields() override;

public:
   luabind::object            scriptInstance_;
   dm::Boxed<std::string>     scriptName_;
   dm::Boxed<std::string>     jsonConfig_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_USER_BEHAVIOR_H
