local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local RunToLocation = class()

RunToLocation.name = 'run to location'
RunToLocation.does = 'stonehearth:goto_location'
RunToLocation.args = {
   location = Point3,     -- location to go to
}
RunToLocation.version = 2
RunToLocation.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunToLocation)
         :execute('stonehearth:create_proxy_entity', { location = ai.ARGS.location })
         :execute('stonehearth:goto_entity', { entity = ai.PREV.entity })

