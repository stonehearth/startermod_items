#ifndef _RADIANT_OM_LUA_OM_H
#define _RADIANT_OM_LUA_OM_H

#include "lib/lua/bind.h"
#include "om/region.h"
#include <ostream>

BEGIN_RADIANT_OM_NAMESPACE

void RegisterLuaTypes(lua_State* L);

class DeepRegionGuardLua;
DECLARE_SHARED_POINTER_TYPES(DeepRegionGuardLua);

class DeepRegionGuardLua : public std::enable_shared_from_this<DeepRegionGuardLua>
{
   public:
      DeepRegionGuardLua(lua_State* L, Region3BoxedPtrBoxed const& bbrp, const char* reason);

   public:
      DeepRegionGuardLuaPtr OnChanged(luabind::object cb);
      void Destroy();
      void FireTrace();

   private:
      lua_State*                    L_;
      DeepRegionGuardPtr           region_guard_;
      std::vector<luabind::object>  cbs_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
