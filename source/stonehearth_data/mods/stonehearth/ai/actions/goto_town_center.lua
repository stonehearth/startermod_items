local Town = require 'services.server.town.town'
local Point3 = _radiant.csg.Point3

local GotoTownCenter = class()

GotoTownCenter.name = 'goto town center'
GotoTownCenter.does = 'stonehearth:goto_town_center'
GotoTownCenter.args = {
   town = Town   --the town we want to go to
}
GotoTownCenter.version = 2
GotoTownCenter.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GotoTownCenter)
         :execute('stonehearth:get_town_center', {
            town = ai.ARGS.town
         })
         :execute('stonehearth:goto_closest_standable_location', {
            location = ai.PREV.location,
            max_radius = 16,
         })
