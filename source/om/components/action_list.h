#ifndef _RADIANT_OM_ACTION_LIST_H
#define _RADIANT_OM_ACTION_LIST_H

#include "dm/set.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class ActionList : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(ActionList);

   ActionList() { }

   const dm::Set<std::string>& GetActions() const { return actions_; }

   void AddAction(std::string action) { actions_.Insert(action); }

private:
   void InitializeRecordFields() override;

public:
   dm::Set<std::string>    actions_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ACTION_LIST_H
