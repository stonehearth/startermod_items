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
CombatIdleShuffle.priority = 2
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

   local weapon = radiant.entities.get_equipped_item(entity, 'mainHand')
   local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:weapon_data')
   local melee_range = stonehearth.combat:get_melee_range(entity, weapon_data, enemy)
   ai:execute('stonehearth:bump_against_entity', { entity = enemy, distance = melee_range })

   ai:execute('stonehearth:turn_to_face_entity', { entity = enemy })
   ai:execute('stonehearth:run_effect', { effect = 'combat_1h_idle' })
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
   enemy_direction:normalize()

   while num_angles > 0 do
      roll = rng:get_int(1, num_angles)
      angle = angles[roll]
      destination = self:_calculate_location(entity_location, enemy_direction, angle, distance)

      if destination ~= nil then
         if radiant.terrain.can_stand_on(entity, destination) then
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

local function _rotate_facing(point, degrees)
   local radians = degrees / 180 * math.pi
   local q = Quaternion(Point3f(0, 1, 0), radians)
   return q:rotate(point)
end

function CombatIdleShuffle:_calculate_location(start_location, enemy_direction, angle, distance)
   local vector = _rotate_facing(enemy_direction, angle)
   vector:scale(distance)

   local new_location = start_location + vector

   local x = MathFns.round(new_location.x)
   local z = MathFns.round(new_location.z)
   local y = radiant.terrain.get_height(x, z)

   if y == nil then
      return nil
   end

   local new_grid_location = Point3(x, y, z)
   return new_grid_location
end

function CombatIdleShuffle:_is_occupied(location)
   local cube = Cube3(location, location + Point3(1, 1, 1))
   local entities = radiant.terrain.get_entities_in_cube(cube)

   local occupied = next(entities) ~= nil
   return occupied
end

return CombatIdleShuffle
