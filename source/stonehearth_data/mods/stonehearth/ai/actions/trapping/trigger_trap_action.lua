local Entity = _radiant.om.Entity

local TriggerTrap = class()

TriggerTrap.name = 'trigger trap'
TriggerTrap.does = 'stonehearth:trapping:trigger_trap'
TriggerTrap.args = {
   trap = Entity
}
TriggerTrap.version = 2
TriggerTrap.priority = 1

function TriggerTrap:run(ai, entity, args)
   local trap = args.trap

   if not trap:is_valid() then
      ai:abort('trap is destroyed')
      return
   end

   local trap_component = trap:add_component('stonehearth:bait_trap')

   if not trap_component:is_armed() then
      ai:abort('trap is not armed')
      return
   end

   trap_component:try_trap(entity)
end

return TriggerTrap
