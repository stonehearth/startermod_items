local Entity = _radiant.om.Entity

local CheckBaitTrap = class()

CheckBaitTrap.name = 'check bait trap'
CheckBaitTrap.status_text = 'checking traps'
CheckBaitTrap.does = 'stonehearth:trapping:check_bait_trap'
CheckBaitTrap.args = {
   trap = Entity
}
CheckBaitTrap.version = 2
CheckBaitTrap.priority = 1

function CheckBaitTrap:start_thinking(ai, entity, args)
   local backpack_component = entity:add_component('stonehearth:backpack')
   if backpack_component and not backpack_component:is_full() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(CheckBaitTrap)
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:goto_entity', {
      entity = ai.ARGS.trap,
      stop_distance = 1.7
   })
   :execute('stonehearth:trapping:check_bait_trap_adjacent', {
      trap = ai.ARGS.trap
   })
