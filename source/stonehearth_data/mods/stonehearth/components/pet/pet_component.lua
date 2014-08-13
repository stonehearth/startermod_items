local PetComponent = class()

function PetComponent:__init()
   self._setting_owner = false
end

function PetComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.owner = nil
      self._sv.initialized = true
   end
end

function PetComponent:destroy()
end

function PetComponent:get_owner()
   return self._sv.owner
end

function PetComponent:set_owner(owner)
   if owner ~= self._sv.owner and not self._setting_owner then
      self._setting_owner = true

      local old_owner = self._sv.owner
      self._sv.owner = owner

      if old_owner and old_owner:is_valid() then
         old_owner:add_component('stonehearth:pet_owner'):remove_pet(self._entity:get_id())
      end

      if owner and owner:is_valid() then
         owner:add_component('stonehearth:pet_owner'):add_pet(self._entity)
      end
      self.__saved_variables:mark_changed()

      self._setting_owner = false
   end
end

return PetComponent
