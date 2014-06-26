local AiHelpers = require 'ai.actions.ai_helpers'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local BumpAllies = class()
BumpAllies.name = 'bump allies'
BumpAllies.does = 'stonehearth:bump_allies'
BumpAllies.args = {
   distance = 'number'   -- separation distance (as measeured from both entity centers)
}
BumpAllies.version = 2
BumpAllies.priority = 1

function BumpAllies:run(ai, entity, args)
   self._entity = entity

   local grid_location = entity:add_component('mob'):get_world_grid_location()
   local radius = math.ceil(args.distance)
   local entities = self:_get_entities_in_cube(grid_location, radius)

   for _, other_entity in pairs(entities) do
      if self:_should_bump(other_entity) then
         local bump_vector = AiHelpers.calculate_bump_vector(entity, other_entity, args.distance)
         _radiant.sim.create_bump_location(other_entity, bump_vector)
      end
   end
end

function BumpAllies:_get_entities_in_cube(location, radius)
   local cube = Cube3(location + Point3(-radius, -1, -radius),
                      location + Point3(radius+1, 1+1, radius+1))

   local entities = radiant.terrain.get_entities_in_cube(cube)
   return entities
end

function BumpAllies:_should_bump(other_entity)
   if other_entity == self._entity then
      return false
   end

   if not radiant.entities.is_friendly(other_entity, self._entity) then
      return false
   end

   local mob = other_entity:get_component('mob')

   if not mob then
      return false
   end
   
   return mob:get_mob_collision_type() == _radiant.om.Mob.HUMANOID
end

return BumpAllies
