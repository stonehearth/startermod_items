--[[
   Contains information about the leases owned by this entitiy.
   
   Leasing is done by the acquire() and release() methods on the
   lease component.  This stores backpointers to those leases
]]
local LeaseHolderComponent = class()

function LeaseHolderComponent:initialize(entity, json)
   self._entity = entity  -- the entity this component is attached to

   self._sv = self.__saved_variables:get_data()
   if not self._sv.leases then
      self._sv.leases = {}
   end
end

function LeaseHolderComponent:_add_lease(lease_name, entity)
   local leases = self._sv.leases[lease_name]
   if not leases then
      leases = {}
      self._sv.leases[lease_name] = leases
   end
   leases[entity:get_id()] = entity
   self.__saved_variables:mark_changed()
end

function LeaseHolderComponent:_remove_lease(lease_name, entity)
   if entity and entity:is_valid() then
      local leases = self._sv.leases[lease_name]
      if leases then
         leases[entity:get_id()] = nil
         if not next(leases) then
            self._sv.leases[lease_name] = nil
         end
         self.__saved_variables:mark_changed()
      end
   end
end

function LeaseHolderComponent:get_all_leases(lease_name)
   return self._sv.leases[lease_name] or {}
end

function LeaseHolderComponent:get_first_lease(lease_name)
   local leases = self._sv.leases[lease_name]
   if leases then      
      local id, entity = next(leases)
      return entity
   end
end


return LeaseHolderComponent