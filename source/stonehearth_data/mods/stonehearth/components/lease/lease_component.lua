--[[
   Adding this component to an entity makes it lease-able.
]]
local LeaseComponent = class()

function LeaseComponent:__init(entity)
   self._entity = entity  -- the entity we will be leasing
   self._owner = nil      -- the entity that leased us
end

--[[
   Lease the entity to an actor
--]]
function LeaseComponent:lease_to(entity)
   self._owner = entity
end

function LeaseComponent:get_owner()
   return self._owner
end

function LeaseComponent:release_lease()
   self._owner = nil
end

return LeaseComponent