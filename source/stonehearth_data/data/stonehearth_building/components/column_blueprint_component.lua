local Structure = radiant.mods.require('/stonehearth_building/lib/structure.lua')

local ColumnBlueprintComponent = class(Structure)

function ColumnBlueprintComponent:__init(entity)
   self[Structure]:__init(entity)
end

function ColumnBlueprintComponent:extend(json)
   self._colors = {}
   self._height = json.height
   if json.colors then
      for _, rgb in radiant.resources.pairs(json.colors) do
         table.insert(self._colors, radiant.util.colorcode_to_integer(rgb))
      end
   end
end

function ColumnBlueprintComponent:set_height(height)
   self._height = height
end

return ColumnBlueprintComponent
