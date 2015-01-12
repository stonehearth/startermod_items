local rng = _radiant.csg.get_default_rng()
local Idle = class()

Idle.name = 'idle'
Idle.does = 'stonehearth:idle'
Idle.status_text = 'idle'
Idle.args = {
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }
}
Idle.version = 2
Idle.priority = 1

function Idle:run(ai, entity, args)
   while true do
      local delay = rng:get_int(1, 2)

      if radiant.entities.is_carrying(entity) then
         -- we usually only end up carrying something while idling when some
         -- unexpected circumstance caused an action to exit early (e.g. deleting
         -- a stockpile while someone is carrying an item to it).  there might not
         -- be another action that wants to do anything with us while we're carrying
         -- this thing, so drop it on the first bored cycle.  wait an extra long time,
         -- just in case there's something we *could* do, but it will take a long time
         -- to figure out.
         delay = delay + 5
      end

      for i = 1, delay do
         ai:execute('stonehearth:idle:breathe')
      end

      if radiant.entities.is_carrying(entity) then
         -- still carrying?  we must be doomed!  go ahead and drop it.
         if not args.hold_position then
            ai:execute('stonehearth:wander_within_leash', { radius = 3 })
         end
         ai:execute('stonehearth:drop_carrying_now')
      end
      ai:execute('stonehearth:idle:bored', {
            hold_position = args.hold_position
         })
   end
end

return Idle
