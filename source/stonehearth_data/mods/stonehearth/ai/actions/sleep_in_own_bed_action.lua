local Entity = _radiant.om.Entity
local SleepInOwnBed = class()

SleepInOwnBed.name = 'sleep in own bed'
SleepInOwnBed.does = 'stonehearth:sleep_when_tired'
SleepInOwnBed.args = { }
SleepInOwnBed.version = 2
SleepInOwnBed.priority = 1

function is_bed(entity)
   return radiant.entities.get_entity_data(entity, 'stonehearth:bed') ~= nil
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepInOwnBed)
         :when(function (ai, entity) 
               -- only run if we have a bed
               local lease_component = entity:get_component('stonehearth:lease_holder')
               if lease_component then
                  local bed = lease_component:get_first_lease('stonehearth:bed')
                  if bed and bed:is_valid() then
                     ai.CURRENT.owned_bed = bed
                     return true
                  end
               end
            end)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:reserve_entity', { entity =  ai.CURRENT.owned_bed })
         :execute('stonehearth:goto_entity', { entity = ai.CURRENT.owned_bed })
         :execute('stonehearth:sleep_in_bed_adjacent', { bed = ai.CURRENT.owned_bed })
