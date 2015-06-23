local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ReserveStorageSpace = class()

ReserveStorageSpace.name = 'reserve storage space'
ReserveStorageSpace.does = 'stonehearth:reserve_storage_space'
ReserveStorageSpace.args = {
   storage = Entity,           -- entity to reserve
}
ReserveStorageSpace.version = 2
ReserveStorageSpace.priority = 1

function ReserveStorageSpace:start(ai, entity, args)
   local reserved = args.storage:get_component('stonehearth:storage')
                                    :reserve_space()
   if not reserved then
      ai:abort('%s could not reserve storage space for %s', tostring(entity), tostring(args.storage))
   end
   self._reserved = true
end

function ReserveStorageSpace:stop(ai, entity, args)
   if self._reserved then
      args.storage:get_component('stonehearth:storage')
                     :unreserve_space()
      self._reserved = false
   end
end

return ReserveStorageSpace
