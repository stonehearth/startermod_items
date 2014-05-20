#include "pch.h"
#include "om/region.h"
#include "region_collision_shape.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, RegionCollisionShape const& o)
{
   return (os << "[RegionCollisionShape]");
}

void RegionCollisionShape::ConstructObject()
{
   Component::ConstructObject();
   region_collision_type_ = RegionCollisionTypes::SOLID;
}

static std::unordered_map<RegionCollisionShape::RegionCollisionTypes, std::string> regionCollisonTypesToString;
static std::unordered_map<std::string, RegionCollisionShape::RegionCollisionTypes> regionCollisonTypesToEnum;
static void InitRegionCollisionTypesMap()
{
   if (regionCollisonTypesToString.empty()) {
      regionCollisonTypesToString[RegionCollisionShape::RegionCollisionTypes::NONE] = "none";
      regionCollisonTypesToString[RegionCollisionShape::RegionCollisionTypes::SOLID] = "solid";
      for (auto const& i : regionCollisonTypesToString) {
         regionCollisonTypesToEnum[i.second] = i.first;
      }
   }
}

void RegionCollisionShape::LoadFromJson(json::Node const& obj)
{
   if (obj.has("region")) {
      region_ = GetStore().AllocObject<Region3Boxed>();
      (*region_)->Set(obj.get("region", csg::Region3()));
   }

   InitRegionCollisionTypesMap();

   std::string enum_str = obj.get("region_collision_type", "solid");
   auto i = regionCollisonTypesToEnum.find(enum_str);
   if (i != regionCollisonTypesToEnum.end()) {
      region_collision_type_ = i->second;
   }
}

void RegionCollisionShape::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }

   InitRegionCollisionTypesMap();

   auto i = regionCollisonTypesToString.find(region_collision_type_);
   // the value in the model better be valid
   ASSERT(i != regionCollisonTypesToString.end());
   node.set("region_collision_type", i->second);
}
