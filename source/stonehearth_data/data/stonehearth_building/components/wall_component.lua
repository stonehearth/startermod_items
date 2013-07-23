local WallComponent = class()

function WallComponent:__init(entity)
   self._entity = entity
end

function WallComponent:extend(json)
end

function WallComponent:start_project(blueprint)
end

return WallComponent
