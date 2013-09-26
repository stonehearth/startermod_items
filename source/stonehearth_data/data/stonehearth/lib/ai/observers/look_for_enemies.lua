local LookForEnemies = class()

LookForEnemies.name = 'stonehearth.actions.chase'
LookForEnemies.does = 'stonehearth.activities.top'
LookForEnemies.priority = 0

function LookForEnemies:__init(entity)
   self._entity = entity
   self._aggro_table = radiant.entities.create_target_table(entity, 'aggro')
   radiant.events.listen('radiant.events.very_slow_poll', self)
end

LookForEnemies['radiant.events.very_slow_poll'] = function(self)
   self:init_sight_sensor();

   if self._sensor then
      for i, target in ipairs(self:get_hostile_entities()) do
         --radiant.log.info('  adding %s to aggro table of %s', tostring(target), tostring(self._entity))
         self._aggro_table:add_entry(target)
                          :set_value(1)
      end
      --radiant.log.info('---')
   end

end

--- Grab and store the sight sensor from this observer's entity
function LookForEnemies:init_sight_sensor()
   if not self._sensor then
      local sensor_list = self._entity:get_component('sensor_list')
      
      if not sensor_list then 
         return 
      end
      
      self._sensor = sensor_list:get_sensor('sight')   
   end
end

--- Compute the entities that are hostile to this entity.
function LookForEnemies:get_hostile_entities()
   assert(self._sensor)

   local items = {}
   for id in self._sensor:get_contents():items() do table.insert(items, id) end

   local index = 1   
   local hostile_entities = {}
   for index = 1, #items do
      local id = items[index]
      local entity = radiant.entities.get_entity(id)

      --radiant.log.info('sensor found %s', tostring(id))
      --radiant.log.info('  %s', tostring(entity))

      if radiant.check.is_entity(entity) and self:is_hostile(entity) then
         table.insert(hostile_entities,entity)
      end
   end

   return hostile_entities
end

--- Check if an entity is hostile to the entity that owns this observer
-- For now this is just a simple faction check. If the entity is a member of
-- a different faction, it's hostile.
-- @param entity The entity to determine whether
function LookForEnemies:is_hostile(entity)
   local faction_a = self._entity:get_component('unit_info'):get_faction()
   local unit_info_b = entity:get_component('unit_info')

   if unit_info_b then 
      local faction_b = unit_info_b:get_faction()
      return faction_a and faction_b and faction_b ~= '' and faction_a ~= faction_b
   else
      return false
   end
end

return LookForEnemies