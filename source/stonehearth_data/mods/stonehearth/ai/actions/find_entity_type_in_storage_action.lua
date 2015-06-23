local Entity = _radiant.om.Entity
local FindEntityTypeInStorageAction = class()

FindEntityTypeInStorageAction.name = 'reserve entity type in storage'
FindEntityTypeInStorageAction.does = 'stonehearth:find_entity_type_in_storage'
FindEntityTypeInStorageAction.args = {
   filter_fn = 'function',
   storage = Entity,          -- storage which potentially has the item
}
FindEntityTypeInStorageAction.think_output = {
   item = Entity,             -- the item found
}
FindEntityTypeInStorageAction.version = 2
FindEntityTypeInStorageAction.priority = 1

function FindEntityTypeInStorageAction:start_thinking(ai, entity, args)
   -- Look for _anything_ in the storage that passes our filter, and can be acquired.
   local storage = args.storage:get_component('stonehearth:storage')
   if not storage then
      return
   end

   for _, item in pairs(storage:get_items()) do
      if args.filter_fn(item) then
         if stonehearth.ai:can_acquire_ai_lease(item, entity) then
            ai:get_log():debug('no one holds lease yet on %s.', item)
            ai:set_think_output({ item = item })
            return
         end
      end
   end
end

return FindEntityTypeInStorageAction
