local rng = _radiant.csg.get_default_rng()
local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1
Idle.preemptable = true

function Idle:run(ai, entity)
   local countdown = rng:get_int(0, 1)
   while true do
      if countdown <= 0 then

         if rng:get_int(0, 1) == 1 then
            ai:execute('stonehearth:wander', { radius = 5, radius_min = 2 })
         else 
            ai:execute('stonehearth:idle:bored')
         end

         countdown = rng:get_int(0, 1)
      else
         ai:execute('stonehearth:idle:breathe')
         if not radiant.entities.is_carrying(entity) then
            countdown = countdown - 1
         end
      end
   end
end

return Idle
