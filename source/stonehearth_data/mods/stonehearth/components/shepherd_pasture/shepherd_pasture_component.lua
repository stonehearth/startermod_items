local ShepherdPastureComponent = class()

function ShepherdPastureComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   
end

function ShepherdPastureComponent:get_size()
   return self._sv.size
end

function ShepherdPastureComponent:set_size(x, z)
   self._sv.size = { x = x, z = z}
   self.__saved_variables:mark_changed()
end

return ShepherdPastureComponent