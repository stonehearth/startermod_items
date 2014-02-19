local Entity = _radiant.om.Entity
local ReserveEntity = class()

ReserveEntity.name = 'reserve entity'
ReserveEntity.does = 'stonehearth:reserve_entity'
ReserveEntity.args = {
   entity = Entity,           -- entity to reserve
}
ReserveEntity.version = 2
ReserveEntity.priority = 1

function ReserveEntity:start(ai, entity, args)
   local target = args.entity

   self._lease_component = target:add_component('stonehearth:lease')
   if not self._lease_component:acquire(stonehearth.ai.RESERVATION_LEASE_NAME, entity) then
      ai:abort('could not lease %s (%s has it).', tostring(target), tostring(self._lease_component:get_owner(stonehearth.ai.RESERVATION_LEASE_NAME)))
      return
   end
   self._acquired_lease = true
end

function ReserveEntity:stop(ai, entity, args)   
   if self._acquired_lease then
      self._lease_component:release(stonehearth.ai.RESERVATION_LEASE_NAME, entity)
      self._acquired_lease = false
   end
   self._lease_component = nil
end

return ReserveEntity
