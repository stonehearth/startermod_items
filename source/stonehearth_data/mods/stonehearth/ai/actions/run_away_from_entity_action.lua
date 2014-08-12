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

   self._entity = entity
   self._entity_grid_location = ai.CURRENT.location

   self._destination = self:_choose_destination(entity, threat, args.distance)
   if self._destination then
      ai:set_think_output()
   end
end

function RunAwayFromEntity:run(ai, entity, args)
   ai:execute('stonehearth:go_toward_location', {
      destination = self._destination,
      move_effect = args.move_effect,
   })
end

function RunAwayFromEntity:_choose_destination(entity, threat, distance)
   local entity_grid_location = self._entity_grid_location
   local threat_grid_location = threat:add_component('mob'):get_world_grid_location()

   local opposite_direction = (entity_grid_location - threat_grid_location):to_float()
   opposite_direction.y = 0

   if opposite_direction:length() ~= 0 then
      opposite_direction:normalize()
   else
      opposite_direction = radiant.math.random_xz_unit_vector()
   end

   local sign = rng:get_int(0, 1)*2 - 1
   local angles = { 0, sign*90, -sign*90 }
   local entity_grid_location_as_float = entity_grid_location:to_float()
   local destination = nil

   for _, angle in ipairs(angles) do
      destination = self:_calculate_location(entity_grid_location_as_float, opposite_direction, angle, distance)
      -- could also find the longest path, instead of just the first path > 1
      if self:_get_path_length(entity, entity_grid_location, destination) > 1 then
         break
      end
   end

   return destination
end

-- start_location and direction are Point3fs
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

-- start_location and destination are Point3s
function RunAwayFromEntity:_get_path_length(entity, start_location, destination)
   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity)
      :set_start_location(start_location)
      :set_end_location(destination)
      :set_allow_incomplete_path(true)
      :set_reversible_path(true)

   local path = direct_path_finder:get_path()
   local distance = self:_get_distance_to_end(start_location, path)
   return distance
end

-- this is not the distance travelled along the path, just the distance between start and end
-- we explicitly specify the start_location because, by convention, the path leaves this point off
-- to prevent travelling to the center of the current block first
function RunAwayFromEntity:_get_distance_to_end(start_location, path)
   if not path or path:is_empty() then
      return 0
   end

   -- get_finish_point is undefined when the path is empty
   local end_location = path:get_finish_point()
   local distance = start_location:distance_to(end_location)
   return distance
end

return RunAwayFromEntity
