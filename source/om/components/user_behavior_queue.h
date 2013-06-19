#ifndef _RADIANT_OM_USER_BEHAVIOR_QUEUE_H
#define _RADIANT_OM_USER_BEHAVIOR_QUEUE_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/queue.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"
#include "user_behavior.h"

BEGIN_RADIANT_OM_NAMESPACE

class UserBehaviorQueue : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(UserBehaviorQueue);

public:
   UserBehaviorPtr AddUserBehavior();

private:
   NO_COPY_CONSTRUCTOR(UserBehaviorQueue)
   void InitializeRecordFields() override;

public:
   dm::Queue<UserBehaviorPtr>      behaviors_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_USER_BEHAVIOR_QUEUE_H
