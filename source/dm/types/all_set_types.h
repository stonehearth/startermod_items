#include "dm/types/type_macros.h"
#include "dm/boxed.h"
#include "om/entity.h"
#include "om/json.h"
#include "om/components/effect.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "lib/json/node.h"

#define ALL_DM_SET_TYPES \
   SET(int) \
   SET(std::string) \
   SET(om::ModelLayerPtr) \
   SET(om::EffectPtr) \
   SET(om::EntityRef) \
   SET(std::shared_ptr<om::JsonBoxed>)

