#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "movement_modifier_shape.ridl.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, MovementModifierShape const& o)
{
   return (os << "[MovementModifierShape]");
}

void MovementModifierShape::ConstructObject()
{
   Component::ConstructObject();
}

void MovementModifierShape::LoadFromJson(json::Node const& obj)
{
   modifier_.Set(obj.get("modifier", 9.0f));

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
}

void MovementModifierShape::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::Region3fBoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }

   float f = GetModifier();
   node.set("modifier", f);
}
