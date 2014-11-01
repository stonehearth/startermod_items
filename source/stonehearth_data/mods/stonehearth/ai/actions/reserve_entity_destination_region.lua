local Entity = _radiant.om.Entity
local Region3 = _radiant.csg.Region3
local ReserveEntityDestinationRegion = class()

ReserveEntityDestinationRegion.name = 'reserve entity destination region'
ReserveEntityDestinationRegion.does = 'stonehearth:reserve_entity_destination_region'
ReserveEntityDestinationRegion.args = {
   entity = Entity,
   region = Region3, -- in world space
}
ReserveEntityDestinationRegion.version = 2
ReserveEntityDestinationRegion.priority = 1

-- we allow reservations on regions that are not (currently) in the destination region
function ReserveEntityDestinationRegion:start(ai, entity, args)
   local target = args.entity
   local location = radiant.entities.get_world_grid_location(target)
   local proposed_reserved_region = args.region:translated(-location)

   self._entity_reserved_region = nil
   self._destination_component = target:add_component('destination')
   local reserved_region_boxed = self._destination_component:get_reserved()

   local intersection = proposed_reserved_region:intersected(reserved_region_boxed:get())
   if not intersection:empty() then
      ai:abort('could not reserve region')
   end

   reserved_region_boxed:modify(function(cursor)
         cursor:add_unique_region(proposed_reserved_region)
      end)

   self._entity_reserved_region = proposed_reserved_region
end

function ReserveEntityDestinationRegion:stop(ai, entity, args)
   if self._entity_reserved_region then
      if self._destination_component:is_valid() then
         self._destination_component:get_reserved():modify(function(cursor)
            cursor:subtract_region(self._entity_reserved_region)
         end)
      end
   end

   self._entity_reserved_region = nil
   self._destination_component = nil
end

return ReserveEntityDestinationRegion
