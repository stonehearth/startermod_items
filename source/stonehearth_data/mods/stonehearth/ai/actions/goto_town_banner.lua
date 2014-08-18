local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local Town = require 'services.server.town.town'

local GotoTownBanner = class()

GotoTownBanner.name = 'goto town banner'
GotoTownBanner.does = 'stonehearth:goto_town_center'
GotoTownBanner.args = {
   town = Town
}

--By default, consider wherever the banner is as the town center. 
--If there is no banner, goto_town_center will be called instead

GotoTownBanner.version = 2
GotoTownBanner.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(GotoTownBanner)
         :execute('stonehearth:get_town_banner', {town = ai.ARGS.town})
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.banner
         })
