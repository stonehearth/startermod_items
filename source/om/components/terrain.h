#ifndef _RADIANT_OM_TERRAIN_H
#define _RADIANT_OM_TERRAIN_H

#include "dm/record.h"
#include "dm/set.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "om/om.h"
#include "om/object_enums.h"
#include "om/region.h"
#include "csg/cube.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Terrain : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Terrain, terrain);

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

   typedef dm::Map<csg::Point3, BoxedRegion3Ptr, csg::Point3::Hash> TileMap;

   void AddRegion(csg::Point3 const& location, BoxedRegion3Ptr r);
   void PlaceEntity(EntityRef e, const csg::Point3& pt);
   void CreateNew();

   TileMap const& GetTileMap() const { return tiles_; }

private:
   void InitializeRecordFields() override;
   BoxedRegion3Ptr GetRegion(csg::Point3 const& pt, csg::Point3 const*& regionOffset);

public:
   TileMap  tiles_;
};

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_TERRAIN_H
