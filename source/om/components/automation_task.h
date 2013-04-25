#ifndef _RADIANT_OM_AUTOMATION_TASK_H
#define _RADIANT_OM_AUTOMATION_TASK_H

#include <luabind/luabind.hpp>
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class AutomationTask : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(AutomationTask);

   AutomationTask() : Component() { }

   luabind::object GetHandler() const { return handler_; }
   void SetHandler(luabind::object handler) { handler_ = handler; }

private:
   NO_COPY_CONSTRUCTOR(AutomationTask)

   void InitializeRecordFields() override;

public:
   luabind::object      handler_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_AUTOMATION_TASK_H
