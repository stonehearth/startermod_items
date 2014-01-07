local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1

function Idle:run(ai, entity)
   local countdown = math.random(1, 3)
   while true do
      if countdown <= 0 then
         ai:execute('stonehearth:idle:bored')
         countdown = math.random(1, 3)
      else
         ai:execute('stonehearth:idle:breathe')
         if not radiant.entities.is_carrying(entity) then
            countdown = countdown - 1
         end
      end
   end
end

return Idle
