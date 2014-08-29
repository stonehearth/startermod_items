local Town = require 'services.server.town.town'
local Point3 = _radiant.csg.Point3

local GetTownCenter = class()

GetTownCenter.name = 'get town center'
GetTownCenter.does = 'stonehearth:get_town_center'
GetTownCenter.args = {
   town = Town   --the town we want to go to
}
GetTownCenter.think_output = {
   location = Point3,      -- the town banner for the entity
}
GetTownCenter.version = 2
GetTownCenter.priority = 1

function GetTownCenter:start_thinking(ai, entity, args)
   local faction = args.town:get_faction()
   local explored_region = stonehearth.terrain:get_visible_region(faction):get()
   local center_location = _radiant.csg.get_region_centroid(explored_region):to_closest_int()
   local proposed_location = Point3(center_location.x, 0, center_location.y)
   local actual_location = radiant.terrain.find_placement_point(proposed_location, 0, 16)
   ai:set_think_output({location = actual_location})
   return
end

return GetTownCenter
