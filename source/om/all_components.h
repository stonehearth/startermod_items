#ifndef _RADIANT_OM_ALL_COMPONENTS
#define _RADIANT_OM_ALL_COMPONENTS

#include "components/clock.h"
#include "components/entity_container.h"
#include "components/mob.h"
#include "components/model_variants.h"
#include "components/terrain.h"
#include "components/region_collision_shape.h"
#include "components/sphere_collision_shape.h"
#include "components/vertical_pathing_region.h"
#include "components/effect_list.h"
#include "components/render_info.h"
#include "components/sensor_list.h"
#include "components/target_tables.h"
#include "components/lua_components.h"
#include "components/destination.h"
#include "components/render_region.h"
#include "components/carry_block.ridl.h"
#include "components/paperdoll.h"
#include "components/unit_info.h"
#include "components/item.h"

#define OM_ALL_COMPONENT_TEMPLATES \
   CARRY_BLOCK_TEMPLATE \
   CLOCK_TEMPLATE

#endif // _RADIANT_OM_ALL_COMPONENTS
