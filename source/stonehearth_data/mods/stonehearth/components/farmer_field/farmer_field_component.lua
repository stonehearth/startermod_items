--[[
   Stores data about the field in question
]]

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

function FarmerFieldComponent:__init(entity, data_binding)
   self._data = data_binding:get_data()
   self._data.size = {0, 0}
   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function FarmerFieldComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
end

function FarmerFieldComponent:set_size(size)
   self._data.size = { size[1], size[2] }
end


return FarmerFieldComponent