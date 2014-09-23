#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "region_collision_shape.ridl.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "resources/res_manager.h"

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
   if (obj.has("region")) {
      region_ = GetStore().AllocObject<Region3fBoxed>();
      (*region_)->Set(obj.get("region", csg::Region3f()));
   } else if (obj.has("region_from_model")) {
      csg::Region3f shape;

      json::Node regionFromModel = obj.get_node("region_from_model");
      std::string qubicleFile = regionFromModel.get<std::string>("model", "");
      voxel::QubicleFile const* qubicle = res::ResourceManager2::GetInstance().LookupQubicleFile(qubicleFile);

      if (qubicle) {
         auto addMatrix = [&shape](voxel::QubicleMatrix const* matrix) {
            if (matrix) {
               csg::Region3 model = voxel::QubicleBrush(matrix, 0)
                  .SetPaintMode(voxel::QubicleBrush::Opaque)
                  .SetOffsetMode(voxel::QubicleBrush::File)
                  .PaintOnce();

               csg::Point3 size = matrix->GetSize();
               csg::Point3 pos  = matrix->GetPosition();

               // Convert left to right handed without messing up facing by sliding
               // the model over rather than flipping the coords
               int xOffset = pos.x * 2 + size.x;
               shape += csg::ToFloat(model.Translated(csg::Point3(-xOffset, 0, 0)));
            }
         };
         region_ = GetStore().AllocObject<Region3fBoxed>();
         json::Node matrices = regionFromModel.get_node("matrices");
         if (matrices.begin() != matrices.end()) {
            for (json::Node child : matrices) {
               voxel::QubicleMatrix const* matrix = qubicle->GetMatrix(child.as<std::string>(""));
               addMatrix(matrix);
            }
         } else {
            for (auto const& entry : *qubicle) {
               addMatrix(&entry.second);
            }
         }
         (*region_)->Set(shape);
      }
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
