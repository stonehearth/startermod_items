#include "dm/types/type_macros.h"
#include "dm/boxed.h"
#include "om/entity.h"
#include "om/components/effect.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "om/components/target_table.ridl.h"
#include "lib/json/node.h"

#define ALL_DM_SET_TYPES \
   SET(int) \
   SET(std::string) \
   SET(om::TargetTablePtr) \
   SET(om::ModelLayerPtr) \
   SET(om::EffectPtr) \
   SET(om::EntityRef) \
   SET(std::shared_ptr<dm::Boxed<json::Node>>)

