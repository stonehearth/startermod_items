local IdleSwayAction = class()

IdleSwayAction.name = 'stonehearth.actions.look_around'
IdleSwayAction.does = 'stonehearth.idle'
IdleSwayAction.priority = 0

function IdleSwayAction:__init(ai, entity)
   self._ai = ai;
   self._entity = entity;
   self._attributes = entity:add_component('attributes');
   radiant.events.listen('radiant.events.calendar.minutely', self)
end

IdleSwayAction['radiant.events.calendar.minutely'] = function(self)
   local boredom = self._attributes:get_attribute('boredom')
   local carrying = radiant.entities.get_carrying(self._entity)

   if not carrying and boredom > 100 then
      self._ai:set_action_priority(self, 2)     
   end
end

function IdleSwayAction:run(ai, entity)
   local sways = math.random(1, 3)
   for _=1, sways do
      ai:execute('stonehearth.run_effect', 'idle_sway')
   end
   ai:set_action_priority(self, 0)
end

return IdleSwayAction