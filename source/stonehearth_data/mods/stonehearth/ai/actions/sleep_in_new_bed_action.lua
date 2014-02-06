local Entity = _radiant.om.Entity
local SleepInNewBed = class()

SleepInNewBed.name = 'sleep in new bed'
SleepInNewBed.does = 'stonehearth:sleep_when_tired'
SleepInNewBed.args = { }
SleepInNewBed.version = 2
SleepInNewBed.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(SleepInNewBed)
         :when(function (ai, entity) 
               -- only run if we don't have a current bed lease
               local lease_component = entity:get_component('stonehearth:lease_holder')
               if not lease_component then
                  return true
               end
               local bed = lease_component:get_first_lease('stonehearth:bed')
               return not bed or not bed:is_valid()
            end)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity_with_entity_data', { key = 'stonehearth:bed' })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.entity })
         :execute('stonehearth:sleep_in_bed_adjacent', { bed = ai.PREV.entity })
