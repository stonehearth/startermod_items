local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local GetItemLocation = class()

GetItemLocation.name = 'get entity location'
GetItemLocation.does = 'stonehearth:get_entity_location'
GetItemLocation.args = {
   entity = Entity,
}
GetItemLocation.think_output = {
   grid_location = Point3,
}
GetItemLocation.version = 2
GetItemLocation.priority = 1

function GetItemLocation:start_thinking(ai, entity, args)
   local grid_location = radiant.entities.get_world_grid_location(args.entity)
   if grid_location then
      ai:set_think_output({ grid_location = grid_location })
   end
end

return GetItemLocation
