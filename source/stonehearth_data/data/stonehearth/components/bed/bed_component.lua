--[[
   Adding this component to an entity makes it a bed, aka something
   that can be slept on. The bed can be leased, which adds the bed_lease_component
   to the entity leasing the bed.
   
   This component, along with bed_lease_component, implement the bed leasing system
   that pairs citizens to beds. 
]]
local BedComponent = class()

function BedComponent:__init(entity)
   self._entity = entity  -- the entity this component is attached to
   self._owner = nil
end

--[[
   Lease a bed to the person sleeping on it.
--]]
function BedComponent:lease_bed_to(entity)
   self._owner = entity
   local bed_lease_component = entity:get_component('stonehearth:bed_lease')
   bed_lease_component:set_bed(self._entity)
end

function BedComponent:get_owner()
   return self._owner
end

return BedComponent