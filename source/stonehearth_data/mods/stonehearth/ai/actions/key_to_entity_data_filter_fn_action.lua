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

ALL_FILTER_FNS = {}

function KeyToEntityDataFilter:start_thinking(ai, entity, args)
   -- make sure we return the exact same filter function for all
   -- keys so we can share the same pathfinders.  returning an
   -- equivalent implementation is not sufficient!  it must be
   -- the same function (see the 'stonehearth:pathfinder' component)
   
   local filter_fn = ALL_FILTER_FNS[args.key]
   if not filter_fn then
      local key = args.key
      filter_fn = function(item)
         return radiant.entities.get_entity_data(item, key) ~= nil
      end
      ALL_FILTER_FNS[key] = filter_fn
   end

   ai:set_think_output({
         filter_fn = filter_fn,
      })
end

return KeyToEntityDataFilter
