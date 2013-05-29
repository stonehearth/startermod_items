/* Boilerplate intentionally omitted... */

#if defined(DEFINE_ALL_OBJECTS)
// Super tricky here to get the macros defined in the right order...
#  undef DEFINE_ALL_OBJECTS
#  include "all_object_types.h"
#  define DEFINE_ALL_OBJECTS
#
#  include "entity.h"
#  include "grid/grid.h"
#  include "grid/grid_tile.h"
#  include "components/user_behavior.h"
#  define DEFINE_ALL_COMPONENTS
#    include "all_components.h"
#  undef DEFINE_ALL_COMPONENTS
#else
#  include "all_components.h"
#endif

#if !defined(OM_ALL_OBJECTS) 

#  define OM_ALL_OBJECTS \
      OM_OBJECT(Entity,         entity) \
      OM_OBJECT(Grid,           grid) \
      OM_OBJECT(GridTile,       grid_tile) \
      OM_OBJECT(Region,         region) \
      OM_OBJECT(Effect,         effect) \
      OM_OBJECT(UserBehavior,   user_behavior) \
      OM_OBJECT(Sensor,         sensor) \
      OM_OBJECT(Aura,           aura) \
      OM_OBJECT(TargetTable,        target_table) \
      OM_OBJECT(TargetTableGroup,   target_table_group) \
      OM_OBJECT(TargetTableEntry,   target_table_entry) \

#endif

