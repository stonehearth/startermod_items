local Entity = _radiant.om.Entity
local SleepInBed = class()

SleepInBed.name = 'pickup an item'
SleepInBed.does = 'stonehearth:sleep_when_tired'
SleepInBed.args = { }
SleepInBed.version = 2
SleepInBed.priority = 1

function is_bed(entity)
   return radiant.entities.get_entity_data(entity, 'stonehearth:bed') ~= nil
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepInBed)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity_with_entity_data', { key = 'stonehearth:bed' })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.entity })
         :execute('stonehearth:sleep_in_bed_adjacent', { bed = ai.PREV.entity })
