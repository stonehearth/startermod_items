#ifndef _RADIANT_OM_LUA_REGION_H
#define _RADIANT_OM_LUA_REGION_H

#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

template <typename OuterBox>
class LuaDeepRegionGuard : public std::enable_shared_from_this<LuaDeepRegionGuard<OuterBox>>
{
public:
   typedef std::shared_ptr<DeepRegionGuard<OuterBox>> RegionTracePtr;
   typedef std::shared_ptr<LuaDeepRegionGuard> LuaDeepRegionGuardPtr;

public:
   LuaDeepRegionGuard(RegionTracePtr trace);

   LuaDeepRegionGuardPtr  OnChanged(luabind::object changed_cb);
   LuaDeepRegionGuardPtr  PushObjectState();
   void Destroy();
   
private:
   RegionTracePtr   trace_;
};

typedef LuaDeepRegionGuard<Region2BoxedPtrBoxed>    LuaDeepRegion2Guard;
typedef LuaDeepRegionGuard<Region3BoxedPtrBoxed>    LuaDeepRegion3Guard;
typedef LuaDeepRegionGuard<Region2fBoxedPtrBoxed>   LuaDeepRegion2fGuard;
typedef LuaDeepRegionGuard<Region3fBoxedPtrBoxed>   LuaDeepRegion3fGuard;

DECLARE_SHARED_POINTER_TYPES(LuaDeepRegion2Guard)
DECLARE_SHARED_POINTER_TYPES(LuaDeepRegion3Guard)
DECLARE_SHARED_POINTER_TYPES(LuaDeepRegion2fGuard)
DECLARE_SHARED_POINTER_TYPES(LuaDeepRegion3fGuard)

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
