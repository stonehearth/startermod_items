local event_service = require 'services.event.event_service'

local IdleWarmHands = class()

IdleWarmHands.name = 'warm hands'
IdleWarmHands.does = 'stonehearth:idle:warm_hands'
IdleWarmHands.version = 1
IdleWarmHands.priority = 0

function IdleWarmHands:__init(ai, entity, fire_entity)
   self._ai = ai
   self._fire_entity = fire_entity
   local component = fire_entity:get_component('stonehearth:firepit')
   if component then 
      radiant.events.listen(self._fire_entity, 'stonehearth:fire:lit', self, self.on_fire_lit)
   end
end

function IdleWarmHands:on_fire_lit(e) 
   if e.lit then
      self._ai:set_action_priority(self, 1)
   else
      self._ai:set_action_priority(self, 0)
   end
end

function IdleWarmHands:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_warm_hands')
end

function IdleWarmHands:destroy()
   radiant.events.unlisten(self._fire_entity, 'stonehearh:fire:lit', self, self.on_fire_lit)
end

return IdleWarmHands
