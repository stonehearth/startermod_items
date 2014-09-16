local AiHelpers = require 'ai.actions.ai_helpers'
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Quaternion = _radiant.csg.Quaternion
local rng = _radiant.csg.get_default_rng()

local RunAwayFromEntity = class()

RunAwayFromEntity.name = 'run away from entity'
RunAwayFromEntity.does = 'stonehearth:run_away_from_entity'
RunAwayFromEntity.args = {
   threat = Entity,
   distance = 'number',
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

   self._entity = entity
   self._entity_grid_location = ai.CURRENT.location

   self._destination = self:_choose_destination(entity, threat, args.distance)
   if self._destination then
      ai:set_think_output()
   end
end

function RunAwayFromEntity:run(ai, entity, args)
   ai:execute('stonehearth:go_toward_location', { destination = self._destination })
end

function RunAwayFromEntity:_choose_destination(entity, threat, distance)
   local entity_grid_location = self._entity_grid_location
   local threat_grid_location = threat:add_component('mob'):get_world_grid_location()

   local opposite_direction = entity_grid_location - threat_grid_location
   opposite_direction.y = 0

   if opposite_direction:length() ~= 0 then
      opposite_direction:normalize()
   else
      opposite_direction = radiant.math.random_xz_unit_vector()
   end

   local sign = rng:get_int(0, 1)*2 - 1
   local angles = { 0, sign*90, -sign*90 }

   for _, angle in ipairs(angles) do
      local destination = self:_calculate_location(entity_grid_location, opposite_direction, angle, distance)
      -- could also find the longest path, instead of just the first path > 1
      if self:_get_path_length(entity, entity_grid_location, destination) > 1.5 then
         return destination
      end
   end

   return nil
end

-- start_location and direction are Point3fs
function RunAwayFromEntity:_calculate_location(start_location, direction, angle, distance)
   assert(direction.y == 0)
   local vector = radiant.math.rotate_about_y_axis(direction, angle)
   vector:scale(distance)

   local new_location = start_location + vector
   local new_grid_location = new_location:to_closest_int()
   return new_grid_location
end

-- start_location and destination are Point3s
function RunAwayFromEntity:_get_path_length(entity, start_location, desired_destination)
   local resolved_destination = radiant.terrain.get_direct_path_end_point(start_location, desired_destination, entity)
   local distance = start_location:distance_to(resolved_destination)
   return distance
end

return RunAwayFromEntity
