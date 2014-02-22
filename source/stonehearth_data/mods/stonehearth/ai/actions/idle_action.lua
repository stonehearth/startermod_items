local rng = _radiant.csg.get_default_rng()
local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.args = { }
Idle.version = 2
Idle.priority = 1
Idle.preemptable = true

function Idle:run(ai, entity)
   local countdown = rng:get_int(1, 2)

   -- we usually only end up carrying something while idling when some
   -- unexpected circumstance caused an action to exit early (e.g. deleting
   -- a stockpile while someone is carrying an item to it).  there might not
   -- be another action that wants to do anything with us while we're carrying
   -- this thing, so drop it on the first bored cycle.  wait an extra long time,
   -- just in case there's something we *could* do, but it will take a long time
   -- to figure out.
   if radiant.entities.is_carrying(entity) then
      countdown = countdown + 5
   end

   while true do
      if countdown <= 0 then
         if radiant.entities.is_carrying(entity) then
            -- still carrying?  we must be doomed!  go ahead and drop it.
            ai:execute('stonehearth:wander', { radius = 3 })
            ai:execute('stonehearth:drop_carrying_now')
         else
            ai:execute('stonehearth:idle:bored')
         end
         countdown = rng:get_int(1, 2)
      else
         ai:execute('stonehearth:idle:breathe')
         countdown = countdown - 1
      end
   end
end

return Idle
