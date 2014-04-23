local MathFns = require 'services.server.world_generation.math.math_fns'
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
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

local function _rotate_facing(point, degrees)
   local radians = degrees / 180 * math.pi
   local q = Quaternion(Point3f(0, 1, 0), radians)
   return q:rotate(point)
end

function CombatIdleShuffle:run(ai, entity, args)
   local enemy = args.enemy
   if not enemy:is_valid() then
      ai:abort('enemy has been destroyed')
      return
   end

   local entity_location = entity:add_component('mob'):get_world_location()
   local enemy_location = enemy:add_component('mob'):get_world_location()

   local enemy_direction = enemy_location - entity_location
   enemy_direction.y = 0
   enemy_direction:normalize()

   local turn_magnitude = rng:get_int(1, 2) * 45
   local turn_sign = rng:get_int(0, 1) * 2 - 1
   local angle = turn_sign * turn_magnitude

   local wander_vector = _rotate_facing(enemy_direction, angle)
   -- wandering at least sqrt(2) guarantees that diagonal movement will change grid location
   wander_vector:scale(1.5)

   local wander_location = entity_location + wander_vector
   local wander_grid_location = Point3(
      MathFns.round(wander_location.x),
      MathFns.round(wander_location.y),
      MathFns.round(wander_location.z)
   )

   ai:execute('stonehearth:go_toward_location', { destination = wander_grid_location })

   local melee_range = stonehearth.combat:get_melee_range(entity, 'medium_1h_weapon', enemy)
   ai:execute('stonehearth:bump_against_entity', { entity = enemy, distance = melee_range })

   ai:execute('stonehearth:turn_to_face_entity', { entity = enemy })
   ai:execute('stonehearth:run_effect', { effect = 'combat_1h_idle' })
end

return CombatIdleShuffle
