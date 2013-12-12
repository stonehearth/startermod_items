#ifndef _RADIANT_OM_LUA_REGION_H
#define _RADIANT_OM_LUA_REGION_H

#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

class LuaDeepRegionGuard : public std::enable_shared_from_this<LuaDeepRegionGuard>
{
public:
   LuaDeepRegionGuard(DeepRegionGuardPtr trace);

   std::shared_ptr<LuaDeepRegionGuard> OnChanged(luabind::object changed_cb);
   std::shared_ptr<LuaDeepRegionGuard> PushObjectState();
   void Destroy();
   
private:
   DeepRegionGuardPtr   trace_;
};

DECLARE_SHARED_POINTER_TYPES(LuaDeepRegionGuard)

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
