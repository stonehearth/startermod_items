#ifndef _RADIANT_OM_AUTOMATION_QUEUE_H
#define _RADIANT_OM_AUTOMATION_QUEUE_H

#include <luabind/luabind.hpp>
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/queue.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class AutomationQueue : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(AutomationQueue, automation_queue);

public:
   AutomationTaskPtr AddTask(luabind::object handler);
   const dm::Queue<AutomationTaskPtr>& GetContents() const { return queue_; }
   void Clear() { queue_.Clear(); }

private:
   NO_COPY_CONSTRUCTOR(AutomationQueue)

   void InitializeRecordFields() override;

public:
   dm::Queue<AutomationTaskPtr>  queue_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_AUTOMATION_QUEUE_H
