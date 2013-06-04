#ifndef _RADIANT_OM_LUA_COMPONENTS_H
#define _RADIANT_OM_LUA_COMPONENTS_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class LuaComponents : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponents);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   std::string ToString() const;

   luabind::object GetLuaComponent(const char* name) const;
   void AddLuaComponent(const char* name, luabind::object api);

private:
   void InitializeRecordFields() override;

public:
   std::unordered_map<std::string, luabind::object>   lua_components_;
};

static std::ostream& operator<<(std::ostream& os, const LuaComponents& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LUA_COMPONENTS_H
