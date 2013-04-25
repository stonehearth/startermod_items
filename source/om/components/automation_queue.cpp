#include "pch.h"
#include "automation_task.h"
#include "automation_queue.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void AutomationQueue::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("queue", queue_);
}

AutomationTaskPtr AutomationQueue::AddTask(luabind::object handler)
{
   auto task = GetStore().AllocObject<AutomationTask>();
   task->SetHandler(handler);
   queue_.Enqueue(task);
   return task;
}
