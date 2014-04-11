local rng = _radiant.csg.get_default_rng()
local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1
Idle.preemptable = true

function Idle:run(ai, entity)
   local delay = rng:get_int(1, 2)
   for i = 1, delay do
      ai:execute('stonehearth:idle:breathe')
   end
   ai:execute('stonehearth:idle:bored')
end

return Idle
