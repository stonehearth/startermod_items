--[[
   Adding this component to an entity makes it lease-able.
]]
local LeaseComponent = class()

function LeaseComponent:initialize(entity, json)
   self._entity = entity  -- the entity we will be leasing
   self._leases = {}
   
   self.__savestate = radiant.create_datastore({
         leases = self._leases,
      })
end

---Lease the entity to someone, if possible
-- @param entity The entity who would like to acquire the lease
-- @returns true if the entity now has the lease, false otherwise
function LeaseComponent:acquire(lease_name, entity)
   if self:can_acquire(lease_name, entity) then
      self._leases[lease_name] = entity
      entity:add_component('stonehearth:lease_holder'):_add_lease(lease_name, self._entity)
      self.__savestate:mark_changed()
      return true
   end
   return false
end

--- Can this entity (optional) acquire this lease?
-- @returns true if the lease is currently empty or if I currently have it, false otherwise
function LeaseComponent:can_acquire(lease_name, entity)
   local owner = self:get_owner(lease_name)
   if not owner or (owner:get_id() == entity:get_id()) then
      return true
   end
   return false
end

function LeaseComponent:get_owner(lease_name)
   return self._leases[lease_name]
end

--- Release the lease from entity
-- If the entity passed in isn't the owner, then don't do anything and
-- return false. If it is the owner, release the lease and return true
-- @param entity The entity who is releaseing the lease
-- @returns true if the release was successful, false otherwise
function LeaseComponent:release(lease_name, entity)
   if self:can_acquire(lease_name, entity) then
      self._leases[lease_name] = nil
      entity:add_component('stonehearth:lease_holder'):_remove_lease(lease_name, self._entity)
      self.__savestate:mark_changed()
      return true
   end
   return false
end


return LeaseComponent