#include "pch.h"
#include "lua/register.h"
#include "lua_model_variants_component.h"
#include "om/components/model_variants.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaModelVariantsComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<ModelVariants, Component>()
      ,
      lua::RegisterWeakGameObject<ModelVariant>()
      ;
}
