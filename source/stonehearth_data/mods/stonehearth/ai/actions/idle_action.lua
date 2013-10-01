local IdleAction = class()

IdleAction.name = 'stonehearth.actions.idle'
IdleAction.does = 'stonehearth.top'
IdleAction.priority = 1

function IdleAction:__init(ai, entity)
   self._entity = entity
   self:reset_boredom()
   if radiant.entities.get_entity_data(entity, 'stonehearth:idle_boredom_effects') then
      radiant.events.listen('radiant.events.calendar.minutely', self)
   end
end

IdleAction['radiant.events.calendar.minutely'] = function(self)
   self._boredomCountdown = self._boredomCountdown - 1
end

function IdleAction:reset_boredom()
   self._boredomCountdown = math.random(50, 100)
end

function IdleAction:run(ai, entity)
   self:reset_boredom()
   while true do
      if self._boredomCountdown <= 0 then 
         ai:execute('stonehearth.idle.bored')
         self:reset_boredom()
      else
         ai:execute('stonehearth.idle.breathe')
      end
   end
end

function IdleAction:stop()
   self:reset_boredom()
end

return IdleAction