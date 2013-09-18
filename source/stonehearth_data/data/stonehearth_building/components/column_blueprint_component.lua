local Structure = 'lib.structure'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local ColumnBlueprintComponent = class()

function ColumnBlueprintComponent:__init(entity)
   self._entity = entity
end

function ColumnBlueprintComponent:__tojson()
   return "{}"
end

function ColumnBlueprintComponent:extend(json)
   self._colors = {}
   if json.colors then
      for i, rgb in ipairs(json.colors) do
         table.insert(self._colors, radiant.util.colorcode_to_integer(rgb))
      end
   end
end

function ColumnBlueprintComponent:set_height(height)
   local region = self._entity:get_component('region_collision_shape'):get_region()

   local cursor = region:modify()
   cursor:clear()
   cursor:add_cube(Cube3(Point3(0, 0, 0), Point3(1, height, 1)))
end

return ColumnBlueprintComponent
