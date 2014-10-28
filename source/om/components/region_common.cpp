#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "region_common.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "resources/res_manager.h"


using namespace ::radiant;
using namespace ::radiant::om;

Region3fBoxedPtr radiant::om::LoadRegion(json::Node const& obj, radiant::dm::Store& store)
{
   Region3fBoxedPtr region;
   if (obj.has("region")) {
      region = store.AllocObject<Region3fBoxed>();
      region->Set(obj.get("region", csg::Region3f()));
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
         region = store.AllocObject<Region3fBoxed>();
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
         region->Set(shape);
      }
   }
   return region;
}