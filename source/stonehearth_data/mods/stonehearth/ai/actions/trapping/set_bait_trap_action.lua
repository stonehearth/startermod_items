local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local SetBaitTrap = class()

SetBaitTrap.name = 'set bait trap'
SetBaitTrap.status_text_key = 'ai_status_text_set_bait_trap'
SetBaitTrap.does = 'stonehearth:trapping:set_bait_trap'
SetBaitTrap.args = {
   location = Point3,
   trap_uri = 'string',
   trapping_grounds = Entity
}
SetBaitTrap.version = 2
SetBaitTrap.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(SetBaitTrap)
   :execute('stonehearth:clear_carrying_now')
   :execute('stonehearth:goto_location', {
      reason = 'set bait trap',
      location = ai.ARGS.location,
      stop_when_adjacent = true
   })
   :execute('stonehearth:trapping:set_bait_trap_adjacent', {
      location = ai.ARGS.location,
      trap_uri = ai.ARGS.trap_uri,
      trapping_grounds = ai.ARGS.trapping_grounds
   })
   :execute('stonehearth:trigger_event', {
      source = ai.ENTITY, 
      event_name = 'stonehearth:set_trap', 
      event_args = {}
   })
