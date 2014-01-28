local event_service = stonehearth.events

local IdleWarmHands = class()

IdleWarmHands.name = 'warm hands'
IdleWarmHands.does = 'stonehearth:idle:warm_hands'
IdleWarmHands.args = {
   
}
IdleWarmHands.version = 2
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
   if e.lit and not self._signaled then
      self._signaled = true
      self._ai:set_think_output()
   elseif not e.lit and self._signaled then
      self._signaled = false
      self._ai:clear_think_output()
   end
end

function IdleWarmHands:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_warm_hands')
end

function IdleWarmHands:destroy()
   radiant.events.unlisten(self._fire_entity, 'stonehearh:fire:lit', self, self.on_fire_lit)
end

return IdleWarmHands
