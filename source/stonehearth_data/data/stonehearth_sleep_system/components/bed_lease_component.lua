--[[
   Contains information about the bed leased to this entitiy. The
   bed iself, the lease date, etc.
   
   Leasing a bed is done by invoking lease_bed_to(entity) on the
   bed_component of the bed.
]]
local BedLeaseComponent = class()

function BedLeaseComponent:__init(entity)
   self._entity = entity  -- the entity this component is attached to
   self._bed = nil
end

function BedLeaseComponent:set_bed(bed_entity)
   self._bed = bed_entity
end

function BedLeaseComponent:get_bed()
   return self._bed
end

return BedLeaseComponent