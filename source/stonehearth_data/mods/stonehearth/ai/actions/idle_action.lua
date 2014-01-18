local rng = _radiant.csg.get_default_random_number_generator()
local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1

function Idle:run(ai, entity)
   local countdown = rng:get_int(1, 4)
   while true do
      if countdown <= 0 then
         ai:execute('stonehearth:idle:bored')
         countdown = rng:get_int(1, 4)
      else
         ai:execute('stonehearth:idle:breathe')
         if not radiant.entities.is_carrying(entity) then
            countdown = countdown - 1
         end
      end
   end
end

return Idle
