local Entity = _radiant.om.Entity
local FindBackpackEntityType = class()

FindBackpackEntityType.name = 'find backpack entity'
FindBackpackEntityType.does = 'stonehearth:find_backpack_entity_type'
FindBackpackEntityType.args = {
   filter_fn = 'function',
   backpack_entity = Entity,       -- backpack to reserve
}
FindBackpackEntityType.think_output = {
   item = Entity,                  -- the item reserved
}
FindBackpackEntityType.version = 2
FindBackpackEntityType.priority = 1

function FindBackpackEntityType:start_thinking(ai, entity, args)
   -- Look for _anything_ in the backpack that passes our filter, and can be acquired.
   local backpack = args.backpack_entity:get_component('stonehearth:backpack')

   for _, item in pairs(backpack:get_items()) do
      if args.filter_fn(item) then
         if stonehearth.ai:can_acquire_ai_lease(item, entity) then
            ai:get_log():debug('no one holds lease yet on %s.', item)
            ai:set_think_output({ item = item })
            return
         end
      end
   end

   local reason = string.format('%s could not find a filtered backpack item to acquire ai lease on %s.', tostring(entity), tostring(args.backpack_entity))
   ai:get_log():debug('halting: ' .. reason)
   ai:halt(reason)
end

return FindBackpackEntityType
