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
   local centroid = _radiant.csg.get_region_centroid(explored_region):to_closest_int()
   local town_center = radiant.terrain.get_point_on_terrain(Point3(centroid.x, 0, centroid.y))
   ai:set_think_output({ location = town_center })
end

return GetTownCenter
