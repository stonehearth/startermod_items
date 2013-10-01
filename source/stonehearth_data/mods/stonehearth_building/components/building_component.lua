local stonehearth_building = require 'stonehearth_building'

local Point3 = _radiant.csg.Point3
local BuildingComponent = class()

function BuildingComponent:__init(entity)
   self._entity = entity
   self._storeys = {}
end

function BuildingComponent:extend(json)
end

function BuildingComponent:add_storey(storey_entity)
   radiant.check.is_entity(storey_entity)

   local y = #self._storeys * stonehearth_building.STOREY_HEIGHT
   
   table.insert(self._storeys, storey_entity)
   radiant.entities.add_child(self._entity, storey_entity, Point3(0, y, 0))
end

return BuildingComponent
