local Entity = _radiant.om.Entity
local ReserveBackpackEntityType = class()

ReserveBackpackEntityType.name = 'reserve entity'
ReserveBackpackEntityType.does = 'stonehearth:find_backpack_entity_type'
ReserveBackpackEntityType.args = {
   filter_fn = 'function',
   backpack_entity = Entity,       -- backpack to reserve
}
ReserveBackpackEntityType.think_output = {
   item = Entity,                  -- the item reserved
}
ReserveBackpackEntityType.version = 2
ReserveBackpackEntityType.priority = 1

function ReserveBackpackEntityType:start_thinking(ai, entity, args)
   -- Look for _anything_ in the backpack that passes our filter, and can be acquired.
   local backpack = backpack_entity:get_component('stonehearth:backpack')

   for _, item in pairs(backpack:get_items()) do
      if filter_fn(item) then
         if stonehearth.ai:can_acquire_ai_lease(item, entity) then
            ai:get_log():debug('no one holds lease yet on %s.', item)
            ai:set_think_output()
            return
         end
      end
   end

   local reason = string.format('%s could not find a backpack item to acquire ai lease on %s.', tostring(entity), tostring(args.backpack_entity))
   ai:get_log():debug('halting: ' .. reason)
   ai:halt(reason)
end

function ReserveBackpackEntityType:start(ai, entity, args)
   local target = args.backpack_entity

   ai:get_log():debug('trying to acquire lease on a backpack item...')
   for _, item in pairs(backpack:get_items()) do
      if filter_fn(item) then
         if stonehearth.ai:acquire_ai_lease(item, entity) then
            ai:get_log():debug('got backpack lease on %s.', item)
            self._acquired_item = item
            return
         end
      end
   end
   ai:abort('could not lease anything in backpack %s', tostring(target))
end

function ReserveBackpackEntityType:stop(ai, entity, args)
   if self._acquired_item then
      stonehearth.ai:release_ai_lease(self._acquired_item, entity)
      self._acquired_item = nil
   end
end

return ReserveBackpackEntityType
