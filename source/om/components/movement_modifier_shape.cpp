#include "radiant.h"
#include "om/region.h"
#include "csg/util.h"
#include "movement_modifier_shape.ridl.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "resources/res_manager.h"
#include "region_common.h"

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
   modifier_.Set(obj.get("modifier", 2.0f)); // double the speed!

   region_ = LoadRegion(obj, GetStore());
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
