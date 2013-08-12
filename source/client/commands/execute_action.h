#ifndef _RADIANT_CLIENT_EXECUTE_ACTION_H
#define _RADIANT_CLIENT_EXECUTE_ACTION_H

#include "om/om.h"
#include "pending_command.h"
#include "client/selectors/actor_selector.h"
#include "client/selectors/voxel_range_selector.h"
#include "command.h"
#include <boost/any.hpp>

BEGIN_RADIANT_CLIENT_NAMESPACE

class ExecuteAction : public Command
{
public:
   ExecuteAction(PendingCommandPtr cmd);
   ~ExecuteAction();

   void operator()(void);

private:
   void SetGroundArea(const om::Selection &area);

private:
   dm::ObjectId                        self_;
   JSONNode                            args_;
   //std::shared_ptr<resources::DataResource>  action_;
   std::vector<om::Selection>          actual_;
   int                                 deferredCommandId_;
   std::shared_ptr<Selector>           selector_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_EXECUTE_ACTION_H
