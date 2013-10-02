local WallBlueprintComponent = class()

function WallBlueprintComponent:__init(entity)
   self._entity = entity
end

function WallBlueprintComponent:extend(json)
end

function WallBlueprintComponent:set_columns(c0, c1, normal)
   self._c0 = c0
   self._c1 = c1
   self._normal = normal
end

function WallBlueprintComponent:_resize()
   local p0 = self._c0:get_component('mob'):get_location_aligned()
   local p1 = self._c1:get_component('mob'):get_location_aligned()   
end

return WallBlueprintComponent
