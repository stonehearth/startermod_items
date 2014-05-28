local CombatState = require 'components.combat_state.combat_state'

local CombatStateComponent = class()

function CombatStateComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   local combat_state = CombatState()

   if not self._sv.initialized then
      combat_state:initialize()
      self._sv.initialized = true
   else
      combat_state:restore(self._sv.combat_state)
   end

   self._sv.combat_state = combat_state

   -- ten second or minute poll is sufficient
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

function CombatStateComponent:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

function CombatStateComponent:get_combat_state()
   return self._sv.combat_state
end

function CombatStateComponent:_clean_combat_state()
   self._sv.combat_state:remove_expired_cooldowns()
   self._sv.combat_state:remove_expired_assault_events()
end

return CombatStateComponent
