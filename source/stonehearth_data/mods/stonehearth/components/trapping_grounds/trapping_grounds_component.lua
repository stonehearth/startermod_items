local TrappingGroundsComponent = class()

function TrappingGroundsComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
   else
   end
end

function TrappingGroundsComponent:destroy()
end

function TrappingGroundsComponent:set_size(size)
   self._sv.size = size
   self.__saved_variables:mark_changed()
end

return TrappingGroundsComponent
