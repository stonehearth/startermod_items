local Structure = radiant.mods.require('/stonehearth_building/lib/structure.lua')

local ColumnComponent = class(Structure)

function ColumnComponent:__init(entity)
   self[Structure]:__init(entity)
end

function ColumnComponent:extend(json)
   self._colors = {}
   if json.colors then
      for i, rgb in ipairs(json.colors) do
         table.insert(self._colors, radiant.util.colorcode_to_integer(rgb))
      end
   end
end

function ColumnComponent:initialize_from_blueprint(blueprint)
   self._blueprint = blueprint
   --[[
   local color = #self._colors > 0 and self._colors[1] or 0  
   local rgn = self:_modify_region()
   
   rgn:clear()
   rgn:add_cube(Cube3(Point3(0, 0, 0), Point3(1, height, 1), color))
   ]]
end

return ColumnComponent
