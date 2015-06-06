local Entity = _radiant.om.Entity
local ReserveBackpackEntityType = class()

ReserveBackpackEntityType.name = 'reserve entity'
ReserveBackpackEntityType.does = 'stonehearth:reserve_backpack_entity_type'
ReserveBackpackEntityType.args = {
   filter_fn = 'function',
   backpack_entity = Entity,       -- backpack to reserve
}
ReserveBackpackEntityType.version = 2
ReserveBackpackEntityType.priority = 1

function ReserveBackpackEntityType:start_thinking(ai, entity, args)
   if not stonehearth.ai:can_acquire_ai_lease(args.entity, entity) then
      local reason = string.format('%s cannot acquire ai lease on %s.', tostring(entity), tostring(args.entity))
      ai:get_log():debug('halting: ' .. reason)
      ai:halt(reason)
      return
   end
   ai:get_log():debug('no one holds lease yet.')
   ai:set_think_output()
end

function ReserveBackpackEntityType:stop_thinking(ai, entity, args)
end

function ReserveBackpackEntityType:start(ai, entity, args)
   local target = args.entity

   ai:get_log():debug('trying to acquire lease...')
   if not stonehearth.ai:acquire_ai_lease(target, entity) then
      ai:abort('could not lease %s (%s has it).', tostring(target), tostring(stonehearth.ai:get_ai_lease_owner(target)))
      return
   end
   ai:get_log():debug('got lease!')
   self._acquired_lease = true
end

function ReserveBackpackEntityType:stop(ai, entity, args)   
   if self._acquired_lease then
      stonehearth.ai:release_ai_lease(args.entity, entity)
      self._acquired_lease = false
   end
   self._lease_component = nil
end

return ReserveBackpackEntityType
