/* Boilerplate intentionally omitted... */

#if defined(DEFINE_ALL_COMPONENTS)
#  include "components/clock.h"
#  include "components/stockpile_designation.h"
#  include "components/entity_container.h"
#  include "components/item.h"
#  include "components/mob.h"
#  include "components/render_grid.h"
#  include "components/render_rig.h"
#  include "components/terrain.h"
#  include "components/collision_shape.h"
#  include "components/grid_collision_shape.h"
#  include "components/region_collision_shape.h"
#  include "components/sphere_collision_shape.h"
#  include "components/carry_block.h"
#  include "components/unit_info.h"
#  include "components/room.h"
#  include "components/scaffolding.h"
#  include "components/post.h"
#  include "components/wall.h"
#  include "components/floor.h"
#  include "components/fixture.h"
#  include "components/peaked_roof.h"
#  include "components/portal.h"
#  include "components/build_orders.h"
#  include "components/build_order.h"
#  include "components/build_order_dependencies.h"
#  include "components/vertical_pathing_region.h"
#  include "components/user_behavior_queue.h"
#  include "components/effect_list.h"
#  include "components/render_info.h"
#  include "components/profession.h"
#  include "components/automation_task.h"
#  include "components/automation_queue.h"
#  include "components/paperdoll.h"
#  include "components/sensor_list.h"
#  include "components/attributes.h"
#  include "components/aura_list.h"
#  include "components/target_tables.h"
#  include "components/inventory.h"
#  include "components/weapon_info.h"
#  include "components/lua_components.h"
#  include "components/destination.h"
#  include "components/render_region.h"
#endif

#if !defined(OM_ALL_COMPONENTS) 

#  define OM_ALL_COMPONENTS \
      OM_OBJECT(Clock,                 clock) \
      OM_OBJECT(StockpileDesignation,  stockpile_designation) \
      OM_OBJECT(EntityContainer,       entity_container) \
      OM_OBJECT(Item,                  item) \
      OM_OBJECT(Mob,                   mob) \
      OM_OBJECT(RenderGrid,            render_grid) \
      OM_OBJECT(RenderRig,             render_rig) \
      OM_OBJECT(RenderRigIconic,       render_rig_iconic) \
      OM_OBJECT(GridCollisionShape,    grid_collision_shape) \
      OM_OBJECT(SphereCollisionShape,  sphere_collision_shape) \
      OM_OBJECT(RegionCollisionShape,  region_collision_shape) \
      OM_OBJECT(Terrain,               terrain) \
      OM_OBJECT(CarryBlock,            carry_block) \
      OM_OBJECT(UnitInfo,              unit_info) \
      OM_OBJECT(Wall,                  wall) \
      OM_OBJECT(Floor,                 floor) \
      OM_OBJECT(Scaffolding,           scaffolding) \
      OM_OBJECT(Post,                  post) \
      OM_OBJECT(Room,                  room) \
      OM_OBJECT(PeakedRoof,            peaked_roof) \
      OM_OBJECT(Fixture,               fixture) \
      OM_OBJECT(Portal,                portal) \
      OM_OBJECT(BuildOrders,           build_orders) \
      OM_OBJECT(BuildOrderDependencies, build_order_dependencies) \
      OM_OBJECT(VerticalPathingRegion, vertical_pathing_region) \
      OM_OBJECT(UserBehaviorQueue,     user_behavior_queue) \
      OM_OBJECT(EffectList,            effect_list) \
      OM_OBJECT(RenderInfo,            render_info) \
      OM_OBJECT(Profession,            profession) \
      OM_OBJECT(AutomationTask,        automation_task) \
      OM_OBJECT(AutomationQueue,       automation_queue) \
      OM_OBJECT(Paperdoll,             paperdoll) \
      OM_OBJECT(SensorList,            sensor_list) \
      OM_OBJECT(Attributes,            attributes) \
      OM_OBJECT(AuraList,              aura_list) \
      OM_OBJECT(TargetTables,          target_tables) \
      OM_OBJECT(Inventory,             inventory) \
      OM_OBJECT(WeaponInfo,            weapon_info) \
      OM_OBJECT(LuaComponents,         lua_components) \
      OM_OBJECT(Destination,           destination) \
      OM_OBJECT(RenderRegion,          render_region) \

#endif

