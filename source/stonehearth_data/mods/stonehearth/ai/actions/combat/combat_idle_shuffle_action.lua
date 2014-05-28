local MathFns = require 'services.server.world_generation.math.math_fns'
local Entity = _radiant.om.Entity
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Cube3 = _radiant.csg.Cube3
local Quaternion = _radiant.csg.Quaternion
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local CombatIdleShuffle = class()

CombatIdleShuffle.name = 'combat idle shuffle'
CombatIdleShuffle.does = 'stonehearth:combat:idle'
CombatIdleShuffle.args = {
   enemy = Entity
}
CombatIdleShuffle.version = 2
CombatIdleShuffle.priority = 1
CombatIdleShuffle.weight = 1

function CombatIdleShuffle:start_thinking(ai, entity, args)
   local enemy = args.enemy

   if enemy == nil or not enemy:is_valid() then
      return
   end

   self._destination = self:_choose_destination(entity, enemy)
   if self._destination then
      ai:set_think_output()
   end
end

function CombatIdleShuffle:run(ai, entity, args)
   local enemy = args.enemy
   if not enemy:is_valid() then
      ai:abort('enemy has been destroyed')
      return
   end

   ai:execute('stonehearth:go_toward_location', { destination = self._destination })

   local weapon = stonehearth.combat:get_melee_weapon(entity)
   if weapon ~= nil and weapon:is_valid() then
      local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
      assert(weapon_data)
   
      local melee_range_ideal = stonehearth.combat:get_melee_range(entity, weapon_data, enemy)
      ai:execute('stonehearth:bump_against_entity', { entity = enemy, distance = melee_range_ideal })
   end

   ai:execute('stonehearth:turn_to_face_entity', { entity = enemy })
   ai:execute('stonehearth:run_effect', { effect = 'combat_1h_idle' })
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

function CombatIdleShuffle:_choose_destination(entity, enemy)
   local entity_location = entity:add_component('mob'):get_world_location()
   local enemy_location = enemy:add_component('mob'):get_world_location()
   local angles = { -90, -45, 45, 90 }
   local num_angles = #angles
   -- wandering at least sqrt(2) guarantees that diagonal movement will change grid location
   local distance = 1.5
   local roll, angle, destination

   local enemy_direction = enemy_location - entity_location
   enemy_direction.y = 0

   if enemy_direction:distance_squared() ~= 0 then
      enemy_direction:normalize()
   else
      enemy_direction = random_xz_unit_vector()
   end

   while num_angles > 0 do
      roll = rng:get_int(1, num_angles)
      angle = angles[roll]
      destination = self:_calculate_location(entity, entity_location, enemy_direction, angle, distance)

      if destination ~= nil then
         if radiant.terrain.is_standable(entity, destination) then
            if not self:_is_occupied(destination) then
               return destination
            end
         end
      end

      table.remove(angles, roll)
      num_angles = num_angles - 1
   end

   return nil
end

function CombatIdleShuffle:_calculate_location(entity, start_location, enemy_direction, angle, distance)
   local vector = rotate_about_y_axis(enemy_direction, angle)
   vector:scale(distance)

   local new_location = start_location + vector
   return radiant.terrain.get_standable_point(entity, new_location:to_int())
end

function CombatIdleShuffle:_is_occupied(location)
   local cube = Cube3(location, location + Point3(1, 1, 1))
   local entities = radiant.terrain.get_entities_in_cube(cube)

   local occupied = next(entities) ~= nil
   return occupied
end

return CombatIdleShuffle
