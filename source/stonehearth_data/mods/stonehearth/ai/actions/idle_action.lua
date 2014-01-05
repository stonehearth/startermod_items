local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1

function Idle:__init(ai, entity)
   self._entity = entity
   self:reset_boredom()
end

function Idle:reset_boredom()
   self._boredomCountdown = math.random(1, 3)
end

function Idle:run(ai, entity)
   self._finished = false
   if self._boredomCountdown <= 0 then
      ai:execute('stonehearth:idle:bored')
      self:reset_boredom()
   else
      ai:execute('stonehearth:idle:breathe')
      if not radiant.entities.is_carrying(entity) then
         self._boredomCountdown = self._boredomCountdown - 1
      end
   end
   
   self._finished = true
end

function Idle:stop(ai, entity)
   if not self._finished then
      self:reset_boredom()
   end
end

return Idle
