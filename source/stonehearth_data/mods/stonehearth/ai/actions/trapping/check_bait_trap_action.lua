local Entity = _radiant.om.Entity

local CheckBaitTrap = class()

CheckBaitTrap.name = 'check bait trap'
CheckBaitTrap.status_text = 'checking traps'
CheckBaitTrap.does = 'stonehearth:trapping:check_bait_trap'
CheckBaitTrap.args = {
   trapping_grounds = Entity
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
   :execute('stonehearth:trapping:find_trap_in_trapping_grounds', {
      trapping_grounds = ai.ARGS.trapping_grounds
   })
   :execute('stonehearth:reserve_entity', {
      entity = ai.BACK(1).trap
   })
   :execute('stonehearth:follow_path', {
      path = ai.BACK(2).path
   })
   :execute('stonehearth:trapping:check_bait_trap_adjacent', {
      trap = ai.BACK(3).trap
   })
