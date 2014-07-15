#include "dm/types/type_macros.h"
#include "om/entity.h"
#include "om/region.h"
#include "om/selection.h"
#include "om/components/sensor.ridl.h"
#include "om/components/data_store.ridl.h"
#include "om/components/effect.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "lib/lua/bind.h"

#define ALL_DM_MAP_TYPES \
   MAP(int, om::EffectPtr) \
   MAP(int, om::EntityRef) \
   MAP(int, luabind::object) \
   MAP(std::string, luabind::object) \
   MAP(std::string, om::EntityRef) \
   MAP(std::string, dm::ObjectPtr) \
   MAP(std::string, om::ComponentPtr) \
   MAP(std::string, om::Selection) \
   MAP(std::string, om::SensorPtr) \
   MAP(std::string, om::DataStorePtr) \
   MAP(std::string, om::ModelLayerPtr) \
   MAP3(csg::Point3, om::Region3BoxedPtr, csg::Point3::Hash)
