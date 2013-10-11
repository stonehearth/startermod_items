local BuildComponent = require 'services.build.build_component'
local STOREY_HEIGHT = radiant.mods.load('stonehearth').build.STOREY_HEIGHT

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local ColumnComponent = class(BuildComponent)

function ColumnComponent:__init(entity, data_binding)
   self[BuildComponent]:__init(entity, data_binding)

   local cursor = self:get_region():modify()
   cursor:copy_region(Region3(Cube3(Point3(0, 0, 0), Point3(1, STOREY_HEIGHT, 1))))
end

return ColumnComponent
