local Entity = _radiant.om.Entity
local SleepInOwnBed = class()

SleepInOwnBed.name = 'sleep in own bed'
SleepInOwnBed.does = 'stonehearth:sleep'
SleepInOwnBed.args = {}
SleepInOwnBed.version = 2
SleepInOwnBed.priority = 1

function SleepInOwnBed:start_thinking(ai, entity, args)
   local lease_component = entity:get_component('stonehearth:lease_holder')
   local bed = lease_component and lease_component:get_first_lease('stonehearth:bed')
   if bed and bed:is_valid() then
      ai:set_think_output({ owned_bed = bed })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepInOwnBed)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:reserve_entity', { entity = ai.BACK(2).owned_bed })
         :execute('stonehearth:goto_entity', { entity = ai.BACK(3).owned_bed })
         :execute('stonehearth:sleep_in_bed_adjacent', { bed = ai.BACK(4).owned_bed })
