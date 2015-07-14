local Entity = _radiant.om.Entity

local CheckBaitTrap = class()

CheckBaitTrap.name = 'check bait trap'
CheckBaitTrap.status_text_key = 'ai_status_text_check_bait_trap'
CheckBaitTrap.does = 'stonehearth:trapping:check_bait_trap'
CheckBaitTrap.args = {
   trapping_grounds = Entity
}
CheckBaitTrap.version = 2
CheckBaitTrap.priority = 1

function CheckBaitTrap:start_thinking(ai, entity, args)
   local sc = entity:get_component('stonehearth:storage')
   if sc and not sc:is_full() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(CheckBaitTrap)
   :execute('stonehearth:clear_carrying_now')
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
