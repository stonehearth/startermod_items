local BlueprintComponent = class()

function BlueprintComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
end

function BlueprintComponent:extend(json)
end


return BlueprintComponent
