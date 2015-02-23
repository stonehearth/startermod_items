local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local SetHangableAdjacent = class()

SetHangableAdjacent.name = 'set hangable adjacent'
SetHangableAdjacent.does = 'stonehearth:set_hangable_adjacent'
SetHangableAdjacent.args = {
   item = Entity,
}
SetHangableAdjacent.version = 2
SetHangableAdjacent.priority = 1

function SetHangableAdjacent:start_thinking(ai, entity, args)
   local entity_height = self:_get_entity_height(entity)
   local destination_component = args.item:get_component('destination')

   if not destination_component then
      destination_component = args.item:add_component('destination')
      destination_component:set_region(_radiant.sim.alloc_region3())
      destination_component:set_adjacent(_radiant.sim.alloc_region3())
      destination_component:set_reserved(_radiant.sim.alloc_region3())
   end

   local destination_region = destination_component:get_region()
   local adjacent_region = destination_component:get_adjacent()

   destination_region:modify(function(cursor)
         cursor:clear()
         cursor:add_unique_cube(Cube3(Point3.zero, Point3.one))
      end)

   adjacent_region:modify(function(cursor)
         local adjacent_region = csg_lib.create_adjacent_columns(Point3.zero, -(entity_height-1), 1)
         cursor:copy_region(adjacent_region)
      end)

   ai:set_think_output()
end

function SetHangableAdjacent:_get_entity_height(entity)
   local mob_collision_type = entity:add_component('mob'):get_mob_collision_type()

   -- TODO: read the entity's height from someplace sensible
   if mob_collision_type == _radiant.om.Mob.HUMANOID then
      return 3
   else
      assert(false)
   end
end

return SetHangableAdjacent
