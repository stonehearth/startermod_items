local Entity = _radiant.om.Entity
local PickupItemWithUri = class()

PickupItemWithUri.name = 'pickup item with uri'
PickupItemWithUri.does = 'stonehearth:pickup_item_with_uri'
PickupItemWithUri.args = {
   uri = 'string',      -- uri we want
}
PickupItemWithUri.think_output = {
   item = Entity,            -- what was actually picked up
}
PickupItemWithUri.version = 2
PickupItemWithUri.priority = 1

function PickupItemWithUri:start_thinking(ai, entity, args)
   local uri = args.uri
   local function filter(entity)
      if entity:get_uri() == uri then
         return true
      end   
      local proxy = entity :get_component('stonehearth:placeable_item_proxy')
      if proxy then
         return proxy:get_full_sized_entity_uri() == uri
      end
      
      return false
   end
   ai:set_think_output({
         filter_key = 'uri: ' .. uri,
         filter_fn = filter,
         description = uri
      })
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemWithUri)
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.filter_key,
            filter_fn = ai.PREV.filter_fn,
            description = ai.PREV.description,
         })
         :set_think_output({ item = ai.PREV.item })
