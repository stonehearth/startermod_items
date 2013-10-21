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
      Boulder       = 4,
      Soil          = 5,
      DarkGrass     = 6,
      DarkGrassDark = 7,
      LightGrass    = 8,
      DarkWood      = 9, 
      DirtRoad      = 10, 
      Magma         = 11
   };

   typedef dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash> ZoneMap;

   int GetZoneSize();
   void SetZoneSize(int zone_size);
   void AddZone(csg::Point3 const& zone_offset, Region3BoxedPtr region3);
   void PlaceEntity(EntityRef e, const csg::Point3& location);
   void CreateNew();

   ZoneMap const& GetZoneMap() const { return zones_; }
   Region3BoxedPtr GetZone(csg::Point3 const& location, csg::Point3& zone_offset);

private:
   dm::Boxed<int> zone_size_;
   ZoneMap zones_;
   void InitializeRecordFields() override;
};

END_RADIANT_OM_NAMESPACE


#endif //  _RADIANT_OM_TERRAIN_H
