#include "dm/types/type_macros.h"
#include "om/entity.h"
#include "om/region.h"
#include "om/json.h"
#include "om/components/mob.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "csg/color.h"
#include "csg/sphere.h"
#include "csg/transform.h"
#include "csg/region.h"
#include "lib/json/node.h"
#include "lib/lua/data_object.h"


#define ALL_DM_BOXED_TYPES \
   BOXED(std::string) \
   BOXED(bool) \
   BOXED(int) \
   BOXED(float) \
   BOXED(om::EntityRef) \
   BOXED(om::EntityPtr) \
   BOXED(om::MobRef) \
   BOXED(om::Mob::MobCollisionTypes) \
   BOXED(om::RegionCollisionShape::RegionCollisionTypes) \
   BOXED(om::ModelLayer::Layer) \
   CREATE_BOXED(om::JsonBoxed) \
   CREATE_BOXED(om::Region2Boxed) \
   CREATE_BOXED(om::Region3Boxed) \
   CREATE_BOXED(om::Region2BoxedPtrBoxed) \
   CREATE_BOXED(om::Region3BoxedPtrBoxed) \
   CREATE_BOXED(om::Region2fBoxed) \
   CREATE_BOXED(om::Region3fBoxed) \
   CREATE_BOXED(om::Region2fBoxedPtrBoxed) \
   CREATE_BOXED(om::Region3fBoxedPtrBoxed) \
   BOXED(csg::Region3) \
   BOXED(csg::Region2) \
   BOXED(csg::Point3) \
   BOXED(csg::Point3f) \
   BOXED(csg::Cube3) \
   BOXED(csg::Cube3f) \
   BOXED(csg::Transform) \
   BOXED(json::Node) \
   BOXED(lua::DataObject) \
   BOXED(luabind::object)


