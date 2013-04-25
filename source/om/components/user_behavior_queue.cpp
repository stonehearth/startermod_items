#include "pch.h"
#include "user_behavior.h"
#include "user_behavior_queue.h"

using namespace ::radiant;
using namespace ::radiant::om;

void UserBehaviorQueue::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("behaviors", behaviors_);
}

UserBehaviorPtr UserBehaviorQueue::AddUserBehavior()
{
   UserBehaviorPtr b = GetStore().AllocObject<UserBehavior>();
   behaviors_.Enqueue(b);
   return b;
}
