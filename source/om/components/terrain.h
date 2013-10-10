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

   enum BlockTypes {
      Null          = 0,
      RockLayer1    = 1,
      RockLayer2    = 2,
      RockLayer3    = 3,
      Soil          = 4,
      DarkGrass     = 5,
      DarkGrassDark = 6,
      LightGrass    = 7,
      DarkWood      = 8, 
      DirtRoad      = 9, 
      Magma         = 10
   };

   typedef dm::Map<csg::Point3, BoxedRegion3Ptr, csg::Point3::Hash> ZoneMap;

   int GetZoneSize();
   void SetZoneSize(int zone_size);
   void AddZone(csg::Point3 const& zone_offset, BoxedRegion3Ptr region3);
   void PlaceEntity(EntityRef e, const csg::Point3& location);
   void CreateNew();

   ZoneMap const& GetZoneMap() const { return zones_; }
   BoxedRegion3Ptr GetZone(csg::Point3 const& location, csg::Point3& zone_offset);

private:
   dm::Boxed<int> zone_size_;
   ZoneMap zones_;
   void InitializeRecordFields() override;
};

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_TERRAIN_H
