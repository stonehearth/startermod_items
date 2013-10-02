local StoreyComponent = class()

function StoreyComponent:__init(entity)
   self._entity = entity
end

function StoreyComponent:extend(json)
end

return StoreyComponent
