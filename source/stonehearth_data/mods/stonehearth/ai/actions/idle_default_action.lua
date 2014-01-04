local Idle = class()

Idle.name = 'default idle behavior'
Idle.does = 'stonehearth:idle'
Idle.version = 1
Idle.priority = 1

function Idle:__init(ai, entity)
   self._entity = entity
   self:_reset_boredom()
end

function Idle:_reset_boredom()
   self._boredomCountdown = math.random(1, 3)
end

function Idle:run(ai, entity)
   self:_reset_boredom()
   while true do
      if self._boredomCountdown <= 0 then
         ai:execute('stonehearth:idle:bored')
         self:reset_boredom()
      else
         ai:execute('stonehearth:idle:breathe')
         self._boredomCountdown = self._boredomCountdown - 1
      end
   end
end

function Idle:stop()
   self:reset_boredom()
end

return Idle