local LookForEnemies = class()
LookForEnemies.version = 2

function LookForEnemies:__init(entity)
   self._entity = entity
   self._aggro_table = radiant.entities.create_target_table(entity, 'aggro')
   radiant.events.listen(radiant, 'stonehearth:gameloop', self, self.on_gameloop)
end

-- xxx: the 'fire one when i'm constructed' pattern again...
function LookForEnemies:on_gameloop()
   radiant.events.unlisten(radiant, 'stonehearth:gameloop', self, self.on_gameloop)
   self:init_sight_sensor()
end

function LookForEnemies:init_sight_sensor()
   assert(not self._sensor)

   local list = self._entity:get_component('sensor_list')
   if not list then
      -- disable until we decide what to do with this
      return
   end

   local sensor_list = self._entity:add_component('sensor_list')
   local sensor = sensor_list:get_sensor('sight')

   if not sensor then
      sensor = sensor_list:add_sensor('sight', 64)
   end

   self._sensor = sensor

   self.promise = self._sensor:trace_contents('look for enemies')
      :on_added(
         function (id)
            self:on_added_to_sensor(id)
         end
      )
      :on_removed(
         function (id)
            self:on_removed_to_sensor(id)
         end
      ) 
end

function LookForEnemies:on_added_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)

   if self:is_hostile(entity) then
      self._aggro_table:add_entry(entity)
                       :set_value(1)

      local target = radiant.entities.get_target_table_top(self._entity, 'aggro')
   end
end

function LookForEnemies:on_removed_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)
   if entity then
      self._aggro_table:add_entry(entity)
                       :set_value(0)
   end
end

--- Check if an entity is hostile to the entity that owns this observer
-- For now this is just a simple faction check. If the entity is a member of
-- a different faction, it's hostile.
-- @param entity The entity to determine whether
function LookForEnemies:is_hostile(entity)
   -- xxx, this should be pulled out and parameterized for each entity
   if not entity then
      return false
   end

   -- only attack mobs
   local material = entity:get_component('stonehearth:material')
   if not material or not material:is('meat') then
      return false
   end

   local faction_a = self._entity:get_component('unit_info'):get_faction()
   local unit_info_b = entity:get_component('unit_info')

   if unit_info_b then 
      local faction_b = unit_info_b:get_faction()

      -- MEGA HACK ALERT. This should be in some kind of faction allegiance lookup
      if faction_a and faction_b and faction_a == 'civ' and faction_b ~= 'civ' then
         return false
      end
      -- END OF MEGA HACK
      
      return faction_a and faction_b and faction_b ~= '' and faction_a ~= faction_b
   else
      return false
   end
end

return LookForEnemies
