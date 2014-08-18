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

-- This function is a fallback for stonehearth:goto_town_banner. 
-- Since it's at a lower priority, it will run if 
-- there is no banner. Instead, go to somewhere in the center of town
-- This could be someone's house, or a lake, or a farm, so the banner is
-- better. 

local ai = stonehearth.ai
return ai:create_compound_action(GotoTownCenter)
         :execute('stonehearth:get_town_center', { town = ai.ARGS.town })   
         :execute('stonehearth:goto_location', { location = ai.PREV.location })