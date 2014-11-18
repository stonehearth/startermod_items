#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "region_collision_shape.ridl.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "resources/res_manager.h"
#include "region_common.h"

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
      regionCollisonTypesToString[RegionCollisionShape::RegionCollisionTypes::PLATFORM] = "platform";
      for (auto const& i : regionCollisonTypesToString) {
         regionCollisonTypesToEnum[i.second] = i.first;
      }
   }
}

void RegionCollisionShape::LoadFromJson(json::Node const& obj)
{
   region_ = radiant::om::LoadRegion(obj, GetStore());

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

   om::Region3fBoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }

   InitRegionCollisionTypesMap();

   auto i = regionCollisonTypesToString.find(region_collision_type_);
   // the value in the model better be valid
   ASSERT(i != regionCollisonTypesToString.end());
   node.set("region_collision_type", i->second);
}
