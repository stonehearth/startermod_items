local PetOwner = class()

function PetOwner:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.pets = {}
      self._sv.initialized = true
   end
end

function PetOwner:destroy()
end

function PetOwner:get_pets()
   return self._sv.pets
end

function PetOwner:add_pet(pet)
   if not pet:is_valid() then
      return
   end

   local id = pet:get_id()

   if not self._sv.pets[id] then
      self._sv.pets[id] = pet
      pet:add_component('stonehearth:pet'):set_owner(self._entity)
      self.__saved_variables:mark_changed()
   end
end

function PetOwner:remove_pet(id)
   if self._sv.pets[id] then
      self._sv.pets[id] = nil
      local pet = radiant.entities.get_entity(id)
      if pet then
         pet:add_component('stonehearth:pet'):set_owner(nil)
      end
      self.__saved_variables:mark_changed()
   end
end

-- returns number of pets that are alive
function PetOwner:num_pets()
   local pruned = false
   local count = 0

   for id, pet in pairs(self._sv.pets) do
      if pet:is_valid() then
         count = count + 1
      else
         self._sv.pets[id] = nil
         pruned = true
      end
   end

   if pruned then
      self.__saved_variables:mark_changed()
   end

   return count
end

return PetOwner
