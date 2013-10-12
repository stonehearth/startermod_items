--[[
   Adding this component to an entity makes it lease-able.
]]
local LeaseComponent = class()

function LeaseComponent:__init(entity)
   self._entity = entity  -- the entity we will be leasing
   self._owner = nil      -- the entity that leased us
end

---Lease the entity to someone, if possible
-- @param entity The entity who would like to acquire the lease
-- @returns true if the entity now has the lease, false otherwise
function LeaseComponent:try_to_acquire_lease(entity)
   if self:can_acquire_lease(entity) then
      self._owner = entity
   end
   return self._owner:get_id() == entity:get_id()
end

--- Can this entity (optional) acquire this lease?
-- @returns true if the lease is currently empty or if I currently have it, false otherwise
function LeaseComponent:can_acquire_lease(entity)
   if not self._owner or (self._owner:get_id() == entity:get_id()) then
      return true
   end
   return false
end

function LeaseComponent:get_owner()
   return self._owner
end

--- Release the lease from entity
-- If the entity passed in isn't the owner, then don't do anything and
-- return false. If it is the owner, release the lease and return true
-- @param entity The entity who is releaseing the lease
-- @returns true if the release was successful, false otherwise
function LeaseComponent:release_lease(entity)
   if self:can_acquire_lease(entity) then
      self._owner = nil
      return true
   end
   return false
end


return LeaseComponent