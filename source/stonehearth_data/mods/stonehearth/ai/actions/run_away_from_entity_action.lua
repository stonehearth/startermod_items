local AiHelpers = require 'ai.actions.ai_helpers'
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Quaternion = _radiant.csg.Quaternion
local rng = _radiant.csg.get_default_rng()

local RunAwayFromEntity = class()

RunAwayFromEntity.name = 'run away from entity'
RunAwayFromEntity.does = 'stonehearth:run_away_from_entity'
RunAwayFromEntity.args = {
   threat = Entity,
   distance = 'number',
   move_effect = {
      type = 'string',
      default = 'run',
   },
}
RunAwayFromEntity.version = 1
RunAwayFromEntity.priority = 1

function RunAwayFromEntity:start_thinking(ai, entity, args)
   local threat = args.threat

   if not threat:is_valid() then
      local log = ai:get_log()
      log:info('Invalid entity')
      return
   end

   self._destination = self:_choose_destination(entity, threat, args.distance)
   if self._destination then
      ai:set_think_output()
   end
end

-- Run towards your town center (TODO, actually pathfind towards it?)
-- If you don't have one or can't find one, run in a random direction
function RunAwayFromEntity:_choose_destination(entity, threat, distance)
   local entity_location = entity:add_component('mob'):get_world_location()
   local threat_location = threat:add_component('mob'):get_world_location()
   local destination
   local direction

   --Run in the general direction of home
   local player_id = radiant.entities.get_player_id(entity)
   local town = stonehearth.town:get_town(player_id)
   local banner = town:get_banner()
   local banner_location
   if banner then
      banner_location = banner:add_component('mob'):get_world_location()
   end

   if banner and banner_location then
      direction = banner_location - entity_location
   else 
      direction = entity_location - threat_location
   end

   direction.y = 0

   if direction:distance_squared() ~= 0 then
      direction:normalize()
   else
      direction = radiant.math.random_xz_unit_vector()
   end

   destination = self:_calculate_location(entity_location, direction, 0, distance)
   
   return destination
end

function RunAwayFromEntity:_calculate_location(start_location, direction, angle, distance)
   local vector = radiant.math.rotate_about_y_axis(direction, angle)
   vector:scale(distance)

   local new_location = start_location + vector

   local x = radiant.math.round(new_location.x)
   local z = radiant.math.round(new_location.z)
   local y = start_location.y

   local new_grid_location = Point3(x, y, z)
   return new_grid_location
end

function RunAwayFromEntity:run(ai, entity, args)
   ai:execute('stonehearth:go_toward_location', {
      destination = self._destination,
      move_effect = args.move_effect,
   })
end

return RunAwayFromEntity
