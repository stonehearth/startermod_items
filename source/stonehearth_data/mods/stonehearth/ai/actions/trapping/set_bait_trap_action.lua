local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local SetBaitTrap = class()

SetBaitTrap.name = 'set bait trap'
SetBaitTrap.status_text = 'setting traps'
SetBaitTrap.does = 'stonehearth:trapping:set_bait_trap'
SetBaitTrap.args = {
   location = Point3,
   trap_uri = 'string',
   trapping_grounds = Entity
}
SetBaitTrap.version = 2
SetBaitTrap.priority = 1

function SetBaitTrap:start_thinking(ai, entity, args)
   -- can't perform this action if we're carrying anything
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(SetBaitTrap)
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:goto_location', {
      location = ai.ARGS.location,
      stop_when_adjacent = true
   })
   :execute('stonehearth:trapping:set_bait_trap_adjacent', {
      location = ai.ARGS.location,
      trap_uri = ai.ARGS.trap_uri,
      trapping_grounds = ai.ARGS.trapping_grounds
   })
