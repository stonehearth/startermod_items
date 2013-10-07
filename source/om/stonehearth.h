#ifndef _RADIANT_OM_STONEHEARTH_H
#define _RADIANT_OM_STONEHEARTH_H

#include "om.h"
#include "dm/dm.h"
#include "csg/region.h"
#include "radiant_json.h"

// This file clearly does not belong in the om.

struct lua_State;

BEGIN_RADIANT_OM_NAMESPACE

class Stonehearth
{
public:
   // xxx this file has no reason to exist anymore now that lua handles most of this
   static csg::Region3 ComputeStandingRegion(const csg::Region3& r, int height);

   static void InitEntity(om::EntityPtr entity, std::string const& uri, lua_State* L);
   static luabind::object om::Stonehearth::AddComponent(lua_State* L, om::EntityRef e, std::string name);
   static luabind::object om::Stonehearth::GetComponent(lua_State* L, om::EntityRef e, std::string name);

private:
   static void InitEntityByUri(om::EntityPtr entity, std::string const& uri, lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_STONEHEARTH_H
