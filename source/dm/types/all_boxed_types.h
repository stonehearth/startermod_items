#include "dm/types/type_macros.h"
#include "om/entity.h"
#include "om/region.h"
#include "om/components/mob.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "om/components/target_table_entry.ridl.h"
#include "csg/color.h"
#include "csg/sphere.h"
#include "csg/transform.h"
#include "csg/region.h"
#include "lib/json/node.h"
#include "lib/lua/data_object.h"
#include "lib/lua/controller_object.h"


#define ALL_DM_BOXED_TYPES \
   BOXED(std::string) \
   BOXED(bool) \
   BOXED(int) \
   BOXED(float) \
   BOXED(om::EntityRef) \
   BOXED(om::EntityPtr) \
   BOXED(om::MobRef) \
   BOXED(om::ModelLayer::Layer) \
   BOXED(om::TargetTableEntryPtr) \
   CREATE_BOXED(om::Region2Boxed) \
   CREATE_BOXED(om::Region3Boxed) \
   CREATE_BOXED(om::Region2BoxedPtrBoxed) \
   CREATE_BOXED(om::Region3BoxedPtrBoxed) \
   BOXED(csg::Region3) \
   BOXED(csg::Region2) \
   BOXED(csg::Point3) \
   BOXED(csg::Point3f) \
   BOXED(csg::Cube3) \
   BOXED(csg::Cube3f) \
   BOXED(csg::Transform) \
   BOXED(json::Node) \
   BOXED(lua::ControllerObject) \
   BOXED(lua::DataObject)


