local Entity = _radiant.om.Entity
local ReserveEntity = class()

ReserveEntity.name = 'reserve entity'
ReserveEntity.does = 'stonehearth:reserve_entity'
ReserveEntity.args = {
   entity = Entity,           -- entity to reserve
}
ReserveEntity.version = 2
ReserveEntity.priority = 1

function ReserveEntity:start_thinking(ai, entity, args)
   if not stonehearth.ai:can_acquire_ai_lease(args.entity, self._entity) then
      self._log:debug('ignoring %s (cannot acquire ai lease)', args.entity)
      return
   end

   ai:set_think_output()
end

function ReserveEntity:start(ai, entity, args)
   local target = args.entity

   if not stonehearth.ai:acquire_ai_lease(target, entity) then
      ai:abort('could not lease %s (%s has it).', tostring(target), tostring(stonehearth.ai:get_ai_lease_owner(target)))
      return
   end
   self._acquired_lease = true
end

function ReserveEntity:stop(ai, entity, args)   
   if self._acquired_lease then
      stonehearth.ai:release_ai_lease(args.entity, entity)
      self._acquired_lease = false
   end
   self._lease_component = nil
end

return ReserveEntity
