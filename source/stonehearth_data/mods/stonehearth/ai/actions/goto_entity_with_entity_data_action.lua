local Entity = _radiant.om.Entity
local GotoItemWithEntityData = class()

GotoItemWithEntityData.name = 'goto entity with entity data'
GotoItemWithEntityData.does = 'stonehearth:goto_entity_with_entity_data'
GotoItemWithEntityData.args = {   
   key = 'string',       -- the name of the entity_data to find
}
GotoItemWithEntityData.think_output = {
   entity = Entity,      -- the entity which has it
}
GotoItemWithEntityData.version = 2
GotoItemWithEntityData.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GotoItemWithEntityData)
         :execute('stonehearth:key_to_entity_data_filter_fn', { key = ai.ARGS.key })
         :execute('stonehearth:goto_entity_type', {
            description = ai.ARGS.key,
            filter_key = ai.PREV.filter_key,
            filter_fn = ai.PREV.filter_fn,
         })
         :set_think_output({ entity = ai.PREV.destination_entity })
         
