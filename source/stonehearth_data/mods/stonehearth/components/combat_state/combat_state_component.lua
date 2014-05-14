local CombatState = require 'components.combat_state.combat_state'

local CombatStateComponent = class()

function CombatStateComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   if true then
   --if not self._sv.initialized then
      self._sv.combat_state = self:_create_combat_state()
      self._sv.initialized = true
   else
      self._sv.combat_state = self:_create_combat_state(self._sv.combat_state)
   end

   -- ten second or minute poll is sufficient
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

function CombatStateComponent:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

function CombatStateComponent:get_combat_state()
   return self._sv.combat_state
end

function CombatStateComponent:_create_combat_state(datastore)
   datastore = datastore or radiant.create_datastore()

   local combat_state = CombatState()
   combat_state.__saved_variables = datastore
   combat_state:initialize()
   return combat_state
end

function CombatStateComponent:_clean_combat_state()
   self._sv.combat_state:remove_expired_cooldowns()
   self._sv.combat_state:remove_expired_assault_events()
end

return CombatStateComponent
