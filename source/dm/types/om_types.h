#include "om/entity.h"
#include "om/region.h"
#include "om/selection.h"
#include "om/components/mob.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "om/components/target_table_entry.ridl.h"
#include "om/components/target_table_group.ridl.h"
#include "om/components/target_table.ridl.h"
#include "om/components/effect.ridl.h"
#include "om/components/sensor.ridl.h"
#include "om/components/data_store.ridl.h"

BOXED(om::EntityRef)
BOXED(om::EntityPtr)
BOXED(om::MobRef)
BOXED(om::ModelLayer::Layer)
BOXED(om::TargetTableEntryPtr)

CREATE_BOXED(om::Region2Boxed)
CREATE_BOXED(om::Region3Boxed)
CREATE_BOXED(om::Region2BoxedPtrBoxed)
CREATE_BOXED(om::Region3BoxedPtrBoxed)

MAP(int, om::TargetTableEntryPtr)
MAP(int, om::EntityRef)
MAP(std::string, om::Selection)
MAP(std::string, om::SensorPtr)
MAP(std::string, om::DataStorePtr)
MAP(std::string, om::TargetTableGroupPtr)
MAP3(csg::Point3, om::Region3BoxedPtr, csg::Point3::Hash)

SET(om::TargetTablePtr)
SET(om::ModelLayerPtr)
SET(om::EffectPtr)
SET(om::EntityRef)
