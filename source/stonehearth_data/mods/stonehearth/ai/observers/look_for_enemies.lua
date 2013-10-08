local LookForEnemies = class()

LookForEnemies.name = 'stonehearth:actions:chase'
LookForEnemies.does = 'stonehearth:top'
LookForEnemies.priority = 0

function LookForEnemies:__init(entity)
   self._entity = entity
   self._aggro_table = radiant.entities.create_target_table(entity, 'aggro')
   radiant.events.listen('radiant:events:gameloop', self)
end

-- xxx: the 'fire one when i'm constructed' pattern again...
LookForEnemies['radiant:events:gameloop'] = function(self)
   radiant.events.unlisten('radiant:events:gameloop', self)
   self:init_sight_sensor()
end

function LookForEnemies:init_sight_sensor()
   assert(not self._sensor)

   local list = self._entity:get_component('sensor_list')
   if list then
      radiant.events.unlisten('radiant:events:gameloop', self)
   else 
      return
   end

   self._sensor = self._entity:get_component('sensor_list'):get_sensor('sight')
   self.promise = self._sensor:get_contents():trace()

   self.promise:on_added(
      function (id)
         self:on_added_to_sensor(id)
      end
   ):on_removed(
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
   end
end

function LookForEnemies:on_removed_to_sensor()
   -- xxx, remove from the aggro table? do nothing?
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
   local ok = entity:add_component('stonehearth:materials'):has_material('meat')
   if not ok then
      return false
   end

   local faction_a = self._entity:get_component('unit_info'):get_faction()
   local unit_info_b = entity:get_component('unit_info')

   if unit_info_b then 
      local faction_b = unit_info_b:get_faction()

      -- MEGA HACK ALERT. This should be in some kind of faction allegiance lookup
      if faction_a and faction_b and faction_a == 'civ' and faction_b == 'critter' then
         return false
      end
      -- END OF MEGA HACK
      
      return faction_a and faction_b and faction_b ~= '' and faction_a ~= faction_b
   else
      return false
   end
end

return LookForEnemies