local DeathAction = class()

DeathAction.name = 'stonehearth.actions.die'
DeathAction.does = 'stonehearth.activities.top'
DeathAction.priority = 0

radiant.events.register_event('stonehearth.events.on_damage')

function DeathAction:__init(ai, entity)
   radiant.check.is_entity(entity)  

   self._ai = ai
   self._aggro_table = radiant.entities.create_target_table(entity, 'stonehearth.tables.aggro')

   radiant.events.listen_to_entity(entity, 'stonehearth.events.on_damage', self)
end

function DeathAction:run(ai, entity)
   ai:execute('radiant.actions.perform', 'combat/1h_downed')
   om:destroy_entity(entity)
end

function DeathAction:destroy(entity)
   om:destroy_target_table(entity, self._aggro_table)
   radiant.events.unlisten_to_entity(entity, 'stonehearth.events.on_damage', self)
end

DeathAction['stonehearth.events.on_damage'] = function(self, entity, source, amount, type)
   radiant.check.is_entity(entity)
   radiant.check.is_number(amount)
   radiant.check.is_string(type)

   local health = radiant.entities.get_attribute(entity, 'health');
   radiant.log.warning('health is %d before taking %d damage...', health, amount)

   health = radiant.entities.update_attribute(entity, 'health', -amount)
   radiant.log.warning('health is now %d...', health)
   
   if source then
      self._aggro_table:add_entry(source):set_value(amount)
   end

   if health <= 0 then
      self._ai:set_action_priority(self, 999999)
      self._death_reason = type
   end
end

return DeathAction
