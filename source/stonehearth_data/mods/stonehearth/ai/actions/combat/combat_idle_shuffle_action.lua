local Entity = _radiant.om.Entity
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Cube3 = _radiant.csg.Cube3
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
   self._entity = entity
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

   ai:execute('stonehearth:bump_allies', { distance = 2 })

   ai:execute('stonehearth:turn_to_face_entity', { entity = enemy })
   ai:execute('stonehearth:run_effect', { effect = 'combat_1h_idle' })
end

function CombatIdleShuffle:_choose_destination(entity, enemy)
   local entity_grid_location = entity:add_component('mob'):get_world_grid_location()
   local entity_location = entity:add_component('mob'):get_world_location()
   local enemy_location = enemy:add_component('mob'):get_world_location()
   local angles = { -90, -45, 45, 90 }
   local num_angles = #angles
   -- wandering at least sqrt(2) guarantees that diagonal movement will change grid location
   local distance = 1.5
   local radius = 2
   local roll, angle, destination
   local entities, current_density, destination_density

   entities = self:_get_entities_in_cube(entity_grid_location, radius)
   current_density = self:_count_other_entities(entities)

   local enemy_direction = enemy_location - entity_location
   enemy_direction.y = 0

   if enemy_direction:distance_squared() ~= 0 then
      enemy_direction:normalize()
   else
      enemy_direction = radiant.math.random_xz_unit_vector()
   end

   while num_angles > 0 do
      roll = rng:get_int(1, num_angles)
      angle = angles[roll]
      destination = self:_calculate_destination(entity, entity_location, enemy_direction, angle, distance)

      if destination ~= nil then
         -- don't shuffle to a destination that is more crowded than the current location
         entities = self:_get_entities_in_cube(destination, radius)
         destination_density = self:_count_other_entities(entities)

         if destination_density <= current_density and not self:_destination_is_occupied(destination, entities) then
            return destination
         end

         -- simple version - don't shuffle to an occupied destination
         -- if not self:_is_occupied(destination) then
         --    return destination
         -- end
      end

      table.remove(angles, roll)
      num_angles = num_angles - 1
   end

   return nil
end

function CombatIdleShuffle:_calculate_destination(entity, start_location, direction, angle, distance)
   local vector = radiant.math.rotate_about_y_axis(direction, angle)
   vector:scale(distance)

   local proposed_destination = start_location + vector
   local actual_destination = radiant.terrain.get_standable_point(entity, proposed_destination:to_int())
   return actual_destination
end

function CombatIdleShuffle:_get_entities_in_cube(location, radius)
   -- also include entities on 1 voxel elevation changes
   local cube = Cube3(location + Point3(-radius, -1, -radius),
                      location + Point3(radius+1, 1+1, radius+1))

   local entities = radiant.terrain.get_entities_in_cube(cube)
   return entities
end

function CombatIdleShuffle:_destination_is_occupied(destination, entities)
   for _, entity in pairs(entities) do
      local mob = entity:get_component('mob')
      if mob and mob:get_world_grid_location() == destination then
         return true
      end
   end
   return false
end

function CombatIdleShuffle:_count_other_entities(entities)
   local count = 0

   for _, entity in pairs(entities) do
      if entity ~= self._entity then
         count = count + 1
      end
   end

   return count
end

-- currently unused
function CombatIdleShuffle:_is_occupied(location)
   local cube = Cube3(location, location + Point3(1, 1, 1))
   local entities = radiant.terrain.get_entities_in_cube(cube)

   local occupied = next(entities) ~= nil
   return occupied
end

return CombatIdleShuffle
