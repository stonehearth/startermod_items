--[[
   Adding this component to an entity makes it lease-able.
]]
local LeaseComponent = class()

function LeaseComponent:initialize(entity, json)
   self._entity = entity  -- the entity we will be leasing
   self._sv = self.__saved_variables:get_data()
   if not self._sv.factions then
      self._sv.factions = {}
   end

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         self:_remove_nonpersistent_leases()
      end)
end

function LeaseComponent:_remove_nonpersistent_leases()
   for _, leases in pairs(self._sv.factions) do
      for lease_name, info in pairs(leases) do
         if not info.persistent then
            info[lease_name] = nil
         end
      end
   end
end

---Lease the entity to someone, if possible
-- @param entity The entity who would like to acquire the lease
-- @returns true if the entity now has the lease, false otherwise
function LeaseComponent:acquire(lease_name, entity, options)
   if self:can_acquire(lease_name, entity) then
      local faction = self:_get_faction(entity)
      local leases = self._sv.factions[faction]
      if not leases then
         leases = {}
         self._sv.factions[faction] = leases
      end

      leases[lease_name] = {
         owner = entity,
         persistent = not options or options.persistent,
      }
      entity:add_component('stonehearth:lease_holder'):_add_lease(lease_name, self._entity)
      self.__saved_variables:mark_changed()
      return true
   end
   return false
end

--- Can this entity (optional) acquire this lease?
-- @returns true if the lease is currently empty or if I currently have it, false otherwise
function LeaseComponent:can_acquire(lease_name, entity)
   local owner = self:get_owner(lease_name, entity)
   local can_acquire = not owner or not owner:is_valid() or owner == entity
   return can_acquire
end

function LeaseComponent:get_owner(lease_name, allied_entity)
   local faction = self:_get_faction(allied_entity)
   local leases = self._sv.factions[faction]
   if not leases then
      return
   end
   local info = leases
   return info and info.owner
end

--- Release the lease from entity
-- If the entity passed in isn't the owner, then don't do anything and
-- return false. If it is the owner, release the lease and return true
-- @param entity The entity who is releaseing the lease
-- @returns true if the release was successful, false otherwise
function LeaseComponent:release(lease_name, entity)
   local faction = self:_get_faction(entity)
   local leases = self._sv.factions[faction]
   if not leases then
      return false
   end
   local info = leases[lease_name]
   if not info or info.owner ~= entity then
      return false
   end

   leases[lease_name] = nil
   entity:add_component('stonehearth:lease_holder'):_remove_lease(lease_name, self._entity)
   self.__saved_variables:mark_changed()
end

function LeaseComponent:_get_faction(entity)
   return radiant.entities.get_player_id(entity)
end

return LeaseComponent
