local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ReserveEntityDestination = class()

ReserveEntityDestination.name = 'reserve entity destination'
ReserveEntityDestination.does = 'stonehearth:reserve_entity_destination'
ReserveEntityDestination.args = {
   entity = Entity,           -- entity to reserve
   location = Point3,         -- the point in the region to reserve
}
ReserveEntityDestination.version = 2
ReserveEntityDestination.priority = 1

function ReserveEntityDestination:start(ai, entity, args)
   local target = args.entity
   local location = args.location

   -- the point of interest still needs to be inside the region and
   -- outside the reserve region.  otherwise someone (or something) got
   -- there first!
   local in_region = radiant.entities.point_in_destination_region(target, location)
   if not in_region then
      ai:abort('%s is not in %s region.  cannot be reserved!', tostring(location), tostring(target))
      return
   end

   -- if it's in the region, we know it can't be in the reserved region,
   -- but double check just to make sure
   local in_reserved = radiant.entities.point_in_destination_reserved(target, location)
   if in_reserved then
      ai:abort('%s is in %s reserve region.  cannot be reserved again!!', tostring(location), tostring(target))
      return
   end

   self._destination = target:get_component('destination')
   self._reserved_pt = location - radiant.entities.get_world_grid_location(target)
   self._destination:get_reserved():modify(function(cursor)
      cursor:add_point(self._reserved_pt)
   end)
   --log:debug('finished adding point %s to reserve region', tostring(pt))
end

function ReserveEntityDestination:stop(ai, entity, args)   
   if self._reserved_pt then
      if self._destination:is_valid() then
         self._destination:get_reserved():modify(function(cursor)
            cursor:subtract_point(self._reserved_pt)
         end)
         --log:debug('finished removing point %s from reserve region', tostring(pt))
      end
      self._reserved_pt = nil
      self._destination = nil
   end
end

return ReserveEntityDestination
