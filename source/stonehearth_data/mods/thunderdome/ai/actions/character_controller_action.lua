local CharacterControllerAction = class()

CharacterControllerAction.name = 'character controller'
CharacterControllerAction.does = 'stonehearth:top'
CharacterControllerAction.version = 1
CharacterControllerAction.priority = 5

function CharacterControllerAction:__init(ai, entity)
   self._ai = ai
   radiant.events.listen(entity, "thunderdome:run_effect", self, self.on_run_effect)
end

function CharacterControllerAction:run(ai, entity)
   if self._effect then
      if self._effect_delay then
         ai:wait_until(radiant.gamestate.now() + self._effect_delay)
      end
      ai:execute('stonehearth:run_effect', self._effect)
      self._effect = nil
      self._ai:set_action_priority(self, 5)
   end
   
   ai:execute('stonehearth:run_effect', 'combat_1h_idle')
   

end

function CharacterControllerAction:on_run_effect(e)
   self._effect = e.effect

   self._effect_delay = e.start_time

   self._ai:set_action_priority(self, 1000)
   self._ai:abort()
end

function CharacterControllerAction:stop() 
   local foo = 1
end

return CharacterControllerAction