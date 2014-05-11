local MathFns = require 'services.server.world_generation.math.math_fns'
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
      ai:abort('Invalid entity')
      return
   end

   self._destination = self:_choose_destination(entity, threat, args.distance)
   if self._destination then
      ai:set_think_output()
   end
end

local function rotate_about_y_axis(point, degrees)
   local radians = degrees / 180 * math.pi
   local q = Quaternion(Point3f(0, 1, 0), radians)
   return q:rotate(point)
end

local function random_xz_unit_vector()
   local unit_x = Point3f(1, 0, 0)
   local unit_y = Point3f(0, 1, 0)
   local angle = rng:get_real(0, 2 * math.pi)
   local vector = Quaternion(unit_y, angle):rotate(unit_x)
   return vector
end

function RunAwayFromEntity:_choose_destination(entity, threat, distance)
   local entity_location = entity:add_component('mob'):get_world_location()
   local threat_location = threat:add_component('mob'):get_world_location()
   local destination

   local opposite_direction = entity_location - threat_location
   opposite_direction.y = 0

   if opposite_direction:distance_squared() ~= 0 then
      opposite_direction:normalize()
   else
      opposite_direction = random_xz_unit_vector()
   end

   destination = self:_calculate_location(entity_location, opposite_direction, 0, distance)

   return destination
end

function RunAwayFromEntity:_calculate_location(start_location, direction, angle, distance)
   local vector = rotate_about_y_axis(direction, angle)
   vector:scale(distance)

   local new_location = start_location + vector

   local x = MathFns.round(new_location.x)
   local z = MathFns.round(new_location.z)
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
