#ifndef _RADIANT_OM_TERRAIN_H
#define _RADIANT_OM_TERRAIN_H

#include "math3d.h"
#include "dm/record.h"
#include "dm/set.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "om/om.h"
#include "om/grid/grid.h"
#include "om/all_object_types.h"
#include "om/region.h"
#include "csg/cube.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Terrain : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Terrain, terrain);
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   std::string ToString() const;

   enum TerrainTypes {
      Null        = 0,
      Magma       = 1,
      Bedrock     = 2,
      Topsoil     = 3,
      Grass       = 4, // On top of Topsoil...
      Plains        = 5, // the dark green, flat floors
      TopsoilDetail = 6,
      DarkWood      = 7, 
      DirtPath      = 8, 
   };

   const csg::Region3& GetRegion() const { return *region_; }
   void AddRegion(csg::Region3 const& region);
   void PlaceEntity(EntityRef e, const math3d::ipoint3& pt);
   void CreateNew();
   void AddCube(csg::Cube3 const& cube);
   void RemoveCube(csg::Cube3 const& region);

private:
   void InitializeRecordFields() override;

public:
   Region                    region_;
   //dm::Set<dm::Ref<Entity>>      mobs_;
   //dm::Set<dm::Ref<Entity>>      items_;
};

static std::ostream& operator<<(std::ostream& os, const Terrain& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_TERRAIN_H
