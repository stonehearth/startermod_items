local Entity = _radiant.om.Entity
local KeyToEntityDataFilter = class()

KeyToEntityDataFilter.name = 'entity data key to filter fn'
KeyToEntityDataFilter.does = 'stonehearth:key_to_entity_data_filter_fn'
KeyToEntityDataFilter.args = {
   key = 'string',            -- the key for the entity_data to look for
}
KeyToEntityDataFilter.think_output = {
   filter_fn = 'function',    -- a function which checks for that key
}
KeyToEntityDataFilter.version = 2
KeyToEntityDataFilter.priority = 1

function KeyToEntityDataFilter:start_thinking(ai, entity, args)
   local filter_fn = function(item)  
      return radiant.entities.get_entity_data(item, args.key) ~= nil
   end
   ai:set_think_output({ filter_fn = filter_fn })
end

return KeyToEntityDataFilter
