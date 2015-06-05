local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ReserveBackpackSpace = class()

ReserveBackpackSpace.name = 'reserve backpack space'
ReserveBackpackSpace.does = 'stonehearth:reserve_backpack_space'
ReserveBackpackSpace.args = {
   backpack_entity = Entity,           -- entity to reserve
}
ReserveBackpackSpace.version = 2
ReserveBackpackSpace.priority = 1

function ReserveBackpackSpace:start(ai, entity, args)
   if not args.backpack_entity:get_component('stonehearth:backpack'):reserve_space() then
      ai:abort('%s could not reserve backpack space for %s', tostring(entity), tostring(args.backpack_entity))
   end
   self._reserved = true
end

function ReserveBackpackSpace:stop(ai, entity, args)
   if self._reserved then
      args.backpack_entity:get_component('stonehearth:backpack'):unreserve_space()
      self._reserved = false
   end
end

return ReserveBackpackSpace
